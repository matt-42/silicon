
The Silicon Web Framework
=================================


Silicon is a easy to use, fast, safe, modern C++ remote procedure call framework
based on the C++14 standard. It relies the powerful new
features of C++ and imitate the flexibility of other languages like
Ruby, Python, Go or Javascript.

Features
=========================

  - Ease of use:

    The framework is designed so that the user only write the body of
    the procedures, the framework take care of the rest
    (serialization, deserialization, generation of client libraries, routing, ...)

  - Fast:
    
    Zero overhead (dependency injection is done at compile time).
    C++ multithreaded http server.

  - Safe (almost):

    At least safer than any interpreted language. The use of static
    C++ meta programming helps the compiler to verify the consistency
    of the code.

  - Automatic serialization and deserialization of the messages:

    Thanks to static introspection and meta programming, a json parser
    and encoder is generated for each message. No hash table and other
    dynamic structures as in all other json frameworks.

  - Automatic generation of the client libraries:

    Again, thanks to static introspection, the framework generates at
    compile time the client libraries. As of today, only javascript is
    supported.

  - Dependency Injection and Middlewares:

    Procedures take only the middleware they need as argument.


Getting Started
=========================

The most basic procedure an object to write in the http response body:

```c++
namespace sl = silicon
int main(int argc, char* argv[])
{
  sl::server s(argc, argv);

  // Create the Hello world procedure.
  s["hello_world"] = std::string("Hello world!");
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

  // A lambda function returning the string to send.
  s["lambda1"] = [] () { return "Hello from lambda"; };

  // Or an staticaly introspectable object.
  // The serialization to json is automatic.
  s["lambda2"] = [] () { return D(_Name = "Paul", _City = "Paris"); };

  // A procedure taking parameters.
  // The type of the parameter must be specified with iod::D.
  s["lambda3"] = [] (decltype(D(_Name = std::string())) params)
  { 
    return "Hello" + params.name; 
  };

  // A function taking the response object, for example to set the response headers.
  s["lambda2"] = [] (response& response) {
     response << iod::D(_Message = "Hello World");
  };

  // A session middleware
  s["session"] = [] (my_session_middleware& session) {
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
construction. Hence, there is not restriction on the arguments order,
and procedures instantiate only the middleware they need. For example,
even if a user own a session, not all the procedures actually need to
read data from this user session. Some cpu cycles are then saved if
only the minimum set of procedures instantiate the session.


```c++
  // Instantiate a session middleware
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
