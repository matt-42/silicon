---
title: Introduction
layout: post
---

Why Silicon?
=================================

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

A Quick Introduction
=========================

###Hello world

Here is how you could write a simple hello world with silicon.

```c++
// Let's define our API.
auto my_api = http_api(GET / _hello = [] () { return "hello world";});

// Serve it with the microhttpd backend.
auto server = mhd_json_serve(my_api, 8080);
```

###GET parameters

Let's add a GET parameter name.

```c++
// Let's define our API.
auto my_api = http_api(
        GET / _hello * get_parameters(_name = std::string())=
          [] (auto p) { return std::string("hello") + p.name; }
        );
```

###POST parameters

POST parameters do not differ much.

```c++
// Let's define our API.
auto my_api = http_api(
        GET / _hello * post_parameters(_name = std::string())=
          [] (auto p) { return std::string("hello") + p.name; }
        );
```

###URL parameters

Like in REST APIs, parameters can also be part of the url.

```c++
// Let's define our API.
auto my_api = http_api(
        GET / _hello / _name[std::string()] =
          [] (auto p) { return std::string("hello") + p.name; }
        );
```
