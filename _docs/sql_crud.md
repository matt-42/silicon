---
layout: documentation
title: SQL generic CReate Update Delete procedures
---

SQL generic CReate Update Delete procedures
===========================

```sql_crud``` facilitates the setup of CRUD procedures in an API:

```c++
// Given a object type
typedef decltype(iod::D(_id(_auto_increment, _primary_key) = int(),
                        _name = std::string(),
                        _age = int(),
                        _address = std::string()
                   )) User;

// And its ORM
typedef sqlite_orm_factory<User> user_orm_factory;

auto api = http_api(
    
      // Attach the set of CRUD procedures to the namespace user.
      // See bellow the option descriptions.
      _user = sql_crud<user_orm>(options...)
);


auto middlewares = std::make_tuple(
  sqlite_connection_factory("test_crud.sqlite"), // sqlite middleware. Set the db filepath.
  user_orm_factory("users") // ORM middleware. Set the users table name.
);
```

This sets up the following procedures:

```javascript
GET user.get_by_id(id: int) -> {id: int, name: string, age: int, address: string}
POST user.create(name: string, age: int, address: string) -> {id: int}
POST user.update(id: int, name: string, age: int, address: string) -> void
POST user.destroy(id: int) -> void
```

## Options

```sql_crud``` takes a set of options to configure the behavior of the
procedures. All options are lambda functions and take as argument
a set of middlewares and the requested object. For example, the
following example checks if the user session has enough privileges to
alter an object:

```c++
sql_crud<user_orm>(
  _read_permission = [] (session& sess, User& u) { 
    return sess.id == u.id; 
  }
);
```

### ```_read_permission```

Default: ```[] () { return true; }```

Check for read permission: return true if the client has enough
privileges to read the requested object. If not, return false.

### ```_write_permission```

Default: ```[] () { return true; }```

Check for write permission: return true if the client has enough
privileges to update the requested object. If not, return false.

### ```_validate```

Default: ```[] () { return true; }```

In the update procedure, validate the state of the updated object. 
Returns true if it is valid, false to abort the update.

### ```_on_create_success```

Default: ```[] () {}```

After a successful object creation, the create procedure calls this
function.

### ```_on_update_success```

Default: ```[] () {}```

After a successful object update, the update procedure calls this
function.

### ```_on_destroy_success```

Default: ```[] () {}```

After a successful object destruction, the destroy procedure calls this
function.

### ```_before_update```

Default: ```[] () {}```

Before saving the object to the database, if _write_access returns
true, the update procedure calls this function.

### ```_before_create```

Default: ```[] () {}```

Before saving the object to the database, if _write_access returns
true, the create procedure calls this function.

