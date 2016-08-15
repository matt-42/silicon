---
layout: documentation
title: Session middlewares
---

Session middlewares
======================

There are several silicon middlewares helping the management of the
client sessions.


## List of session middlewares:

Given D the object representing the session of one user:

 - ```mysql_session<D>```: Store the session data in a mysql table.
 - ```sqlite_session<D>```: Store the session data in a sqlite table.
 - ```hashmap_session<D>```: Store the session data in an in-memory hash map.

The headers are located under the ```middlewares``` directory:

```c++
#include <silicon/middlewares/mysql_session.hh>
#include <silicon/middlewares/sqlite_session.hh>
#include <silicon/middlewares/hashmap_session.hh>
```

## SQL Session

The following example shows how to implement a simple sqlite
sessions.

```c++
// session_data handles the session data.
struct session_data
{
  // The constructor must create a empty session.
  session_data() : user_id (0) {}
  // The constructor must empty the session
  // so the middleware can recycle it.
  ~session_data() { logout(); }

  // Returns information for introspection on the session_data type.
  auto sio_info() { return D(_user_id = int()); }

  int user_id;
};

using session = sqlite_session<session_data>;

int main()
{
  auto api = http_api(
    GET / _login * get_parameters(_id = int()) = [] (auto p, session& sess)
    {
      // Access to the session_data object with operator->
      sess->user_id = p.id;
      // Access to DB related functions with operator.
      sess.save();
    },

    GET / _get_my_id() = [] (session& sess)
    {
      return D(_id = sess->user_id);
    },

    GET / _logout = [] (session& sess)
    {
      sess.destroy();
    }

    );
  
  auto middlewares = std::make_tuple(
      sqlite_connection_factory("/tmp/sl_test_authentication.sqlite"), // sqlite middleware.
      sqlite_session_factory<session_data>("sessions") // The middleware stores the sessions in the "sessions" table.
      );
}

```

To initialize a sqlite session factory call:
```c++
sqlite_session_factory<__a_session_data_type__>("__the_session_sqlite_table_name__");
```

To initialize a mysql session factory call:
```c++
mysql_session_factory<__a_session_data_type__>("__the_session_mysql_table_name__");
```

## Hashmap Session

```hashmap_session``` stores the sessions in a classic
```std::unordered_map```. This storage does not persits on server
reboot, in opposition to sql session storages.

```c++
struct session_data
{
  // The constructor must create a empty session.
  session_data() : user_id (0) {}
  // The constructor must empty the session
  // so the middleware can recycle it.
  ~session_data() { logout(); }

  void logout() { user_id = 0; }

  int user_id;
};

int main()
{
  auto api = http_api(
    GET / _login * get_parameters(_id = int()) = [] (auto p, session_data& sess)
    {
      sess.user_id = p.id;
    },

    GET / _get_my_id() = [] (session_data& sess)
    {
      return D(_id = sess.user_id;
    },

    GET / _logout = [] (session_data& sess)
    {
      sess.logout();
    }

    ));
  auto middlewares = std::make_tuple(
      hashmap_session_factory() // The middleware factory storing the session hashmap.
      );
}
```

<!--
### Options:

All the session middlewares take the following options:


#### ```expires = N``` 

Default: 10000

Set the session to timeout after N seconds.

Example:
```c++
sqlite_session_factory("db.sqlite", _expires = 3600);
```
-->
