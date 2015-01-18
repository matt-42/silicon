
The Silicon Web Framework.
=================================

Silicon is a high performance, middleware oriented C++14 http web
framework. It brings to C++ the high expressive power of other web
frameworks based on dynamic languages.



Features
=========================

  - Ease of use:

    The framework is designed such that the developper only writes the core of
    the server, the framework takes care of the rest
    (serialization, deserialization, generation of client libraries,
    routing, multithreading, ...)

  - Fast:

    Finally, C++ has no overhead such as other web programming languages: No
    interpreter, no garbage collection, no virtual machine, and no
    just in time compiler. It relies on a thread based mimosa http server.

  - Verified:

    Thanks to the static programming paradigm of the framework, lots
    of the common errors will be detected and reported by the
    compiler.

  - Automatic dependency injection and middlewares:

    The framework implement a dependency injection patern running at
    compile time. It traverses the dependency graph of the middlewares
    required by each handler, instantiante them to finally call the handler.

  - Automatic validation, serialization and deserialization of json messages:

    Relying on static introspection, a pair fast json parser and
    encoder is generated for each message type. Since the json parser
    knows about the structure of each message, it throws an exeption
    when a field is missing, or when the type of a value is incorect.

  - Automatic generation of the client libraries:

    The framework generates the client libraries. As of today, only
    javascript is supported.

  - Not dependent on the underlying network library or serialization format:

    The abstraction layer is not tight to a specific network library or serialization format.
    The first version of silicon provides simple http backends: microhttpd, mimosa and cpp-netlib,
    and the json format. However, in the future the library could support websocket,
    binary protocols, ...


Installation
=========================

```
git clone https://github.com/matt-42/silicon.git
cd silicon;
./install.sh __YOUR__INSTALL__PREFIX__
```

Hello World
=========================

Here is a simple hello world example serving the string {"message":"hello world."} at the route /hello

```c++
#include <silicon/api.hh>
#include <silicon/mhd_serve.hh>

using namespace sl;

// Define the API:
auto hello_api = make_api(

  // The hello world procedure.
  @hello = [] () { return D(@message = "Hello world."); }

);

int main()
{
  // Serve hello_api via microhttpd using the json format:
  sl::mhd_json_serve(hello_api, 12345);
}
```

Note that the hello_api is not tight to microhttpd, so it can be served
via any other low level network library and any serialization format.

the sl::mhd_json_serve routine setups the json serialization/deserialization and
the microhttpd server.

```
$ curl "http://127.0.0.1:12345/hello"
{"message":"Hello world."}
```

### Compilation

The project requires:
  - A C++14 compiler
  - the microhttpd lib

Because of the extra syntaxic sugar required by the silicon framework, you need
the iod cmake compilation rules:

```
cmake_minimum_required(VERSION 2.8)

find_package(Iod REQUIRED)
find_package(Silicon REQUIRED)

include(${IOD_USE_FILE})

include_directories(${IOD_INCLUDE_DIR} ${SILICON_INCLUDE_DIR})

add_iod_executable(hello_world main.cc)
target_link_libraries(hello_world microhttpd)
```

Procedures with arguments
=========================

The procedures of an API can take several typed arguments. If none
specified, the default type is std::string. The arguments are
automatically deserialized from the body of the underlying network
request and passed to the "auto" parameter of the lambda function:


```c++
// The hello world procedure.
@hello = [] () { return D(@message = "Hello world."); }

// The hello1(name: string) procedure.
@hello1(@name) = [] (auto params) { return D(@message = "Hello " + params.name); }

// The hello2(name:string, age:int) procedure.
@hello2(@name, @age = int()) = [] (auto params) {
  if (params.age > 50)
    return D(@message = "Hello old " + params.name);
  else
    return D(@message = "Hello young " + params.name);
}
```


Middlewares
=========================

It is often usefull for a procedure to gain access to external data,
like for example a database. Procedures request a middleware by simply
declaring it as a parameter.

The following example get data from a sqlite db using a
sqlite_connection middleware:

```c++
#include <silicon/api.hh>
#include <silicon/mhd_serve.hh>
#include <silicon/sqlite.hh>

int main()
{
  using namespace sl;

  auto api = make_api(
    
    @get_user(@id = int()) = [] (auto p, sqlite_connection& c)
    {
      auto res = D(@name = std::string(), @age = int());
      if (!(c("SELECT name, age from users where id = ?", p.id) >> res))
        throw error::not_found("User with id ", p.id, " not found.");

      return res;
    }
    )
    .bind_middlewares(
      sqlite_middleware("db.sqlite") // sqlite middleware.
      );

  sl::mhd_json_serve(api, 12345);

}
```

A sqlite_middleware, the object responsible for sqlite_connection creation, is
added to the api via bind_middlewares.

Procedures can take any number of middlewares.


Middlewares with dependencies
=========================

Middlewares can depend on each others, until there is no dependency
cycles. For example, a sqlite session storage requires a cookie
tracking id to identify a request and an access to a sqlite database.

```c++
struct sqlite_session_middleware
{
  sqlite_session_middleware(const std::string& table_name)
    : table_name_(table_name)
  {
  }

  // Run once at the initialization of the server:
  void initialize(sqlite_connection& c)
  {
    // Create the table if it does not exists
  }

  // Run for every procedure call tacking session_data as argument.
  session_data instantiate(tracking_cookie& cookie, sqlite_connection& con)
  {
    // Fetch the session belonging to cookie.id and using the connection con.
    // Return it.
  }

  std::string table_name_;
};
```

Javascript client
=========================

FIXME

C++ client.
=========================

FIXME

Roadmap.
=========================

###V2
  - Websockets
  - A binary protocol


###V1
  - core of the framework
  - json serialization
  - basic http servers with mimosa, cpp-netlib and microhttpd
  - C++ client
  - Cookie tracking
  - sqlite connection middleware
  - sqlite session middleware
  - sqlite orm
  - Generic CRUD


## Contributing

Contributions are welcome. Silicon can be easily extended with
  - New middlewares
  - New backends (websockets, other non-http protocols)
  - New message formats (messagepack, protobuf, ...)

Do not hesitate to fill issues, send pull requests, or contact me
at matthieu.garrigues@gmail.com.

