#pragma once

#include <mutex>
#include <sstream>
#include <map>
#include <thread>
#include <memory>
#include <cstring>
#include <boost/lockfree/queue.hpp>

#include <mysql/mysql.h>
#include <iod/sio.hh>
#include <iod/callable_traits.hh>
#include <silicon/symbols.hh>
#include <silicon/blob.hh>
#include <silicon/error.hh>

namespace sl
{
  using namespace iod;
  // template <typename T>
  // inline std::string mysql_type_string(T*, std::enable_if_t<std::is_integral<T>::value>* = 0) { return "INTEGER"; }
  // template <typename T>
  // inline std::string mysql_type_string(T*, std::enable_if_t<std::is_floating_point<T>::value>* = 0) { return "REAL"; }
  // inline std::string mysql_type_string(std::string*) { return "TEXT"; }
  // inline std::string mysql_type_string(blob*) { return "BLOB"; }

  // template <typename T>
  // inline std::string mysql_type_string(const T*) { return mysql_type_string((T*)0); }

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

  struct mysql_statement
  {
    mysql_statement(MYSQL* c) : con_(c)
    {
    }

    template <typename... A>
    void row_to_sio(iod::sio<A...>& o,
                    MYSQL_ROW row,
                    MYSQL_FIELD* fields,
                    unsigned long* lengths,
                    int num_fields)
    {
      int filled[sizeof...(A)];
      for (unsigned i = 0; i < sizeof...(A); i++) filled[i] = 0;

      int i = 0;
      for (int i = 0; i < num_fields; i++)
      {
        const char* cname = fields[i].name;
        bool found = false;
        int j = 0;
        foreach(o) | [&] (auto& m)
        {
          if (!found and !filled[j] and !strcmp(cname, m.symbol().name()))
          {
            this->read_column(row[i], lengths[i], m.value());
            found = true;
            filled[j] = 1;
          }
          j++;
        };
      }
    }
  
    template <typename... A>
    int operator>>(iod::sio<A...>& o) {

      mysql_scoped_use_result res(con_);
      MYSQL_FIELD* fields = mysql_fetch_fields(res);
      int num_fields = mysql_num_fields(res);
      MYSQL_ROW row = mysql_fetch_row(res);

      if (!row) return 0;

      row_to_sio(o, row, fields, mysql_fetch_lengths(res), num_fields);

      return 1;
    }

    template <typename T>
    int operator>>(T& o)
    {
      mysql_scoped_use_result res(con_);
      MYSQL_FIELD* fields = mysql_fetch_fields(res);
      int num_fields = mysql_num_fields(res);
      MYSQL_ROW row = mysql_fetch_row(res);

      if (!row || num_fields == 0) return 0;

      this->read_column(row[0], mysql_fetch_lengths(res)[0], o);

      return 1;
    }

    int last_insert_rowid()
    {
      return mysql_insert_id(con_);
    }

    bool exists() {
      MYSQL_RES* res = mysql_use_result(con_);
      bool e = mysql_fetch_row(res) != 0;
      mysql_free_result(res);
      return e;
    }

    template <typename F>
    void operator|(F f)
    {
      mysql_scoped_use_result res(con_);
      MYSQL_ROW row;
      MYSQL_FIELD* fields = mysql_fetch_fields(res);
      int num_fields = mysql_num_fields(res);
      while ((row = mysql_fetch_row(res)))
      {
        typedef callable_arguments_tuple_t<F> tp;
        typedef std::remove_reference_t<std::tuple_element_t<0, tp>> T;
        T o;
        row_to_sio(o, row, fields, mysql_fetch_lengths(res), num_fields);
        f(o);
      }
    }

    template <typename V>
    void append_to(V& v)
    {
      (*this) | [&v] (typename V::value_type& o) { v.push_back(o); };
    }

    void read_column(const char* col, unsigned long len, int& v) {
      v = 0;
      for (int i = 0; i <= 9  and i < len; i++)
        v = v * 10 + col[i] - '0';
    }

    template <typename T>
    void read_column(const char* col, unsigned long len, T& v) {
      std::istringstream ss(std::string(col, len));
      ss >> v;
      if (!ss.good())
        throw std::runtime_error("Mysql read column: Could not parse the mysql result.");
    }

    void read_column(const char* col, unsigned long len, std::string& v) {
      v = std::string(col, len);
    }

    MYSQL* con_;
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
    
    template <typename... A>
    mysql_statement operator()(A&&... args) const
    {
      std::stringstream ss;
  
      foreach(std::forward_as_tuple(args...)) | [&] (auto& m)
      {
        ss << m;
      };

      if (mysql_real_query(con_, ss.str().c_str(), ss.str().size()))
        throw std::runtime_error(std::string("Mysql error: ") + mysql_error(con_));
      return mysql_statement(con_);
    }
  
    MYSQL* con_;
  };

  struct mysql_middleware
  {
    template <typename... O>
    mysql_middleware(const std::string& host,
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
