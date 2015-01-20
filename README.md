
The Silicon Web Framework.
=================================

Silicon is a high performance, middleware oriented C++14 http web
framework. It brings to C++ the high expressive power of other web
frameworks based on dynamic languages.

Note that the framework relies on the @ symbol which is not part of C++14. It
is compiled into plain C++ via a preprocessor.

Features
=========================

  - Ease of use:

    The framework is designed such that the developper only writes the core of
    the API, the framework takes care of the rest
    (serialization, deserialization, generation of client libraries,
    routing, multithreading, ...)

  - Fast:

    C++ has no overhead such as other web programming
    languages: No interpreter, no garbage collection, no virtual
    machine and no just in time compiler. It provides several http
    server backend: microhttpd, mimosa and cpp-netlib.

  - Verified:

    Thanks to the static programming paradigm of the framework, lots
    of the common errors will be detected and reported by the
    compiler.

  - Automatic dependency injection and middlewares:

    Middlewares are objects requests by the api. They can be access to a session,
    connection to a database or another webservice, or anything else that need
    to be initialized before each api call. Middlewares may depend on each other,
    leading to a middleware dependency graph automatically resolved at compile time.

  - Automatic validation, serialization and deserialization of messages:

    Relying on static introspection on api arguments and return types,
    a pair fast parser and encoder is generated for each message
    type. The request do not reach the api until it contains all the
    required arguments.

  - Automatic generation of the client libraries:

    Because the framework knows the input and output type of each api
    procedure, it is able to automatically generate the code of the remote client.
    The C++ client is generated at compile time and other languages are generated
    at runtime thanks to a tiny templating engine.

  - Not dependent on the underlying network library or serialization format:

    The core code of an API is not tied to a specific network library or serialization format.
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

Here is a simple hello world example serving the string {"message":"hello world."} at the route /hello.


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

The D function create plain C++ object such that:
```c++
auto o = D(@attr1 = 12, @attr2 = "test");
// is equivalent to:
struct { int attr1; std::string attr2; } o{12, "test"};
```
The only difference is that objects created via D are introspectable, thus
automatically serializable.

Note that the hello_api is not tied to microhttpd, so it can be served
via any other low level network library and any serialization format.

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
When running cmake, do not forget to pass the location of the silicon install directory:
```
cmake __HELLO_WORLD_DIR__ -DCMAKE_PREFIX_PATH=__YOUR__INSTALL__PREFIX__
```

Procedures with arguments
=========================

The procedures of an API can take several typed arguments. If none
specified, the default type of an argument is std::string. The arguments are
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
like for example access to a database. Procedures request a middleware by simply
declaring it as a parameter.

The following example gets data from a sqlite database using a
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

A sqlite_middleware, the object responsible for the sqlite_connection creation, is
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


C++ Remote Client
=========================

The C++ remote client is automatically built upon the api of the server. It allows to
remotely call procedures on the server as if they were real C++ functions. All the
serializations, network communications, deserializations are automatically generated
at compile time.

Given an API:
```c++
auto api = make_api(
  @hello_world(@name) = [] (auto p)
  {
    return D(@message = std::string("Hello ") + p.name);
  });
```

And a server:
```c++
mimosa_json_serve(api, 12345);
```

The client is:
```c++
// Instantiate the client
auto c = json_client(api, "127.0.0.1", 12345);

// Blocking call to http://127.0.0.1:12345/hello_world("John")
auto r = c.hello_world("John");

assert(r.response.message == "Hello John");
assert(r.status == 200);
```

Javascript Remote Client
=========================

Silicon also provides an automaticaly generated javascript client. The js source can be
served directly served from the api:

```c++

// Predeclare the js source string.
std::string js_client;
  
auto hello_api = make_api(

  @hello_world(@name) = [] (auto p)
  {
    return D(@message = std::string("Hello ") + p.name);
  });

  // Serve it at /js_client
  @js_client = [&] () { return js_client; }

  );

// Generate the js source.
js_client = generate_javascript_client(hello_api);

// Start the server.
sl::mimosa_json_serve(hello_api, 12345);
```

A browser could directly interact with the server with the following html javascript
page:

```html
<script type="text/javascript" src="http://127.0.0.1:12345/js_client"></script>

<script type="text/javascript">

// Set the address of the server.
sl.set_server("http://127.0.0.1:12345");
// Call the hello_world method
sl.hello_world({name: "John"}).then(function (r) { console.log(r); });
// The response r is the Json object returned by the server: {message: "Hello John"}

</script>
```


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

Donations
================================

The ressources of the project are quite limited today (few days per month of my free time). If you want to support the project, [please consider donating here](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=E5URY2QDRB54J). This means more features, middlewares, backends, documentation, and bugs quickly fixed.

Contributing
===========================

Contributions are welcome. Silicon can be easily extended with
  - New middlewares
  - New backends (websockets, other non-http protocols)
  - New message formats (messagepack, protobuf, ...)

Do not hesitate to, fill issues, send pull requests, or contact me
at matthieu.garrigues@gmail.com.

