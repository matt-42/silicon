---
layout: post
title: apis
---


APIs and Procedures
=============================

An API is a set of procedures that silicon will serve to the external
world. Remote clients are able to call these functions via http, or
other protocols depending on the backend serving the API.

```c++
#include <silicon/api.hh>

auto my_api = make_api(

  _procedure1 = [] () {
    std::cout << "Hello from procedure1" << std::endl;
  },

  _procedure2(_arg1, _arg2 = int()) = [] (auto param) {
    std::cout << "Hello from procedure2: " << param.arg1 << param.arg2 << std::endl;
  }

);

```

The declaration of a procedure specifies its name, arguments, and function:

```c++
_procedureX(_arg1, _arg2 = int())  = [] (auto param) { return D(_message = "Hello") }
```


__Passing arguments:__ From this signature, the backend deserializes the arguments and pass
them to the ```auto param``` argument of the function. In this case,
```param.arg1``` has the type ```std::string``` and ```param.arg2``` is an
```int```. If the arguments are invalid, the backend will send back an error
code without calling the function.

__The response:__ The return value of the function is serialized and sent to the remote
client.


## Namespaces

A API can include namespaces. They allows to better organise large
APIs.

```c++
auto my_api = make_api(

  _procedure1 = [] () {
    std::cout << "Hello from procedure1" << std::endl;
  },

  _namespace1 = D(
    _procedure2(_arg1, _arg2 = int()) = [] (auto param) {
      std::cout << "Hello from procedure2: " << param.arg1 << param.arg2 << std::endl;
    },
    _procedure3 = [] () {
      std::cout << "Hello from procedure3"  << std::endl;
    }),

  _namespace2 = D(
    _procedure4 = [] () {
      std::cout << "Hello from procedure4"  << std::endl;
    },
    _procedure5 = [] () {
      std::cout << "Hello from procedure5"  << std::endl;
    }),

);

```

The backends usually reflect the namespace hierarchy in the
routes. For example, with a classic http backend, procedure5 will be
accessible under /namespace2/procedure5.
