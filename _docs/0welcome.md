---
title: Introduction
layout: post
---

The Silicon Web Framework
=================================

Silicon is a high performance, middleware oriented, C++14 http web
framework. It brings to C++ the high expressive power of other web
frameworks based on dynamic languages without introducing run-time
overhead. Its compile-time static metaprogramming paradigm allows to
match the performances of servers written in C.

Installation
=========================

```
git clone https://github.com/matt-42/silicon.git
cd silicon;
./install.sh __YOUR__INSTALL__PREFIX__
```

Features
=========================

  - __Ease of use__

    Silicon is designed such that the developer only writes the core
    of the API. The framework and the middlewares handle other stuffs
    like serialization, deserialization, generation of the client
    libraries, routing, multithreading, managing a pool of connection
    to a database... Its learning curve is similar to frameworks based
    on nodejs or go.

  - __High performance__

    C++ has no overhead such as other web programming languages: No
    interpreter, no garbage collection, no virtual machine and no just
    in time compiler. Finally, Silicon provides several high
    performance C/C++ server libraries leveraging the processing power
    of multi-core processors.

  - __A compile-time static paradigm for more safety and performances__

    Silicon flexibility entirely runs at compile time. It does not use
    C++ virtual classes or dynamic inheritance. This allows the
    compiler to detect more bugs and to generate faster code.

  - __Automatic dependency injection and middlewares__

    A remote procedure can request access to one or several
    middlewares. Middlewares provide access to external data like a session,
    connection to a database, another webservice, or anything else
    that needs to be initialized before each API call. Middlewares may
    depend on each other, leading to a middleware dependency graph
    automatically resolved at compile time.

  - __Automatic validation, serialization and deserialization of messages__

    The IOD static introspection on API arguments and return types
    enable the implementation of ultra fast message parsers and
    encoders generated at compile time and specialized for each
    message structure. It also allows automatic validation of API
    arguments: the request do not reach the API until it contains all
    the required arguments.

  - __Flexible__

    The core of most silicon application servers is tied neither to a
    specific message format, neither to a communication backend. In
    other words, you can easily switch from JSON to another binary
    format, or from a thread based to an epoll based http server.

  - __Automatic generation of the client libraries__

    Because the framework knows the input and output type of each API
    procedure, it is able to automatically generate the code of the
    remote client. The C++ client is generated at compile time and
    other languages are generated at runtime thanks to a tiny
    templating engine.

<!--
  - __Coming soon: asynchronism [with the C++17 resumable functions]__
  
    While asynchronism can be implemented with C/C++, it involves
    using callbacks, or javascript-like promises and complexify the
    code. The C++17 will probably standardize the resumable functions
    with the ```async await``` keywords. The Silicon framework will
    leverage this syntax to build async handlers.
-->
