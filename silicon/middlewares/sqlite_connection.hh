#pragma once

#include <memory>
#include <mutex>
#include <cstring>
#include <unordered_map>

#include <sqlite3.h>
#include <iod/sio.hh>
#include <iod/callable_traits.hh>
#include <silicon/symbols.hh>
#include <silicon/blob.hh>
#include <silicon/null.hh>

namespace sl
{

  void free_sqlite3_statement(void* s)
  {
    sqlite3_finalize((sqlite3_stmt*) s);
  }

  struct sqlite_statement
  {
    typedef std::shared_ptr<sqlite3_stmt> stmt_sptr;

    sqlite_statement() {}

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

      if (empty()) return false;
      row_to_sio(o);
      return true;
    }

    template <typename T>
    int operator>>(T& o) {

      if (empty()) return false;
      this->read_column(0, o);
      return true;
    }

    int last_insert_id()
    {
      return sqlite3_last_insert_rowid(db_);
    }

    int empty() {
      return last_step_ret_ != SQLITE_ROW;
    }

    template <typename... T>
    auto& operator()(T&&... args)
    {
      sqlite3_reset(stmt_);
      sqlite3_clear_bindings(stmt_);
      int i = 1;
      foreach(std::forward_as_tuple(args...)) | [&] (auto& m)
      {
        int err;
        if ((err = this->bind(stmt_, i, m)) != SQLITE_OK)
          throw std::runtime_error(std::string("Sqlite error during binding: ") + sqlite3_errmsg(db_));
        i++;
      };

      last_step_ret_ = sqlite3_step(stmt_);
      if (last_step_ret_ != SQLITE_ROW and last_step_ret_ != SQLITE_DONE)
        throw std::runtime_error(sqlite3_errstr(last_step_ret_));

      return *this;
    }

    template <typename F>
    void operator|(F f)
    {
      while (last_step_ret_ == SQLITE_ROW)
      {
        typedef callable_arguments_tuple_t<F> tp;
        typedef std::remove_reference_t<std::tuple_element_t<0, tp>> T;
        T o;
        row_to_sio(o);
        f(o);
        last_step_ret_ = sqlite3_step(stmt_);
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

    void read_column(int pos, int& v) { v = sqlite3_column_int(stmt_, pos); }
    void read_column(int pos, float& v) { v = sqlite3_column_double(stmt_, pos); }
    void read_column(int pos, double& v) { v = sqlite3_column_double(stmt_, pos); }
    void read_column(int pos, int64_t& v) { v = sqlite3_column_int64(stmt_, pos); }
    void read_column(int pos, std::string& v) {
      auto str = sqlite3_column_text(stmt_, pos);
      auto n = sqlite3_column_bytes(stmt_, pos);
      v = std::move(std::string((const char*) str, n));
    }
    // Todo: Date types
    // template <typename C, typename D>
    // void read_column(int pos, std::chrono::time_point<C, D>& v)
    // {
    //   v = std::chrono::time_point<C, D>(sqlite3_column_int(stmt_, pos));
    // }

    int bind(sqlite3_stmt* stmt, int pos, double d) const { return sqlite3_bind_double(stmt, pos, d); }
    int bind(sqlite3_stmt* stmt, int pos, int d) const { return sqlite3_bind_int(stmt, pos, d); }
    void bind(sqlite3_stmt* stmt, int pos, null_t) { sqlite3_bind_null(stmt, pos); }
    int bind(sqlite3_stmt* stmt, int pos, const std::string& s) const {
      return sqlite3_bind_text(stmt, pos, s.data(), s.size(), nullptr); }
    int bind(sqlite3_stmt* stmt, int pos, const blob& b) const {
      return sqlite3_bind_blob(stmt, pos, b.data(), b.size(), nullptr); }

    sqlite3* db_;
    sqlite3_stmt* stmt_;
    stmt_sptr stmt_sptr_;
    int last_step_ret_;
  };

  void free_sqlite3_db(void* db)
  {
    sqlite3_close_v2((sqlite3*) db);
  }

  struct sqlite_connection
  {
    typedef std::shared_ptr<sqlite3> db_sptr;
    typedef std::unordered_map<std::string, sqlite_statement> stmt_map;
    typedef std::shared_ptr<std::unordered_map<std::string, sqlite_statement>> stmt_map_ptr;
    typedef std::shared_ptr<std::mutex> mutex_ptr;

    sqlite_connection() : db_(nullptr), stm_cache_(new stmt_map()), cache_mutex_(new std::mutex()) // FIXME
    {
    }

    void connect(const std::string& filename, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)
    {
      int r = sqlite3_open_v2(filename.c_str(), &db_, flags, nullptr);
      if (r != SQLITE_OK)
        throw std::runtime_error(std::string("Cannot open database ") + filename + " " + sqlite3_errstr(r));

      db_sptr_ = db_sptr(db_, free_sqlite3_db);
    }

    template <typename E>
    inline void format_error(E&) const {}

    template <typename E, typename T1, typename... T>
    inline void format_error(E& err, T1 a, T... args) const
    {
      err << a;
      format_error(err, args...);
    }

    sqlite_statement operator()(const std::string& req)
    {
      auto it = stm_cache_->find(req);
      if (it != stm_cache_->end()) return it->second;

      sqlite3_stmt* stmt;

      int err = sqlite3_prepare_v2(db_, req.c_str(), req.size(), &stmt, nullptr);
      if (err != SQLITE_OK)
        throw std::runtime_error(std::string("Sqlite error during prepare: ") + sqlite3_errmsg(db_) + " statement was: " + req);

      cache_mutex_->lock();
      auto it2 = stm_cache_->insert(it, std::make_pair(req, sqlite_statement(db_, stmt)));
      cache_mutex_->unlock();
      return it2->second;
    }


    template <typename T>
    inline std::string type_to_string(const T&, std::enable_if_t<std::is_integral<T>::value>* = 0) { return "INTEGER"; }
    template <typename T>
    inline std::string type_to_string(const T&, std::enable_if_t<std::is_floating_point<T>::value>* = 0) { return "REAL"; }
    inline std::string type_to_string(const std::string&) { return "TEXT"; }
    inline std::string type_to_string(const blob&) { return "BLOB"; }

    mutex_ptr cache_mutex_;
    sqlite3* db_;
    db_sptr db_sptr_;
    stmt_map_ptr stm_cache_;
  };

  struct sqlite_database
  {
    sqlite_database() {}

    sqlite_connection& get_connection() {
      return con_;
    }

    template <typename... O>
    void init(const std::string& path, O... options_)
    {
      auto options = D(options_...);

      path_ = path;
      con_.connect(path,  SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
      if (options.has(_synchronous))
      {
        std::stringstream ss;
        ss << "PRAGMA synchronous=" << options.get(_synchronous, 2);
        con_(ss.str())();
      }
    }

    sqlite_connection con_;
    std::string path_;
  };

  struct sqlite_connection_factory
  {
    template <typename... O>
    sqlite_connection_factory(const std::string& database_path, O... options)
    {
      db_.init(database_path, options...);
    }

    sqlite_connection& instantiate()
    {
      return db_.get_connection();
    }

    sqlite_database db_;
  };

}
