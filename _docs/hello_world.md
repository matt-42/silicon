---
layout: post
title: Hello World
---

Hello World
=========================

Here is a simple hello world example serving the string {"message":"hello world."} at the route /hello.


```c++
#include <silicon/api.hh>
#include <silicon/mhd_serve.hh>

// Declare the iod symbols.
iod_define_symbol(message);
iod_define_symbol(hello);

using namespace sl; // Silicon namespace
using namespace s; // Symbols namespace

// Define the API:
auto hello_api = make_api(

  // The hello world procedure.
  _hello = [] () { return D(_message = "Hello world."); }

);

int main()
{
  // Serve hello_api via microhttpd using the json format:
  sl::mhd_json_serve(hello_api, 12345);
}
```

The D function builds plain C++ objects such that:

```c++
auto o = D(_attr1 = 12, _attr2 = "test");
// is equivalent to:
struct { int attr1; std::string attr2; } o{12, "test"};
```

The only difference is that objects created via D are introspectable, thus
automatically serializable.

Note that the hello_api is not tied to microhttpd, so it can be served
via any other low level network library and any serialization
format.

the sl::mhd_json_serve routine setups the json serialization/deserialization and
the microhttpd server.

```
$ curl "http://127.0.0.1:12345/hello"
{"message":"Hello world."}
```

### Compilation

The hello_world project requires:

  - A C++14 compiler
  - the microhttpd lib

```
g++/clang++ -std=c++14 main.cc -lmicrohttpd
```
