---
layout: post
title: sql
---

SQL middlewares
========================

Silicon provides middlewares to fast access sql databases. It leverages
SQL prepared statements to speedup the execution of queries. It also
caches prepared statements so that one request is only prepared once
per sql session, even if it is call from different procedures.

## Supported databases

  - sqlite
  - mysql

## Getting started

A procedure can take an ```sqlite_connection``` as argument. In case of a
faillure to establish the connection, the framework returns an error
500 to the client and skip the procedure call. Thus, there is no need
to check the validity of the connection at the begining of each
procedure.

Data required to instantiate the connection (i.e. the database filepath) is
passed once to the ```sqlite_middleware``` constructor that is bound the the
api via its method ```bind_middleware```:

```c++
auto api = make_api(

  // The procedure request a connection by simply declaring it as argument.
  _get_user_name(_id = int()) = [] (auto p, sqlite_connection& c)
  {
    std::string name;
    // Lanch a select query an get the result in res.
    // Return false if the query did not return a result compatible with the 
    // type of the variable res.
    if (!(c("SELECT name from users where id = ?")(p.id) >> name))
      throw error::not_found("User with id ", p.id, " not found.");

    return D(_name = name);
  }
)
.bind_middlewares(
  // Bind the sqlite middleware and specify the path to the db file.
  sqlite_middleware("db.sqlite") // sqlite middleware.
);
```


## SELECT queries

In the procedure, the connection object helps to read the result of
SELECT queries via a type safe manner.

```c++
// Read one scalar.
int s;
c("SELECT 1+2")() >> s;


// Read a record.
int age; std::string name;
c("SELECT name, age from users LIMIT 1") >> std::tie(age, name);
// name == "first_user_name"
// age == first_user_age


// Iterate on a list of records:
typedef decltype(r) R;
c("SELECT name, age from users") | [] (std::string& name, int& age) {
  std::cout << name << " " << age << std::endl;
};

// Read a record using a IOD object.
auto r = D(_name = std::string(), _age = int());
c("SELECT name, age from users LIMIT 1") >> r;
// r.name == "first_user_name"
// r.age == first_user_age


// Iterate on a list of records using a IOD object:
typedef decltype(r) R;
c("SELECT name, age from users") | [] (R r) {
  std::cout << r.name << " " << r.age << std::endl;
};

```

## Other queries

```c++
// Execute a query.
std::string n = "John";
int age = 12;
c("INSERT into users(name, age) VALUES (?, ?)")(n, age);
```
