---
layout: post
title: Mimosa backend
---

Mimosa
=====================

This backend wraps [the Mimosa library](https://github.com/abique/mimosa) to serve Silicon APIs:

```c++
#include <silicon/backends/mimosa.hh>
mimosa_json_serve(api, port);
```

The following hello world example ...

```c++
auto hello_api = make_api(

  // The hello world procedure.
  _hello = [] () { return D(_message = "Hello world."); }

);

// Serve hello_api via mimosa using the json format:
mimosa_json_serve(hello_api, 8080);
```

... starts a HTTP server listening on port 8080 and serve the hello world
procedure at the route ```/hello```. It relies on the JSON message
format to communicate with the client.

## Concurency and Parallelism

Mimosa spans one thread per connection.
