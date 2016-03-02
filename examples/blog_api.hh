#include <silicon/api.hh>
#include <silicon/sql_crud.hh>
#include <silicon/hash.hh>
#include <silicon/middlewares/sqlite_connection.hh>
#include <silicon/middlewares/sql_orm.hh>
#include <silicon/middlewares/hashmap_session.hh>

#include "symbols.hh"

using namespace sl;
using namespace s;

typedef decltype(D(_id(_auto_increment, _primary_key) = int(),
                   _login                             = std::string(),
                   _password                          = std::string()))
  user;

typedef decltype(D(_id(_auto_increment, _primary_key) = int(),
                   _user_id(_read_only)               = int(),
                   _title                             = std::string(),
                   _body                              = std::string()))
  post;


typedef sql_orm_factory<sqlite_connection, user> user_orm_factory;
typedef sql_orm<sqlite_connection, user> user_orm;

typedef sql_orm_factory<sqlite_connection, post> post_orm_factory;
typedef sql_orm<sqlite_connection, post> post_orm;

struct session
{
  session() : user_id(-1) {}

  bool authenticate(sqlite_connection& c, std::string login, std::string password)
  {
    auto res = c("SELECT id FROM blog_users where login = ? and password = ?")
      (login, hash_sha3_512(password));
    auto user_exists = !res.empty();
    if (user_exists)
      res >> user_id;
    return user_exists;
  }

  void logout() { user_id = -1; }
  bool is_authenticated() { return user_id != -1; }
  int user_id;
};

struct restricted_area
{
  static restricted_area instantiate(session& u) 
  {
    if (!u.is_authenticated())
      throw error::unauthorized("Only authenticated users can execute this request.");
    return restricted_area();
  }
};


auto blog_api = http_api(
			   
  POST / _login * post_parameters(_login, _password) = [] (auto p, session& s, sqlite_connection& c)
  {
    if (!s.authenticate(c, p.login, p.password))
      throw error::bad_request("Invalid user or password");      
  },

  GET / _logout = [] (session& s, restricted_area)
  {
    s.logout();
  },
    
  _user = sql_crud<user_orm>(

    _before_create = [] (user& u, sqlite_connection& c) {
      if (!c("SELECT * from blog_users where login = ?")(u.login).empty())
        throw error::bad_request("User with login ", u.login, " already exists.");
      u.password = hash_sha3_512(u.password);
    },
    
    _validate = [] (user& u, sqlite_connection& c) {
      return u.login.size() > 0;
    },

    _write_access = [] (user& u, session& s) {
      return u.id == s.user_id;
    }

    ),

  _post = sql_crud<post_orm>(

    _validate = [] (post& p) {
      return p.title.size() > 0 and p.body.size() > 0;
    },

    _before_create = [] (post& p, session& s, restricted_area) {
      p.user_id = s.user_id;
    },

    _write_access = [] (post& p, session& s, restricted_area) {
      return p.user_id == s.user_id;
    }

    )
  );
auto blog_middlewares = middleware_factories(sqlite_connection_factory("blog.sqlite"),
                                       user_orm_factory("blog_users"),
                                       post_orm_factory("blog_posts"),
                                       hashmap_session_factory<session>());
