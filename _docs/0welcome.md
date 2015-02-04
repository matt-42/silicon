---
layout: post
title: The Silicon Web Framework
---

The Silicon Web Framework
=================================

Silicon is a high performance, middleware oriented C++14 http web
framework. It brings to C++ the high expressive power of other web
frameworks based on dynamic languages.

_The resources of the project are quite limited today (few days per
month of my free time). If you want to support the project, [please
consider
donating](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=E5URY2QDRB54J). This
means more features, middlewares, backends, documentation, and bugs
quickly fixed._


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

    The framework is designed such that the developer only writes the
    core of the API, the framework takes care of the rest
    (serialization, deserialization, generation of client libraries,
    routing, multithreading, ...). Its learning curve is similar to
    frameworks based on nodejs or go.

  - __High performance__

    C++ has no overhead such as other web programming languages: No
    interpreter, no garbage collection, no virtual machine and no just
    in time compiler. Plus, the static paradigm of the framework
    enable the compiler to better optimize the code. Finally, Silicon
    provides several high performance C/C++ server libraries
    leveraging the processing power of multi-cores processors.

  - __Type safe__

    Thanks to the static programming paradigm of the framework, lots
    of the common errors will be detected and reported by the
    compiler.

  - __Automatic dependency injection and middlewares__

    A remote procedure can request access to one ore several
    middlewares. It could access to a session, connection to a
    database or another webservice, or anything else that need to be
    initialized before each api call. Middlewares may depend on each
    other, leading to a middleware dependency graph automatically
    resolved at compile time.

  - __Automatic validation, serialization and deserialization of messages__

    Relying on static introspection on api arguments and return types,
    a pair fast parser and encoder is generated for each message
    type. The request do not reach the api until it contains all the
    required arguments.

  - __Flexible__

    The core of a silicon application server is tied neither to a
    specific message format, neither to a communication backend. In
    other words, you can easily switch from JSON to another binary
    format, or from a thread based to an epoll based http server.

  - __Automatic generation of the client libraries__

    Because the framework knows the input and output type of each api
    procedure, it is able to automatically generate the code of the remote client.
    The C++ client is generated at compile time and other languages are generated
    at runtime thanks to a tiny templating engine.


  - __Coming soon: asynchronism [with the C++17 resumable functions]__
  
    While asynchronism can be implemented with C/C++, it involves
    using callbacks, or javascript-like promises and complexify the
    code. The C++17 will probably standardize the resumable functions
    with the ```async await``` keywords. The Silicon framework will
    leverage this syntax to build async handlers.
