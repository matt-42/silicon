#pragma once

#include <mutex>
#include <sstream>
#include <map>
#include <unordered_map>
#include <thread>
#include <memory>
#include <cstring>

#include <mysql/mysql.h>
#include <iod/sio.hh>
#include <iod/tuple_utils.hh>
#include <iod/callable_traits.hh>
#include <silicon/symbols.hh>
#include <silicon/blob.hh>
#include <silicon/null.hh>
#include <silicon/error.hh>

namespace sl
{
  using namespace iod;

  struct mysql_scoped_use_result
  {
    inline mysql_scoped_use_result(MYSQL* con)
      : res_(mysql_use_result(con))
    {
      if (!res_) throw std::runtime_error(std::string("Mysql error: ") + mysql_error(con));
    }

    inline ~mysql_scoped_use_result() { mysql_free_result(res_); }
    inline operator MYSQL_RES*() { return res_; }

  private:
    MYSQL_RES* res_;
  };

  auto type_to_mysql_statement_buffer_type(const char&)          { return MYSQL_TYPE_TINY;     }
  auto type_to_mysql_statement_buffer_type(const short int&)     { return MYSQL_TYPE_SHORT;    }
  auto type_to_mysql_statement_buffer_type(const int&)           { return MYSQL_TYPE_LONG;     }
  auto type_to_mysql_statement_buffer_type(const long long int&) { return MYSQL_TYPE_LONGLONG; }
  auto type_to_mysql_statement_buffer_type(const float&)         { return MYSQL_TYPE_FLOAT;    }
  auto type_to_mysql_statement_buffer_type(const double&)        { return MYSQL_TYPE_DOUBLE;   }
  auto type_to_mysql_statement_buffer_type(const blob&)          { return MYSQL_TYPE_BLOB;     }
  auto type_to_mysql_statement_buffer_type(const char*)          { return MYSQL_TYPE_STRING;   }

  auto type_to_mysql_statement_buffer_type(const unsigned char&)          { return MYSQL_TYPE_TINY;     }
  auto type_to_mysql_statement_buffer_type(const unsigned short int&)     { return MYSQL_TYPE_SHORT;    }
  auto type_to_mysql_statement_buffer_type(const unsigned int&)           { return MYSQL_TYPE_LONG;     }
  auto type_to_mysql_statement_buffer_type(const unsigned long long int&) { return MYSQL_TYPE_LONGLONG; }

  struct mysql_statement
  {
    mysql_statement() : metadata_(nullptr), stmt_(nullptr) {}

    mysql_statement(MYSQL_STMT* s) : stmt_(s)
    {
      metadata_ = mysql_stmt_result_metadata(stmt_);

      metadata_sptr_ = std::shared_ptr<MYSQL_RES>(metadata_, mysql_free_result);
      stmt_sptr_ = std::shared_ptr<MYSQL_STMT>(stmt_, mysql_stmt_close);

      if (!metadata_)
        throw std::runtime_error(std::string("mysql_stmt_result_metadata error: ") + mysql_stmt_error(stmt_));

      fields_ = mysql_fetch_fields(metadata_);
      num_fields_ = mysql_num_fields(metadata_);
    }

    template <typename... T>
    auto& operator()(T&&... args)
    {
      MYSQL_BIND bind[sizeof...(T)];
      memset(bind, 0, sizeof(bind));

      int i = 0;
      foreach(std::forward_as_tuple(args...)) | [&] (auto& m)
      {
        this->bind(bind[i], m);
        i++;
      };

      if (mysql_stmt_bind_param(stmt_, bind) != 0)
        throw std::runtime_error(std::string("mysql_stmt_bind_param error: ") + mysql_stmt_error(stmt_));

      if (mysql_stmt_execute(stmt_) != 0)
        throw std::runtime_error(std::string("mysql_stmt_execute error: ") + mysql_stmt_error(stmt_));
        
      return *this;
    }

    int affected_rows()
    {
      return mysql_stmt_affected_rows(stmt_);
    }

    template <typename V>
    void bind(MYSQL_BIND& b, V& v)
    {
      b.buffer = const_cast<std::remove_const_t<V>*>(&v);
      b.buffer_type = type_to_mysql_statement_buffer_type(v);
      b.is_unsigned = std::is_unsigned<V>::value;
    }

    void bind(MYSQL_BIND& b, std::string& s)
    {
      b.buffer = &s[0];
      b.buffer_type = MYSQL_TYPE_STRING;
      b.buffer_length = s.size();      
    }
    void bind(MYSQL_BIND& b, const std::string& s) { bind(b, *const_cast<std::string*>(&s)); }
    
    void bind(MYSQL_BIND& b, char* s)
    {
      b.buffer = s;
      b.buffer_type = MYSQL_TYPE_STRING;
      b.buffer_length = strlen(s);
    }
    void bind(MYSQL_BIND& b, const char* s) { bind(b, const_cast<char*>(s)); }
    
    void bind(MYSQL_BIND& b, blob& s)
    {
      b.buffer = &s[0];
      b.buffer_type = MYSQL_TYPE_BLOB;
      b.buffer_length = s.size();      
    }
    void bind(MYSQL_BIND& b, const blob& s) { bind(b, *const_cast<blob*>(&s)); }
    
    void bind(MYSQL_BIND& b, null_t& n)
    {
      b.buffer_type = MYSQL_TYPE_NULL;
    }

    template <typename T>
    void fetch_column(MYSQL_BIND*, unsigned long, T&, int) {}
    void fetch_column(MYSQL_BIND* b, unsigned long real_length, std::string& v, int i)
    {
      v.resize(real_length);
      b[i].buffer_length = real_length;
      b[i].length = nullptr;
      b[i].buffer = &v[0];
      if (mysql_stmt_fetch_column(stmt_, b + i, i, 0) != 0)
        throw std::runtime_error(std::string("mysql_stmt_fetch_column error: ") + mysql_stmt_error(stmt_));
    }

    template <typename T>
    void bind_output(MYSQL_BIND& b, unsigned long* real_length, T& v) { bind(b, v); }
    void bind_output(MYSQL_BIND& b, unsigned long* real_length, std::string& v)
    {
      b.buffer_type = MYSQL_TYPE_STRING;
      b.buffer_length = 0;
      b.buffer = 0;
      b.length = real_length;
    }

    template <typename A>
    static constexpr int number_of_fields(const A&) { return 1; }
    template <typename... A>
    static constexpr int number_of_fields(const sio<A...>&) { return sizeof...(A); }
    template <typename... A>
    static constexpr int number_of_fields(const std::tuple<A...>&) { return sizeof...(A); }

    template <typename T>
    int operator>>(T&& o) {

      unsigned long real_lengths[number_of_fields(o)];
      MYSQL_BIND bind[number_of_fields(o)];
      memset(bind, 0, sizeof(bind));

      prepare_fetch(bind, real_lengths, o);
      int f = fetch();
      int res = 1;
      if (f == MYSQL_NO_DATA)
        res = 0;
      else
        finalize_fetch(bind, real_lengths, o);

      mysql_stmt_free_result(stmt_);
      return res;
    }

    template <typename F>
    void operator|(F f)
    {
      typedef std::remove_reference_t<callable_arguments_tuple_t<F>> tp;
      static_if<std::tuple_size<tp>::value == 1>(
        [&, this] (auto* tp_) {
          typedef std::remove_pointer_t<decltype(tp_)> tp2;
          typedef std::remove_reference_t<std::tuple_element_t<0, tp2>> T;
          T o;

          unsigned long real_lengths[number_of_fields(T())];
          MYSQL_BIND bind[number_of_fields(T())];
          memset(bind, 0, sizeof(bind));
      
          prepare_fetch(bind, real_lengths, o);
      
          while (fetch() != MYSQL_NO_DATA)
          {
            finalize_fetch(bind, real_lengths, o);
            f(o);
          }
          mysql_stmt_free_result(stmt_);
        },

        [&, this] (auto* tp_) {
          tuple_remove_references_and_const_t<std::remove_pointer_t<decltype(tp_)>> o;

          unsigned long real_lengths[std::tuple_size<tp>::value];
          MYSQL_BIND bind[std::tuple_size<tp>::value];
          memset(bind, 0, sizeof(bind));
      
          prepare_fetch(bind, real_lengths, o);
      
          while (fetch() != MYSQL_NO_DATA)
          {
            finalize_fetch(bind, real_lengths, o);
            apply(o, f);
          }
          mysql_stmt_free_result(stmt_);
        }, (tp*) 0
        
        );
    }


    template <typename A>
    void prepare_fetch(MYSQL_BIND* bind, unsigned long* real_lengths, A& o)
    {
      if (num_fields_ != 1)
        throw std::runtime_error("mysql_statement error: The number of column in the result set shoud be 1. Use std::tuple or iod::sio to fetch several columns or modify the request so that it returns a set of 1 column.");

      this->bind_output(bind[0], real_lengths, o);
      mysql_stmt_bind_result(stmt_, bind);
    }
    
    template <typename... A>
    void prepare_fetch(MYSQL_BIND* bind, unsigned long* real_lengths, sio<A...>& o)
    {
      if (num_fields_ != sizeof...(A))
        throw std::runtime_error("mysql_statement error: The number of column in the result set does not match the number of attributes of the object to bind.");
      
      foreach(o) | [&] (auto& m) {
        // Find m.symbol().name() position.
        for (int i = 0; i < num_fields_; i++)
          if (!strcmp(fields_[i].name, m.symbol().name()))
            // bind the column.
            this->bind_output(bind[i], real_lengths + i, m.value());
      };
      mysql_stmt_bind_result(stmt_, bind);
    }

    template <typename... A>
    void prepare_fetch(MYSQL_BIND* bind, unsigned long* real_lengths, std::tuple<A...>& o)
    {
      if (num_fields_ != sizeof...(A))
        throw std::runtime_error("mysql_statement error: The number of column in the result set does not match the number of attributes of the tuple to bind.");

      int i = 0;
      foreach(o) | [&] (auto& m) {
        this->bind_output(bind[i], real_lengths + i, m);
        i++;
      };

      mysql_stmt_bind_result(stmt_, bind);
    }
    
    template <typename... A>
    void finalize_fetch(MYSQL_BIND* bind, unsigned long* real_lengths, sio<A...>& o)
    {
      foreach(o) | [&] (auto& m) {
        for (int i = 0; i < num_fields_; i++)
          if (!strcmp(fields_[i].name, m.symbol().name()))
            this->fetch_column(bind, real_lengths[i], m.value(), i);
      };      
    }

    template <typename... A>
    void finalize_fetch(MYSQL_BIND* bind, unsigned long* real_lengths, std::tuple<A...>& o)
    {
      int i = 0;
      foreach(o) | [&] (auto& m) {
        this->fetch_column(bind, real_lengths[i], m, i);
        i++;
      };
    }

    template <typename A>
    void finalize_fetch(MYSQL_BIND* bind, unsigned long* real_lengths, A& o)
    {
      this->fetch_column(bind, real_lengths[0], o, 0);
    }
    
    int fetch()
    {
      int f = mysql_stmt_fetch(stmt_);
      if (f == 1)
        throw std::runtime_error(std::string("mysql_stmt_fetch error: ") + mysql_stmt_error(stmt_));
      return f;
    }
   
    int last_insert_id()
    {
      return mysql_stmt_insert_id(stmt_);
    }

    bool empty() {
      return mysql_stmt_fetch(stmt_) == MYSQL_NO_DATA;
    }

    MYSQL_STMT* stmt_;
    std::shared_ptr<MYSQL_STMT> stmt_sptr_;
    int num_fields_;
    MYSQL_RES* metadata_;
    std::shared_ptr<MYSQL_RES> metadata_sptr_;
    MYSQL_FIELD* fields_;
  };

  struct mysql_connection
  {
    mysql_connection(MYSQL* con)
      : con_(con)
    {
    }

    int last_insert_rowid()
    {
      return mysql_insert_id(con_);
    }
    
    mysql_statement& operator()(std::string rq)
    {
      auto it = stm_cache_.find(rq);
      if (it != stm_cache_.end()) return it->second;

      MYSQL_STMT* stmt = mysql_stmt_init(con_);
      if (!stmt)
        throw std::runtime_error(std::string("mysql_stmt_init error: ") + mysql_error(con_));

      if (mysql_stmt_prepare(stmt, rq.data(), rq.size()))
        throw std::runtime_error(std::string("mysql_stmt_prepare error: ") + mysql_error(con_));

      stm_cache_[rq] = mysql_statement(stmt);
      return stm_cache_[rq];
    }

    template <typename T>
    inline std::string type_to_string(const T&, std::enable_if_t<std::is_integral<T>::value>* = 0) { return "INT"; }
    template <typename T>
    inline std::string type_to_string(const T&, std::enable_if_t<std::is_floating_point<T>::value>* = 0) { return "DOUBLE"; }
    inline std::string type_to_string(const std::string&) { return "MEDIUMTEXT"; }
    inline std::string type_to_string(const blob&) { return "BLOB"; }

    std::unordered_map<std::string, mysql_statement> stm_cache_;
    MYSQL* con_;
  };

  struct mysql_connection_middleware
  {
    template <typename... O>
    mysql_connection_middleware(const std::string& host,
                                const std::string& user,
                                const std::string& passwd,
                                const std::string& database,
                                O... options)
      : host_(host),
        user_(user),
        passwd_(passwd),
        database_(database),
        pool_mutex_(new std::mutex)
    {
      if (mysql_library_init(0, NULL, NULL))
        throw std::runtime_error("Could not initialize MySQL library."); 
      if (!mysql_thread_safe())
        throw std::runtime_error("Mysql is not compiled as thread safe.");
    }

    mysql_connection instantiate()
    {
      auto it = pool_.find(std::this_thread::get_id());
      if (it != pool_.end())
        return mysql_connection(it->second);

      MYSQL* con_ = nullptr;
      con_ = mysql_init(con_);
      con_ = mysql_real_connect(con_, host_.c_str(), user_.c_str(), passwd_.c_str(), database_.c_str() ,0,NULL,0);
      if (!con_)
        throw error::internal_server_error("Cannot connect to the database");

      std::unique_lock<std::mutex> l(*pool_mutex_);
      pool_[std::this_thread::get_id()] = con_;
      return mysql_connection(con_);
    }

    std::shared_ptr<std::mutex> pool_mutex_;
    std::string host_, user_, passwd_, database_;
    std::map<std::thread::id, MYSQL*> pool_;
  };

}
