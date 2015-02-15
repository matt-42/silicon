---
layout: post
title: Middlewares
---


Middlewares in the Silicon framework
============================================


The design of the Silicon middlewares relies on several concepts:

   - A middleware does one thing and does it well.
   - Some middlewares can rely on others to implement higher level features.
   - Each procedure of a silicon API instantiate only the middleware they explicitely requires.
   - The framework introspects at compile time the signature of
     procedures to setup the instantiation of required the required middlewares.
     This introspection has no cost at runtime.

A middleware is represented by one or two classes:

   - __The instance:__ It contains the code and data exposed to the
     procedures. It is instantiated once per procedure call.
   - __The factory:__ Initialized once at the initialization of the
     server. It manages the instances and can hold data and the states
     needed to provide instances.  The ```instantiate``` method may take
     one or more middleware instances as argument if the instance
     stacks on other middlewares. The factory can also implement a
     initialize method if it relies on other middlewares for its
     initialization. For example, ```sqlite_session::initialize``` 
     relies on a ```sqlite_connection``` to create the session table.

__Stateless factories:__ When a middleware factory does not have to
maintain a state or data to provide instances, the instance class
can directly implement _instantiate_ as a static method.

__Naming Convention:__ Given a middleware, for example a connection to
a mysql database. The instance name should be ```mysql_connection```
and the factory ```mysql_connection_factory```.


Using middlewares
=========================

It is often usefull for a procedure to gain access to external data
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
      if (!(c("SELECT name, age from users where id = ?")(p.id)() >> res))
        throw error::not_found("User with id ", p.id, " not found.");

      return res;
    }
    )
    .bind_factories(
      sqlite_connection_factory("db.sqlite") // sqlite middleware.
      );

  sl::mhd_json_serve(api, 12345);

}
```

A ```sqlite_connection_factory```, the object responsible for the
```sqlite_connection``` creation, is added to the api via
bind_factories.

Procedures can take any number of middlewares as argument, in any order.

Middlewares and dependencies
=========================

Middlewares can depend on each others, until there is no dependency
cycle. For example, a sqlite session storage requires a cookie
tracking id to identify a request and an access to a sqlite database.

```c++
struct sqlite_session_factory
{
  sqlite_session_factory(const std::string& table_name)
    : table_name_(table_name)
  {
  }

  // Run once at the initialization of the server:
  void initialize(sqlite_connection& c)
  {
    // Create the table if it does not exists
  }

  // Run for every procedure call taking session_data as argument.
  session_data instantiate(tracking_cookie& cookie, sqlite_connection& con)
  {
    // Fetch the session belonging to cookie.id and using the connection con.
    // Return it.
  }

  std::string table_name_;
};
```

Global Middlewares
=========================

Operations like logging the requests, monitoring the server load need
to insert code before and after each procedure call. In Silicon, an
API can implicitly attach a set of middlewares to each procedure
call. The instantiate method of these middlewares will run before each
request, and the destructor of the instance after.

To pass a set of global middleware to an API, simply call its
```global_middlewares``` method.

Here is a example showing a very simple example logging the running
time of each request:

```c++
#include <iostream>
#include <silicon/backends/mhd_serve.hh>
#include <silicon/api.hh>

using namespace sl;

struct request_logger
{
  request_logger()
  {
    time = get_time();
    std::cout << "Request start!" << std::endl;
  }

  ~request_logger()
  {
    std::cout << "Request took " << (get_time() - time) << " microseconds." << std::endl;
  }

  inline double get_time()
  {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return double(ts.tv_sec) * 1e6 + double(ts.tv_nsec) / 1e3;
  }

  static auto instantiate() { return request_logger(); }

private:
  double time;
};

auto hello_api = make_api(

  _test = [] () { return D(_message = "hello world."); }

  ).template global_middlewares<request_logger, __other_factory_instance_types__...>>();

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mhd_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
```
