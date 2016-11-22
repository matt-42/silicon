#pragma once

#include <random>
#include <silicon/middlewares/tracking_cookie.hh>

namespace sl
{

  template <typename C, typename T>
  struct sql_session
  {
    typedef T data_type;

    sql_session(const std::string& key, const std::string& table_name,
		C& con, int expires)
      : key_(key), table_name_(table_name), con_(con)
    {
      // Get and decode the session data
      //auto sio = iod::cat(T::sio_info(), D(_created_at = sql_date()));

      auto sio = T::sio_info();

      std::stringstream field_ss;
      bool first = true;
      foreach(sio) | [&] (auto& m)
      {
        if (!first) field_ss << ",";
        first = false;
        field_ss << m.symbol().name();
      };
      
      if (con_("SELECT " + field_ss.str() + " from " + table_name_ + " WHERE silicon_session_key = ?")(key) >> sio)
        foreach(sio) | [this] (auto& m) { m.symbol().member_access(data_) = m.value(); };
      else
        con_("INSERT into " + table_name_ + " (silicon_session_key) VALUES (?)")(key);

    }

    data_type* operator->() { return &data_; }
    data_type& data() { return data_; }

    void save()
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
      ss << " WHERE silicon_session_key = \"" << key_ << "\"";

      auto values = foreach(data().sio_info()) | [this] (auto& m)
      {
        return m.symbol() = m.symbol().member_access(data_);
      };

      auto rq = con_(ss.str());
      apply(values, rq);
    };

    void destroy()
    {
      con_("DELETE from  " + table_name_ + " WHERE silicon_session_key = ?")(key_);
    }

    //private:
    T data_;
    std::string key_;
    std::string table_name_;
    C& con_;
  };

  // Session middleware.
  template <typename C, typename D>
  struct sql_session_factory
  {
    template <typename... O>
    sql_session_factory(const std::string& table_name, O... opts)
      : expires_(iod::D(opts...).get(_expires, 10000)),
	table_name_(table_name)
    {
    }

    void initialize(C& c)
    {
      std::stringstream ss;
      ss << "CREATE TABLE if not exists " <<  table_name_ << " ("
         << "silicon_session_key CHAR(32) PRIMARY KEY NOT NULL,";

      bool first = true;
      foreach(D().sio_info()) | [&] (auto& m)
      {
        if (!first) ss << ", ";
        ss << m.symbol().name() << " " << c.type_to_string(m.value());
        first = false;
      };
      ss << ");";
      
      try
      {
        c(ss.str())();
      }
      catch (std::exception e)
      {
        std::cout << "Warning: Silicon could not create the " << table_name_ << " sql table." << std::endl
                  << "You can ignore this message if the table already exists."
                  << "The sql error is: " << e.what() << std::endl;
      }
    }

    typedef sql_session<C, D> session_type;
    session_type instantiate(tracking_cookie& token, C& con)
    {
      return session_type(token.id(), table_name_, con, 10000);
    }

    int expires_;
    std::string table_name_;
  };

}
