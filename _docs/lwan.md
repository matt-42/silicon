---
layout: documentation
title: Microhttpd backend
---

LWAN backend
=====================

This backend wraps [the LWAN library](http://lwan.ws) to serve Silicon APIs.
 It supports the following content-types:
  - application/x-www-form-urlencoded

```c++
lwan_json_serve(api, port, options...);
lwan_json_serve(api, middlewares, port, options...);
```

The hello world example:

```c++
auto hello_api = http_api(

  // The hello world procedure.
  GET / _hello = [] () { return D(_message = "Hello world."); }

);

// Serve hello_api via microhttpd using the json format:
lwan_json_serve(hello_api, 8080);
```

starts a HTTP server listening on port 8080 and serves the hello world
procedure at the route ```/hello```.

### ```_blocking / _non_blocking```

Default: _blocking

If set as non_blocking, this function returns a lwan_ctx that
will stop and cleanup the server at the end of its lifetime. If set as
blocking (default), it never returns except if an error prevents or stops
the execution of the server.
