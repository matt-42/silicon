---
layout: post
title: middlewares
---


Middlewares in the Silicon framework
============================================


The design of the silicon middlewares relies on several concepts:

   - A middleware does one thing and does it well.
   - Middlewares can stack on each others.
   - Each procedure of a silicon API instantiate only the middleware they explicitely requires.
   - The framework introspect at compile time the signature of
     procedures to setup the instantiation of required the required middlewares.
     This introspection has no cost at runtime.

A middleware is represented by one or two classes:

   - __The instance:__ It contains the api exposed to the handler. It is instantiated
     once for each request and if possible recycled for another request. It does not
     have to be thread-safe.
   - __The factory:__ Initialized once at the initialization of the
     server. It manages the instances and can hold data and the states
     needed to create the instances.  The ```instantiate``` method may take
     one or more middleware instances as argument if the instance
     stacks on other middlewares. The factory can also implement a
     initialize method if it relies on other middlewares for its
     initialization. For example, ```sqlite_session::initialize``` relies on
     a ```sqlite_connection``` to create the session table.

__Stateless factories:__ When a middleware factory does not have to
maintain a state or data to create instances, the instance class
implements _instantiate_ as a static method.

__Naming Convention:__ Given a middleware, for example a connection to a mysql
database. The instance name should be ```mysql_connection``` and the factory
```mysql_connection_middleware```. 


Using middlewares
=========================

It is often usefull for a procedure to gain access to external data,
like access to a database. Procedures request a middleware by simply
declaring it as a parameter.

The following example gets data from a sqlite database using the
```sqlite_connection``` middleware:

```c++
#include <silicon/api.hh>
#include <silicon/mhd_serve.hh>
#include <silicon/sqlite.hh>

int main()
{
  using namespace sl;

  auto api = make_api(
    
    _get_user(_id = int()) = [] (auto p, sqlite_connection& c)
    {
      auto res = D(_name = std::string(), _age = int());
      if (!(c("SELECT name, age from users where id = ?", p.id) >> res))
        throw error::not_found("User with id ", p.id, " not found.");

      return res;
    }
    )
    .bind_middlewares(
      sqlite_connection_middleware("db.sqlite") // sqlite middleware.
      );

  sl::mhd_json_serve(api, 12345);

}
```

A ```sqlite_connection_middleware```, the object responsible for the
```sqlite_connection``` creation, is added to the api via
bind_middlewares.

Procedures can take any number of middlewares as argument, in any order.

Middlewares and dependencies
=========================

Middlewares can depend on each others, until there is no dependency
cycle. For example, a sqlite session storage requires a cookie
tracking id to identify a request and an access to a sqlite database.

```c++
struct sqlite_session_middleware
{
  sqlite_session_middleware(const std::string& table_name)
    : table_name_(table_name)
  {
  }

  // Run once at the initialization of the server:
  void initialize(sqlite_connection& c)
  {
    // Create the table if it does not exists
  }

  // Run for every procedure call tacking session_data as argument.
  session_data instantiate(tracking_cookie& cookie, sqlite_connection& con)
  {
    // Fetch the session belonging to cookie.id and using the connection con.
    // Return it.
  }

  std::string table_name_;
};
```
