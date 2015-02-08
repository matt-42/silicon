#pragma once

#include <random>
#include <silicon/middlewares/tracking_cookie.hh>
#include <silicon/middlewares/sqlite_connection.hh>

namespace sl
{

  template <typename T>
  struct sqlite_session : public T
  {
    typedef T data_type;

    sqlite_session(const std::string& key, const std::string& table_name,
                   sqlite_connection& con)
      : data_type(), key_(key), table_name_(table_name), con_(con)
    {
      // Get and decode the session data
      auto sio = T::sio_info();
      if ((con_("SELECT * from " + table_name_ + " WHERE key = ?")(key) >> sio))
        foreach(sio) | [this] (auto& m) { m.symbol().member_access(*this) = m.value(); };
      else
        con_("INSERT into " + table_name_ + " (key) VALUES (?)")(key);
    }

    data_type& data() { return *static_cast<data_type*>(this); }

    void _save()
    {
      std::stringstream ss;
      ss << "UPDATE " << table_name_ << " SET ";
      bool first = true;
      foreach(data().sio_info()) | [&] (auto& m)
      {
        if (!first) ss << ",";
        first = false;
        ss << m.symbol().name() << " = ? ";
      };
      ss << " WHERE key = \"" << key_ << "\"";

      auto values = foreach(data().sio_info()) | [this] (auto& m)
      {
        return m.symbol() = m.symbol().member_access(this->data());
      };

      auto rq = con_(ss.str());
      apply(values, rq);
    };

    void _destroy()
    {
      con_("DELETE from  " + table_name_ + " WHERE key = ?")(key_);
    }

    //private:
    std::string key_;
    std::string table_name_;
    sqlite_connection con_;
  };

  // Session middleware.
  template <typename D>
  struct sqlite_session_middleware
  {
    sqlite_session_middleware(const std::string& table_name)
      : table_name_(table_name)
    {
    }

    void initialize(sqlite_connection& c)
    {
      std::stringstream ss;
      ss << "CREATE TABLE if not exists " <<  table_name_ << " ("
         << "key CHAR(32) PRIMARY KEY NOT NULL,";

      bool first = true;
      foreach(D().sio_info()) | [&] (auto& m)
      {
        if (!first) ss << ", ";
        ss << m.symbol().name() << " " << c.type_to_string(m.value());
        first = false;
      };
      ss << ");";
      c(ss.str())();
    }

    typedef sqlite_session<D> session_type;
    session_type instantiate(tracking_cookie& token, sqlite_connection& con)
    {
      return session_type(token.id(), table_name_, con);
    }

    std::string table_name_;
  };

}
