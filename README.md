
The Silicon Web Framework
=================================


Silicon is a easy to use, fast, safe, modern C++ json remote procedure call framework
based on the C++14 standard. It relies the powerful new
features of C++ and imitate the flexibility of other languages like
Ruby, Python, Go or Javascript using meta programming.

Features
=========================

  - Ease of use:

    The framework is designed so that the user only writes the body of
    the procedures, the framework take care of the rest
    (serialization, deserialization, generation of client libraries, routing, multithreading, ...)

  - Fast:
    
    C++ has no overhead such as other web programming languages: No interpreter,
    no garbage collection, no virtual machine, and no just in time compiler.
    C++ multithreaded http server with the mimosa framework.

  - Compiled:

    Using a compiled language allows to validate the validity of the
    server code before deploying it. Furthermore, json objects are
    deserialized in plain, type safe, C++ object (not hash tables),
    disallowing access to undefine members.

  - Automatic validation, serialization and deserialization of json messages:

    Thanks to static introspection and meta programming, a pair json
    parser and encoder is generated for each message type. No hash
    table and other dynamic structures as in all other json
    frameworks. Since the json parser knows the structure of each
    message, it throw exeption when a required field is missing, or
    when the type of a value is incorect.

  - Automatic generation of the client libraries:

    Again, thanks to static introspection, the framework generates at
    compile time the client libraries. As of today, only javascript is
    supported.

  - Dependency Injection and Middlewares:

    Procedures only takes the middleware they need as argument.

Getting Started
=========================

The most basic procedure is an object to write in the http response body:

```c++
namespace sl = silicon
int main(int argc, char* argv[])
{
  sl::server s(argc, argv);

  // Create the Hello world procedure.
  s["hello_world"] = "Hello world!";
  // ^^^^^^^^^^^ -> This will be the name of the procedure in the client libraries
  //                (see below the javascript example).

  // Generate the javascript client libraries and serve it on a specific route.
  s["/bindings.js"] = generate_javascript_bindings(s, sl::module = "hello_world_api");
 
  s.serve();
}
```

The javascript client would be:

```javascript
<script type="text/javascript" src="http://my_server_path/bindings.js" />

<script>
        hello_world_api.hello_world().then(function(r){ document.write(r); });
</script>
```

Procedures
========================

Silicon procedures can have different types:

```c++

  // A lambda function returning a string.
  s["lambda1"] = [] () { return "Hello from lambda"; };

  // Or an staticaly introspectable object.
  // The serialization to json is automatic.
  s["lambda2"] = [] () { return D(_Name = "Paul", _City = "Paris"); };

  // A procedure taking parameters.
  // The type of the parameter must be specified with the function D.
  s["lambda3"] = [] (decltype(D(_Name = std::string())) params)
  { 
    return "Hello" + params.name; 
  };

  // A function taking the response object, for example to set the response headers.
  s["lambda2"] = [] (response& response) {
     response << iod::D(_Message = "Hello World");
  };

  // Procedures also take middleware as argument.
  s["session"] = [] (my_session_middleware session) {
     return iod::D(_Message = "Hello " + session.username);
  };

```

A procedure can also be a classic C++ function:

```c++
  auto my_function() { return D(_Message = "Hello world"); }

  [...]

  // A session middleware
  s["classic_function"] = my_function;
```


DI and Middlewares
=========================

At compile time, Silicon analyses the arguments of the procedures and
generates via meta programmation the argument list
construction. Hence, there is not restriction on the arguments order
and procedures instantiate only the middleware they need. For example,
even if a user own a session, not all the procedures actually need to
read data from this session. Some cpu cycles are then saved if
only the minimum set of procedures instantiate the session.


```c++
  // Instantiates a session middleware
  s["session"] = [] (my_session_middleware& session) {
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
