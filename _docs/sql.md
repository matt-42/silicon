---
layout: post
title: sql
---

SQL middlewares
========================


Silicon provides middlewares to access SQL databases.
Their main features are an easy execution of queries and a type
safe access to SELECT results.

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
  _get_user(_id = int()) = [] (auto p, sqlite_connection& c)
  {
    auto res = D(_name = std::string(), _age = int());

    // Lanch a select query an get the result in res.
    // Return false if the query did not return a result compatible with the 
    // type of the variable res.
    if (!(c("SELECT name, age from users where id = ?", p.id) >> res))
      throw error::not_found("User with id ", p.id, " not found.");

    return res;
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
c("SELECT 1+2") >> s;

// Read a record.
auto r = D(_name = std::string(), _age = int());
c("SELECT name, age from users LIMIT 1") >> r;
// r.name == "first_user_name"
// r.age == first_user_age


// Iterate on a list of records:
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
c("INSERT into users(name, age) VALUES (\"", n, "\", ", age, ")").exec();
```
