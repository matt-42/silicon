---
layout: post
title: microhttpd
---

Microhttpd
=====================

This backend wraps [the Microhttpd library](http://www.gnu.org/software/libmicrohttpd/) to serve Silicon APIs:

```c++
mhd_json_serve(api, port, options...);
```

The hello world example:

```c++
auto hello_api = make_api(

  // The hello world procedure.
  _hello = [] () { return D(_message = "Hello world."); }

);

// Serve hello_api via microhttpd using the json format:
mhd_json_serve(hello_api, 8080);
```

starts a HTTP server listening on port 8080 and serve the hello world procedure
at the route ```/hello```.

## Concurency and Parallelism

By default, it spans one thread per core and use select for
intra-thread concurency. This is the fastest method for procedures
with low running time. The option ```_thread_per_connection``` creates
one thread per connection and enables the long running procedures not
to block the server to accept new requests.

```
mhd_json_serve(api, port, _thread_per_connection);
```
