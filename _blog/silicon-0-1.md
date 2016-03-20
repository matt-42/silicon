---
layout: blog
title: Silicon 0.1
date: 2016-03-20
---

# Silicon 0.1

Today I'm happy to announce the first release of the Silicon C++14 web
framework. The goal of this release is to provide a stable library to
easily write fast HTTP APIs with C++. If you don't know Silicon yet, I
invite you to have a look at the [quick
tour](http://siliconframework.org/). Here is some important things to
notice:


- **The finalization of the API embedded domain specific language:**
    HTTP methods are now explicit and parameters can be passed via
    the URL, the GET and/or the POST:

    ```c++
    auto api = http_api(
    // URL parameter
    GET / _hello1 / _name[std::string()] =
           [] (auto p) { return std::string("hello ") + p.name; },
           
    // GET parameter
    GET / _hello2 * get_parameters(_name = std::string()) =
           [] (auto p) { return std::string("hello ") + p.name; },

    // POST parameter
    POST / _hello3 * post_parameters(_name = std::string()) =
           [] (auto p) { return std::string("hello ") + p.name; }

    );
    ```

- **Lower compilation time:** The time spent by the compiler to
    compile silicon APIs has been reduced by 60% thanks to the
    simplification of the IOD dependency injection code. I recommend
    to use clang++ which is faster to compile Silicon's templates.

- **Fast external C backends:** Silicon now wraps two of the
    fastest C webserver libraries to serve its APIs:
    [Microhttpd](http://www.gnu.org/software/libmicrohttpd/), which is
    stable widely use and [LWAN](http://lwan.ws), a faster but experimental one.

- **MySQL and SQLite support**: middlewares are available in this
    release including Mysql and SQLite connections, minimalist object
    relational mapping and session storages.

Checkout [silicon/releases on
GitHub](https://github.com/matt-42/silicon/releases) to try this first
release and do not hesitate to send your feedback or questions on
the [discussion group]
(https://groups.google.com/forum/#!forum/siliconframework)
