
The Silicon Web Framework.
=================================

Note: This project is a work in progress.

Silicon is a high performance, middleware oriented C++14 http web
framework. It brings to C++ the high expressive power of other web
frameworks based on dynamic languages, without compromising on
performances.

Here is a example of simple hello server:

```c++
int main(int argc, char* argv[])
{
  auto server = silicon();
  server.route("/hello")(_Name) = [] (auto p) { return "Hello " + p.name; };
  server.serve(8888);
}
```

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

  - Dependency injection:

    The framework implement a dependency injection patern running at
    compile time. It traverses the dependency graph of the middlewares
    required by each handler, instantiante them to finally call the handler.

  - Automatic validation, serialization and deserialization of json messages:

    Thanks to static introspection and meta programming, a pair json
    parser and encoder is generated for each message type. No hash
    table and other dynamic structures as in all other json
    frameworks. Since the json parser knows the structure of each
    message, it throw exeption when a required field is missing, or
    when the type of a value is incorect.

  - Automatic generation of the client libraries:

    Again, thanks to static introspection, the framework generates the
    client libraries. As of today, only javascript is supported.

Getting Started
=========================

Procedures bodies can have different forms, the simplest procedure is a string:

```c++
namespace sl = silicon
int main(int argc, char* argv[])
{
  // Instantiate a server.
  auto s = silicon();

  // Create the Hello world procedure.
  s["hello_world"] = "Hello world!";
  // ^^^^^^^^^^^ -> This will be the name of the procedure in the client libraries
  //                (see below the javascript example).

  // Generate the javascript client libraries and serve it on a specific route.
  s.route("/bindings.js") = generate_javascript_bindings(s, _Module = "my_api");
 
  s.serve(8888);
}
```

The javascript client would be:

```javascript
<script type="text/javascript" src="http://my_server_path/bindings.js" />

<script>
        my_api.hello_world().then(function(r){ document.write(r); });
</script>
```

Procedures
========================

Silicon procedures can have different types:

```c++

  // A lambda function returning a string.
  s["lambda1"] = [] () { return "Hello from lambda"; };

  // Or a iod object to be serialized in json.
  s["lambda2"] = [] () { return D(_Name = "Paul", _City = "Paris"); };

  // A procedure taking parameters.
  // The type of the parameter must be specified with the function D.
  s["lambda3"](_Name) = [] (auto params)
  { 
    return "Hello" + params.name; 
  };

  // A function taking the response object, for example to set the response headers.
  s["lambda2"] = [] (response& response) {
     response << iod::D(_Message = "Hello World");
  };

  // A raw C++ function:
  auto my_function() { return D(_Message = "Hello world"); }

  [...]

  s["classic_function"] = my_function;


```

Procedures arguments, DI and Middlewares
=========================

At compile time, Silicon analyses the arguments of the procedures and
generates via meta programmation the argument list
construction. Hence, there is not restriction on the arguments order
and procedures instantiate only the middleware they need. For example,
even if a user owns a session, not all the procedures actually need to
read data from this session. Some cpu cycles are then saved if
only the minimum set of procedures instantiate the session.


```c++
  // Instantiates a middleware
  s["session"] = [] (my_middleware& session) {
     return D(_Message = "Hello " + session.username);
  };
```

Prolog and Epilog
=========================

Ofter used for logging, prolog and epilog are special procedures
executed before and after each procedure call.

```c++
  // Define a prologe.
  s.prolog = [] () { std::cout << "prolog" << std::endl; };

  // Define an epilog.
  s.epilog = [] () { std::cout << "epilog" << std::endl; };
```
