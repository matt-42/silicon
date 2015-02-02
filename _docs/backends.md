---
layout: post
title: backends
---


Backends
===================

The backends in Silicon are responsible for:

  - Accepting new clients
  - Managing communication
  - Serialization / deserialization of procedure input / ouput
  - Instantiate the required middlewares
  - Forwarding the remote calls to the API


Given ```my_api``` an API, the following line serves it via the
microhttpd-json backend:

```c++
mhd_json_serve(my_api, 12345);
```

The Silicon code base does not contains low level network
programming. Instead it relies on other libraries like microhttpd to
serve APIs via several protocols.
