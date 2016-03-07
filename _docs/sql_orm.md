---
layout: documentation
title: ORM
---

Object relational mapping
===========================

The SQL ORM middleware stacks on top of the SQL connection middlewares
to implement a basic mapping between C++ object and the database tables.

## Features

  - At initialization, automatically creates the table if it does not already exists.
  - Update
  - Insert
  - Destroy
  - Find by id

## Getting started

The following typedef defines a simple object, with a auto increment attribute on
the field id:

```c++
typedef decltype(D(_id(_auto_increment) = int(),
                   _name = std::string(),
                   _age = int()
                   )) User;
```

An API can rely on the ```sql_orm_factory``` to query, insert, remove,
or update the ```users``` table.

```c++

// Instantiate the user ORM targetting a sqlite database.
typedef sqlite_orm_factory<User> user_orm_factory;
typedef sqlite_orm<User> user_orm;

auto orm_api = http_api(

  GET / _test = [] (user_orm& orm) {
    User u;
    u.name = "John";
    u.age = 42;

    // Insert all the fields expect the id primary keys.
    // return the new id.
    u.id = orm.insert(u);

    u.age++;

    // Save the user name and age members.
    orm.update(u);

    // Destroy the user.
    orm.destroy(u);
    
  }    

);

auto middlewares = std::make_tuple(
    sqlite_connection_factory("./db.sqlite"), // Create the sqlite_connection
    user_orm_factory("users") // The orm takes the table name
);

```
