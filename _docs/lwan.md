---
layout: documentation
title: Microhttpd backend
---

LWAN backend
=====================

This backend wraps [the LWAN library](http://lwan.ws) to serve Silicon APIs:

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
