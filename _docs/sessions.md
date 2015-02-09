---
layout: post
title: sessions
---

Sessions
======================

There are several silicon middlewares helping the managment of the
client sessions.


## List of session middlewares:

Given D the object representing the session of one user:

 - ```mysql_session<D>```: Store the session data in a mysql table.
 - ```sqlite_session<D>```: Store the session data in a sqlite table.
 - ```hashmap_session<D>```: Store the session data in an in-memory hash map.

## SQL Session

```c++
struct session_data
{
  my_session() : user_id(0) {}
  auto sio_info() { return D(_user_id = int()); }
  int user_id;
};

int main()
{
  auto api = make_api(
    _set_id(_id = int()) = [] (auto p, sqlite_session<session_data>& sess)
    {
      sess.user_id = p.id;
      sess._save();
    },

    _get_my_id() = [] (sqlite_session<session_data>& sess)
    {
      return D(_id = sess.user_id;
    },

    _logout = [] (sqlite_session<session_data>& sess)
    {
      sess._destroy();
    }

    )
    .bind_middlewares(
      sqlite_middleware("/tmp/sl_test_authentication.sqlite"), // sqlite middleware.
      sqlite_session_middleware<session_data>("sessions") // The sessions will be stored in the "sessions" table.
      );
}

```

