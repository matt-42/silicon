#include <iostream>

#include <iod/sio_utils.hh>

//#include <silicon/microhttpd_backend.hh>
#include <silicon/mimosa_backend.hh>
#include <silicon/sqlite.hh>
#include <silicon/sqlite_session.hh>
#include <silicon/server.hh>

iod_define_symbol(user_id, _User_id);
iod_define_symbol(id, _Id);
iod_define_symbol(name, _Name);
iod_define_symbol(age, _Age);
iod_define_symbol(address, _Address);
iod_define_symbol(salary, _Salary);

#define silicon_params(...) decltype(iod::D(__VA_ARGS__))

using namespace s;
using namespace iod;

typedef decltype(iod::D(_Id = int(),
                        _Name = std::string(),
                        _Age = int(),
                        _Address = std::string(),
                        _Salary = float()
                   )) User;


// ==================================================
// Session
struct session_data
{
  session_data() { user_id = -1; }

  bool authenticated() const { return user_id != -1; }

  auto sio_info() { return D(_User_id = int()); }
  
  int user_id;
};

typedef sqlite_session<session_data> session;

// Create table sessions (key CHAR(32) PRIMARY KEY NOT NULL, user_id INT);
// ==================================================
// Current user.
struct current_user : public User
{
  static auto instantiate(session& sess, sqlite_connection& con)
  {
    if (!sess.authenticated())
      throw error::unauthorized("Access to this procedure is reserved to logged users.");

    current_user u;
    
    if (!(con("SELECT * from user where id = ?", sess.user_id) >> *static_cast<User*>(&u)))
      throw error::internal_server_error("Session user_id not in user table.");

    return u;
  }
};

// 1/ check if the user is valid (user_is_valid function).
// 2/ store data in the session (session.store(user)).

struct authenticator
{
  bool authenticate(int id)
  {
    User u;
    if (con("SELECT id from user where id == ?", id) >> u)
    {
      sess.user_id = id;
      return true;
    }
    else
      return false;
  };
      
  static auto instantiate(session& sess, sqlite_connection& con)
  {
    return authenticator{sess, con};
  }

  session& sess;
  sqlite_connection& con;
};

int main(int argc, char* argv[])
{
  using namespace iod;
  
  auto server = silicon(sqlite_middleware("./db.sqlite"),
                        sqlite_session_storage<session_data>("sessions"));

  server["current_username"] = [] (sqlite_connection& db, current_user& user)
  {
    return user.name;
  };

  server["signin"] = [] (decltype(D(_Id = int())) params,
                         authenticator& auth)
  {
    if (auth.authenticate(params.id))
      return "success";
    else
      return "Invalid name or password";
  };


  // Maybe better?
  // server["signin"](_Username = std::string(),
  //                  _Password = std::string()) =
  //   [] (auto& params, authenticator& auth)
  //   {
  //     if (auth.authenticate(params.name, params.password))
  //       return "success";
  //     else
  //       return "Invalid name or password";
  //   };

  server.serve();
}
