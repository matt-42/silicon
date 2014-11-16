#pragma once

#include <random>

namespace iod
{
  template <typename D>
  struct sqlite_session_storage;

  template <typename T>
  struct sqlite_session : public T
  {
    typedef T data_type;
    typedef sqlite_session_storage<data_type> middleware_type;

    sqlite_session(const std::string& key, const std::string& table_name,
                   sqlite_connection& con)
    //: data_type(), key_(key), table_name_(table_name), con_(con)
    {
      // Get and decode the session data
      // auto sio = T::sio_info();
      // if ((con_("SELECT * from ? WHERE key = ?", table_name_, key) >> sio))
      //   foreach(sio) | [this] (auto& m) { m.symbol().member_access(*this) = m.value(); };
      // else
      //   con_("INSERT into ? (key) VALUES (?)", table_name_, key);
    }

  //   data_type& data() { return *static_cast<data_type*>(this); }
  //   template <typename U>
  //   auto& field() { return *static_cast<U*>(this); }

  //   void save()
  //   {
  //     std::stringstream ss;
  //     ss << "UPDATE from " << table_name_ << " SET ";
  //     bool first = true;
  //     foreach(data().sio()) | [&] (auto& m)
  //     {
  //       if (!first) ss << ",";
  //       first = false;
  //       ss << m.symbol().name() << " = ? ";
  //     };
  //     ss << " WHERE key = " << key_;

  //     auto values = foreach(data().sio_info()) | [&] (auto& m)
  //     {
  //       return m.symbol() = m.symbol().member_access(data());
  //     };

  //     apply(ss.str().c_str(), values, con_);
  //     // auto args = std::make_tuple(ss.str().c_str(), field<T>().value()...);
  //     // apply(args, con_).exec();
  //   };

  //   void destroy()
  //   {
  //     con_("DELETE from ? WHERE key = ?", table_name_, key_).exec();
  //   }

  // private:
  //   std::string key_;
  //   std::string table_name_;
  //   sqlite_connection& con_;
  };

  // Session middleware.
  template <typename D>
  struct sqlite_session_storage
  {
    sqlite_session_storage(const std::string& table_name)
      : table_name_(table_name)
    {
    }

    std::string&& generate_new_token()
    {
      std::ostringstream os;
      std::random_device rd;
      os << std::hex << rd() << rd() << rd() << rd();
      return std::move(os.str());
    }

    typedef sqlite_session<D> session_type;
    session_type instantiate(request& req, response& resp, sqlite_connection& con)
    {
      std::string token;
      if (!req.get_cookie("token", token))
      {
        token = generate_new_token();
        //resp.set_cookie("token", token);
      }

      //return std::move(session_type());
      return session_type(token, table_name_, con);
    }

    std::string table_name_;
  };

}
