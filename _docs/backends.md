---
layout: post
title: backends
---


Backends
===================

The backends in Silicon are responsible for:

  - Accepting new clients
  - Managing communications
  - Calling the deserialization / serialization of procedure input / ouput
  - Instantiating the required middlewares
  - Forwarding the remote calls to the API


Given ```my_api``` an API, ```mhd_json_serve``` serves it via the
microhttpd-json backend:

```c++
mhd_json_serve(my_api, 12345);
```

As of today, the Silicon code base does not contain low level network
programming. Instead, it relies on other libraries like microhttpd to
serve APIs via several protocols.
