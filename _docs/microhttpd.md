---
layout: documentation
title: Microhttpd backend
---

Microhttpd backend
=====================

This backend wraps [the Microhttpd
library](http://www.gnu.org/software/libmicrohttpd/) to serve Silicon
APIs. It supports the following content-types:
  - application/x-www-form-urlencoded
  - application/json

```c++
mhd_json_serve(api, port, options...);
mhd_json_serve(api, middlewares, port, options...);
```

The hello world example:

```c++
auto hello_api = http_api(

  // The hello world procedure.
  GET / _hello = [] () { return D(_message = "Hello world."); }

);

// Serve hello_api via microhttpd using the json format:
mhd_json_serve(hello_api, 8080);
```

starts a HTTP server listening on port 8080 and serves the hello world
procedure at the route ```/hello```. It relies on the JSON message
format to communicate with the client.

## Concurency and Parallelism

The microhttpd options propose different methods to manage concurent requests
and multi-core architectures.


### ```_one_thread_per_connection```

Spans one thread per connection.

### ```_linux_epoll```

Spans one thread per CPU core, and calls the linux ```epoll``` routine to
manage intra-thread concurency.

### ```_select```

Active by default.

Spans one thread per CPU core, and calls the ```select``` routine to
manage intra-thread concurency.

### ```_nthreads = n```

Default: The number of processor cores.

When using ```_select``` or ```_linux_epoll```, set the size of the
thread pool.


### ```_blocking / _non_blocking```

Default: _blocking

If set as non_blocking, this function returns a silicon_mhd_ctx that
will stop and cleanup the server at the end of its lifetime. If set as
blocking (default), it never returns except if an error prevents or stops
the execution of the server.
