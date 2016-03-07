---
layout: documentation
title: SQL middlewares
---

SQL middlewares
========================

Silicon middlewares provides a fast access to sql databases. It
leverages SQL prepared statements to speedup the execution of
queries. It also caches prepared statements so that one request is
only prepared once per sql session, even if it is called from different
procedures.

## Supported databases

  - sqlite
  - mysql

## Getting started

A procedure can take an ```sqlite_connection``` as argument. In case
of a faillure to establish the connection, it throws an internal
server error skipping the procedure call. Thus, there is no need to
check the validity of the connection at the begining of each
procedure.

Data required to instantiate the connection (for sqlite, the database
filepath) is passed once to the ```sqlite_connection_factory```
constructor. ```bind_factory``` binds the middleware to an API:

```c++
auto api = http_api(

  // The procedure request a connection by simply declaring it as argument.
  GET / _get_user_name * get_parameters(_id = int()) = [] (auto p, sqlite_connection& c)
  {
    std::string name;
    // Lanch a select query an get the result in res.
    // Return false if the query did not return a result compatible with the 
    // type of the variable res.
    if (!(c("SELECT name from users where id = ?")(p.id) >> name))
      throw error::not_found("User with id ", p.id, " not found.");

    return D(_name = name);
  }
);
auto middlewares = std::make_tuple(
  sqlite_connection_factory("db.sqlite") // sqlite middleware.
);
```


## SELECT queries

In the procedure, the connection object helps to read the result of
SELECT queries in a type safe manner.

```c++
// Read one scalar.
int s;
c("SELECT 1+2")() >> s;


// Read a record.
int age; std::string name;
c("SELECT name, age from users LIMIT 1")() >> std::tie(name, age);
// name == "first_user_name"
// age == first_user_age


// Iterate on a list of records:
typedef decltype(r) R;
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

```

## Other queries

The middleware can also issue queries that does not return a result set.

```c++
// Execute an insert query.
c("INSERT into users(name, age) VALUES (?, ?)")("John", 12);
```
