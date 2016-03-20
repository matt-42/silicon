---
title: Introduction
layout: post
---

What is Silicon?
=================================

Silicon is a C++ abstraction built on top of high performance
networking libraries. Its goal is to ease the writing of web APIs without
compromising on performances.

<!--
C++ has never been a popular language to write web servers for many
reasons.  It has a reputation of being a low level language that
leaves the programmer deal with low level things like memory managment
or other error prone tasks. It is also a strongly typed and usually
more verbose than other untyped dynamic languages. It also have a much
longer learning curve.

However, when performance matters, C++ is one of the few choices
left. Simply because it is one of the few languages that does impact the
runtime with a virtual machine, a just in time compiler or a garbage
collector, and because it allows the programmer to directly access low
level optimization like SIMD or thread level parallelism.

Silicon provides the best of both world by providing an almost free
and easy to use abstraction to write web servers and by relying on
fast and scalable C web servers. More than being fast, it is also
secure: No pointer manipulation, no memory managment is left to the
user, and it's static paradigm allows the compiler to detect most of
the errors at compile time. Finally, even if it is C++ and templates,
you will not have to be a C++ master to leverage the framework, no
virtual class, inheritance, or heavy meta programming are required.

Features
=========================

 - GET, POST, and url parameters
 - Databases middlewares
 - Simple object relational mapping
 - Sessions
 - HTTP client
 - LibMicrohttpd and LWAN support
 - Websockets with websocketpp
 - Generation of javascript bindings.

-->

Quick Tour
=========================

###Hello world

A simple mono-procedure api serving a static string under the route /hello.

```c++
auto my_api = http_api(GET / _hello = [] () { return "hello world";});
mhd_json_serve(my_api, 8080);
```

###Returning JSON objects

Silicon relies on static objects (i.e. not hashmaps) to model
objects. They embed static introspection in their type such that the
JSON serializer knows about their members names and types.

```c++
GET / _hi = [] () { return D(_name = "John", _age = 42); }
```

###Passing Parameters

In Silicon, parameters are strongly typed and declared next to the
procedure route. If any error occur during the decoding (i.e. a missing or
ill-formated value), the framework does not call the procedure and
send an error to the client.

```c++
POST / _hello / _id[int()] // Url parameter
   * get_parameters(_name = std::string()) // GET parameter
   * post_parameters(_city = std::string()) // POST parameter

   = [] (auto p) // p holds the 3 typed parameters
   {
     std::ostringstream ss;
     ss << p.name << p.city << p.id;
     return ss.str();
   }
```

###Optional Parameters

All parameters are required by default. GET and POST parameters can be set as optional.

```c++
GET / _hello * get_parameters(_id = optional(int(42)))
```

###Middlewares

Middlewares provide access to the external world (databases, sessions,
...). To require an access to a middleware, the API procedures simply
declare it as argument. If needed, a factory is passed to the backend
in order to instantiate the middleware. For example, the
```mysql_connection_factory``` handles the database information needed to
create a connection.

```c++
auto my_api = http_api(
  GET / _username / _id[int()]
  = [] (auto p, mysql_connection& db) {
    std::string name;
    db("SELECT name from User where id = ?")(id) >> name;
    return D(_name = name);
  }
);

auto middlewares = std::make_tuple(
   mysql_connection_factory("localhost", "user", "password", "database_name")
);
mhd_json_serve(my_api, middlewares, 8080);

```

Procedures can take any number of middlewares in any order. The
framework takes care of instantiating and passing the middlewares to
the procedure in the right order. If an instantiation fails, Silicon
does not call the procedure and send an error to the client.

###MySQL and Sqlite middlewares

Silicon provides an abstraction around the low level C client libraries of MySQL and Sqlite.

```c++
GET / _mysql = [] (auto p, mysql_connection& db) {
    // Read one scalar.
    int s;
    c("SELECT 1+2")() >> s;


    // Read a record.
    int age; std::string name;
    c("SELECT name, age from users LIMIT 1")() >> std::tie(name, age);
    // name == "first_user_name"
    // age == first_user_age


    // Iterate on a list of records:
    c("SELECT name, age from users")() | [] (std::string& name, int& age) {
      std::cout << name << " " << age << std::endl;
    };

    // Read a record using a IOD object.
    auto r = D(_name = std::string(), _age = int());
    c("SELECT name, age from users LIMIT 1")() >> r;
    // r.name == "first_user_name"
    // r.age == first_user_age


    // Iterate on a list of records using a IOD object:
    typedef decltype(r) R;
    c("SELECT name, age from users")() | [] (R r) {
      std::cout << r.name << " " << r.age << std::endl;
    };

    // Inject a variable into a SQL request.
    int id = 42;
    std::string name;
    db("SELECT name from User where id = ?")(id) >> name;
}
```

###Errors

When a procedure cannot complete because of an error, it throws an
exception describing the cause of the error.

```c++
GET / _test / _id[int()] = [] (auto p)
{
  if (p.id != 42)
    // Send an error 401 Unauthorized to the client.
    throw error::unauthorized("Wrong ID");
  return "success";
}
```

###Sessions

The framework also provides session middlewares. It enable an API to
store information about the current user in a database, or in an
in-memory hashtable.

```c++
struct session
{
  int id;
};

auto api = http_api(

    GET / _set_id / _id[int()] = [] (auto p, session& s)
    {
      s.id = p.id;
    },

    GET / _get_id = [] (session& s) {
      return D(_id = s.id);
    }
);

auto middlewares = std::make_tuple(
   hashmap_session_factory<session>()
);

mhd_json_serve(my_api, middlewares, 8080);
```
