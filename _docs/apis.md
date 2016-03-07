---
layout: documentation
title: APIs and Procedures
---


APIs and Procedures
=============================

An API is a set of procedures that Silicon serves to the external
world. Remote clients are able to call these functions via http, or
other protocols depending on the backend serving the API.

```c++
#include <silicon/api.hh>

auto my_api = http_api(

  GET / _procedure1 = [] () {
    std::cout << "Hello from procedure1" << std::endl;
  },

  GET / _procedure2 * get_parameters(_arg1, _arg2 = int()) = [] (auto param) {
    std::cout << "Hello from procedure2: " << param.arg1 << param.arg2 << std::endl;
  }

);
```

## Procedure Parameters

There is 3 way to pass parameters to procedures. They are all strongly
typed. The framework takes care of the deserialisation and does not
call the procedure in case of invalid argument.

Via the url:

```c++
// /procedureX/1234
GET / _procedureX / _id[int()]  = [] (auto param) { return D(_id = param.id);  }
```

Via the GET parameters:

```c++
// /procedureX?id=1234
GET / _procedureX * get_parameters(_id = int())  = [] (auto param) { return D(_id = param.id); }
```

Via the POST parameters:

```c++
// /procedureX (with the body set as "id=1234")
GET / _procedureX * post_parameters(_id = int())  = [] (auto param) { return D(_id = param.id); }
```


## Sending a Response

The return value of the procedure is serialized and sent back to the client.
You can return strings or SIO objects (built with the D function).

## HTTP Errors

When an error occurs and the procedure should not complete, just throw an exeption:

```c++
GET / _login * get_parameters(_id = int())  = [] (auto param) {
    if (param.id != 42) throw error::unauthorized("id ", param.id, " is not allowed.");
    return "welcome 42";
}
```

The framework catch it and send it to the client.


## Namespaces

An API may include namespaces. They allows to better organise large
APIs and for example, to define sub apis in other header files.

```c++
auto my_api = http_api(

  GET / _procedure1 = [] () {
    std::cout << "Hello from procedure1" << std::endl;
  },

  _namespace1 = http_api(
    GET / _procedure2 * get_parameters(_arg1, _arg2 = int()) = [] (auto param) {
      std::cout << "Hello from namespace1.procedure2: "
      		<< param.arg1 << param.arg2 << std::endl;
    },
    GET / _procedure3 = [] () {
      std::cout << "Hello from namespace1.procedure3"  << std::endl;
    }),

  _namespace2 = http_api(
    GET / _procedure4 = [] () {
      std::cout << "Hello from namespace2.procedure4"  << std::endl;
    },
    GET / _procedure5 = [] () {
      std::cout << "Hello from namespace2.procedure5"  << std::endl;
    })

);

```

The backends reflect the namespace hierarchy in the routes. For
example, with a classic http backend, procedure5 will be accessible
under /namespace2/procedure5.
