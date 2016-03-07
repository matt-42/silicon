---
layout: blog
title: A C++ Real-Time Broadcast App with Silicon and Websockets
date: 2015-04-01
---

# A C++ Real-Time Broadcast App with Silicon and Websockets

In the previous post, I presented [how to implement and serve a simple
blog API through HTTP](http://siliconframework.org/blog/a_simple_silicon_blog_api.html).
This protocol is well suited only for traditional
applications where all the communication are initiated by the
client. To overcome this limitation, [the Websocket
protocol](http://en.wikipedia.org/wiki/WebSocket) has been recently
standardized and [implemented in all the major
web browsers](http://caniuse.com/#feat=websockets). It defines
bidirectional communication between the client and the server. In
other words, a websocket server is able to initialize a communication to
push messages to the clients. This allows implementations of real-time
applications where the clients need to receive notifications as soon as
there are available.

I will show in this blog post how to leverage the Silicon web
framework to implement one of them: a real-time broadcast chat. It
relies on the Websocket protocol to automatically forward messages to
all the connected users.

The complete source code of this tutorial is hosted on [the github repository]
(https://github.com/matt-42/silicon/blob/master/examples/ws_broadcast_server.cc).

Despite HTTP and Websockets being two different modes of
communication, the way we use both in Silicon share the same core
concepts: [APIs](http://siliconframework.org/docs/apis.html) and
[middlewares](http://siliconframework.org/docs/middlewares.html), though
there are few novelties:

   - A new Silicon backend based on a websocket C++ implementation: [Websocketpp](https://github.com/zaphoyd/websocketpp)
   - A caller mapping the functions available on the remote client.
   - The on open / on close handlers to maintain the list of connected clients
   - The http fallback API to serve static content to a user who has not
     yet initiated a websocket connection


Let's first define the set of connected users:

```c++
std::set<wspp_connection> users;
```

This set will be updated at every creation and destruction of a
websocket connection.  Because Silicon spreads the processing of these
events in concurrent threads, we need a mutex to avoid race conditions:

```c++
std::mutex users_mutex;
```

As I said earlier, Websockets allows us to push messages to the remote
client.  With Silicon, these messages are seen as function calls and
the ```make_wspp_remote_client``` allows us to easily declare the
signatures of these remote functions.

The bodies of these functions are available client side and executed
as soon as a message transits from the server to the client. Here we
only have to define one remote function: the function ```message```
taking as argument the new message to display.

```c++
auto rclient = make_wspp_remote_client * parameters( _message(_text) );
```


The ```rclient``` exposes the remote functions of a given client
connection ```c``` as if they were plain C++ functions:
```rclient(c).message("Hello John!")``` tells (via the websocket) the
client of connection ```c``` to call the function ```message``` with
the argument ```"Hello John!"```. We will see later in this post how the
remote javascript client decode this message.

The way the rclient is build may seem a little magic but it is
actually plain C++. \_message and \_text are IOD symbols declared in
a header with the ```iod_define_symbol``` construct not included in this tutorial.
You can read more about this paradigm [here](http://siliconframework.org/docs/symbols.html)
and [here](https://github.com/matt-42/iod/blob/master/README.md).


We now have everything to define the server API. It contains one
broadcast procedure that each client will call as soon as a user
enters a message. The role of this procedure is to broadcast the message
to all clients contained in the ```users``` set via the ```rclient``` helper.

```c++
auto server_api = ws_api(

    _broadcast * parameters(_message) = [&] (auto p) {
      for (wspp_connection& c : users) rclient(c).message(p.message);
    }
);
```

The function ```broadcast(message)``` will be exposed to the remote client via the
javascript class ```silicon_json_websocket``` generated server side:

```c++
std::string js_client = generate_javascript_websocket_client(server_api);
```

After loading this javascript class, the client is be able to open
the websocket and call the broadcast with two simple lines of
javascript:

```javascript
var ws = new silicon_json_websocket('ws://' + window.location.host);
ws.broadcast({message: "Hello remote server, please broadcast my message to all the connected users." });
```

We are now able to launch the websocket server. However we still have
to pass it three options: First, via the ```_on_open``` option, we set the
handler registering every new clients. Then, we pass to the
```_on_close``` option the function removing disconnected clients. And
finally, we serve the generated js client and html page via a fallback
HTTP api (via the ```_http_api``` options).

```c++

std::string index_html_source = // See the full html source on github.

wspp_json_serve(server_api, 8080,
 
                _on_open = [&] (wspp_connection& c) {
                                  std::lock_guard<std::mutex> lock(users_mutex);
                                  users.insert(c);
                           },
                  
                _on_close = [&] (wspp_connection& c) {
                                  std::lock_guard<std::mutex> lock(users_mutex);
                                  users.erase(c);
                           },
                  
                _http_api = http_api(
                  GET / _js_client = [&] () { return js_client; },
                  GET / _home = [&] () { return index_html_source; }
                )
);
```

For readability, I did not include the full html source. The only
interesting part is the inline javascript displaying new broadcasts
and broadcasting new messages via the server. As I explained before, the class
```silicon_json_websocket``` generated via
```generate_javascript_websocket_client(server_api)``` manages the
serialization / deserialization of websocket messages.

```html
<script src="/js_client" type="text/javascript"></script>

<script>
  var messages_div = document.getElementById('content');
  var input = document.getElementById('message_input');
  var ws = new silicon_json_websocket('ws://' + window.location.host);

  ws.api.message = function (m) { 
    var n = document.createElement('div');
    n.appendChild(document.createTextNode(m.text));
    messages_div.appendChild(n);
    messages_div.scrollTop = messages_div.scrollHeight;
  }

  document.getElementById('form').onsubmit = function (e) {
    if (input.value.length > 0) ws.broadcast({message: input.value });
    input.value = "";
    return false;
  };

</script>
```

We finally have a full stack C++/javascript broadcast. Both the C++
server and javascript client are about 15 lines each (plus the
HTML page layout). The C++ part is easy to read since it does not ask
the user to implement heavy C++ meta programming. It is also safe and
not error prone: no explicit use of pointer, dynamic memory
management, dynamic inheritance or virtual classes. In short, most of
the errors will be easily detected and reported by the compiler.

There have been many myths about C++ being low level / rigid / verbose
and not suitable for web programming. I have been writing C++ for 10
years and my point of view is that it was true with C++98 and
C++11. However, while being very close to C++11, C++14 enables us to
write amazing abstractions without impacting the performances of the
language (see the [IOD library](https://github.com/matt-42/iod) for
more details about the abstractions used by Silicon). As I showed in
this post, productive high performance
web programming is now possible in C++: The Silicon framework
leverages the last standard to help you write web apps almost as quickly as
you would develop in other dynamic languages, but with the performance
of the equivalent written in plain C. The other good news is the large
number of open source C/C++ libraries available: Almost every databases
already have a C driver and C networking libraries already implements
the vast majority of the web communication protocols. Silicon aims
to wrap them together in a flexible and easy to use framework.

The only downside of this approach is compilation time. This simple
example takes approximately 14 seconds to compile on a desktop
computer. It is due to the meta programming code providing the
flexibility and the safety of Silicon. However, the upcoming
optimizations in the framework and in the C++ compilers are likely to
speedup compilation.
