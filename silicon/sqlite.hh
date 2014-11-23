#pragma once

#include <memory>
#include <cstring>
#include <boost/lockfree/queue.hpp>

#include <sqlite3.h>
#include <iod/sio.hh>
#include <iod/callable_traits.hh>

namespace iod
{

  struct blob : public std::string
  {
    blob() : std::string() {}
    blob(const std::string& s) : std::string(s) {}
    blob(const blob& b) : std::string(b) {}
  };

  static const struct null_t { null_t() {} } null;

  void free_sqlite3_statement(void* s)
  {
    sqlite3_finalize((sqlite3_stmt*) s);
  }

  struct sqlite_statement
  {
    typedef std::shared_ptr<sqlite3_stmt> stmt_sptr;
    
    sqlite_statement(sqlite3* db, sqlite3_stmt* s) : db_(db), stmt_(s),
                                   stmt_sptr_(stmt_sptr(s, free_sqlite3_statement))
    {
    }

    template <typename... A>
    void row_to_sio(iod::sio<A...>& o)
    {
      int ncols = sqlite3_column_count(stmt_);
      int filled[sizeof...(A)];
      for (unsigned i = 0; i < sizeof...(A); i++) filled[i] = 0;

      for (int i = 0; i < ncols; i++)
      {
        const char* cname = sqlite3_column_name(stmt_, i);
        bool found = false;
        int j = 0;
        foreach(o) | [&] (auto& m)
        {
          if (!found and !filled[j] and !strcmp(cname, m.symbol().name()))
          {
            this->read_column(i, m.value());
            found = true;
            filled[j] = 1;
          }
          j++;
        };
      }
    }
  
    template <typename... A>
    int operator>>(iod::sio<A...>& o) {

      if (sqlite3_step(stmt_) != SQLITE_ROW)
        return false;

      row_to_sio(o);
      return true;
    }

    template <typename T>
    int operator>>(T& o) {

      if (sqlite3_step(stmt_) != SQLITE_ROW)
        return false;

      this->read_column(0, o);
      return true;
    }
    
    int exec() {
      int ret = sqlite3_step(stmt_);
      if (ret != SQLITE_ROW and ret != SQLITE_DONE)
        throw std::runtime_error(sqlite3_errstr(ret));

      return ret == SQLITE_ROW or ret == SQLITE_DONE;
    }

    int last_insert_rowid()
    {
      return sqlite3_last_insert_rowid(db_);
    }

    int exists() {
      return sqlite3_step(stmt_) == SQLITE_ROW;
    }

    template <typename F>
    void operator|(F f)
    {
      while (sqlite3_step(stmt_) == SQLITE_ROW)
      {
        typedef callable_arguments_tuple_t<F> tp;
        typedef std::remove_reference_t<std::tuple_element_t<0, tp>> T;
        T o;
        row_to_sio(o);
        f(o);
      }
    }

    template <typename V>
    void append_to(V& v)
    {
      (*this) | [&v] (typename V::value_type& o) { v.push_back(o); };
    }

    template <typename T>
    struct typed_iterator
    {
      template <typename F> void operator|(F f) const { (*_this) | [&f] (T& t) { f(t); }; }
      sqlite_statement* _this;
    };

    template <typename... A>
    auto operator()(A&&... typeinfo)
    {
      return typed_iterator<decltype(iod::D(typeinfo...))>{this};
    }

    void read_column(int pos, int& v) { v = sqlite3_column_int(stmt_, pos); }
    void read_column(int pos, float& v) { v = sqlite3_column_double(stmt_, pos); }
    void read_column(int pos, double& v) { v = sqlite3_column_double(stmt_, pos); }
    void read_column(int pos, int64_t& v) { v = sqlite3_column_int64(stmt_, pos); }
    void read_column(int pos, std::string& v) {
      auto str = sqlite3_column_text(stmt_, pos);
      auto n = sqlite3_column_bytes(stmt_, pos);
      v = std::move(std::string((const char*) str, n));
    }

    sqlite3* db_;
    sqlite3_stmt* stmt_;
    stmt_sptr stmt_sptr_;
  };


  void free_sqlite3_db(void* db)
  {
    sqlite3_close_v2((sqlite3*) db);
  }

  struct sqlite_middleware;
  struct sqlite_connection
  {
    typedef sqlite_middleware middleware_type;

    typedef std::shared_ptr<sqlite3> db_sptr;

    sqlite_connection() : db_(nullptr)
    {
    }

    void connect(const std::string& filename, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)
    {
      int r = sqlite3_open_v2(filename.c_str(), &db_, flags, nullptr);
      if (r != SQLITE_OK)
        throw std::runtime_error(std::string("Cannot open database ") + filename + " " + sqlite3_errstr(r));

      db_sptr_ = db_sptr(db_, free_sqlite3_db);
    }


    int bind(sqlite3_stmt* stmt, int pos, double d) const { return sqlite3_bind_double(stmt, pos, d); }
    int bind(sqlite3_stmt* stmt, int pos, int d) const { return sqlite3_bind_int(stmt, pos, d); }
    void bind(sqlite3_stmt* stmt, int pos, null_t) { sqlite3_bind_null(stmt, pos); }
    int bind(sqlite3_stmt* stmt, int pos, const std::string& s) const {
      return sqlite3_bind_text(stmt, pos, s.data(), s.size(), nullptr); }
    int bind(sqlite3_stmt* stmt, int pos, const blob& b) const {
      return sqlite3_bind_blob(stmt, pos, b.data(), b.size(), nullptr); }

    template <typename E>
    inline void format_error(E&) const {}

    template <typename E, typename T1, typename... T>
    inline void format_error(E& err, T1 a, T... args) const
    {
      err << a;
      format_error(err, args...);
    }

    int last_insert_rowid()
    {
      return sqlite3_last_insert_rowid(db_);
    }
    
    template <typename... A>
    sqlite_statement operator()(const std::string& req, A&&... args) const
    {
      sqlite3_stmt* stmt;

      int err = sqlite3_prepare_v2(db_, req.c_str(), req.size(), &stmt, nullptr);
      if (err != SQLITE_OK)
        throw std::runtime_error(std::string("Sqlite error during prepare: ") + sqlite3_errmsg(db_) + " statement was: " + req);
  
      int i = 1;
      foreach(std::forward_as_tuple(args...)) | [&] (auto& m)
      {
        int err;
        if ((err = this->bind(stmt, i, m)) != SQLITE_OK)
          throw std::runtime_error(std::string("Sqlite error during binding: ") + sqlite3_errmsg(db_));
        i++;
      };

      return sqlite_statement(db_, stmt);
    }
  
    sqlite3* db_;
    db_sptr db_sptr_;
  };

  struct sqlite_database
  {
    sqlite_database() {}

    sqlite_connection& get_connection() {
      return con_;
    }

    void init(const std::string& path)
    {
      path_ = path;
      con_.connect(path,  SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
      con_("PRAGMA synchronous=0").exec();
    }

    sqlite_connection con_;
    std::string path_;
  };

  struct sqlite_middleware
  {
    sqlite_middleware(const std::string& database_path)
    {
      db_.init(database_path);
    }

    sqlite_connection& instantiate()
    {
      return db_.get_connection();
    }

    sqlite_database db_;
  };

}
