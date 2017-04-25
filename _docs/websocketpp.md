---
layout: documentation
---

Websocketpp backend
================================


Websocketpp is a C++ implementation of the websocket
protocol. Websockets allow bidirectional communications between the
server and the client, in opposition to HTTP where all communications
are initiated by the client.
Please note that the source code Websocketpp is not provided by silicon. You
need to install it by yourself before compiling silicon-websocketpp code.

The websocketpp silicon backend enables the server to remotely call function on
the set of client connected clients. It implements a very basic
protocol to serialize and send the calling data (function path and
arguments) back to the client.

Silicon provides an implementation of the javascript client that can
be automatically generated and served on a classic HTTP route.

The example [ws_broadcast_server.cc](https://github.com/matt-42/silicon/blob/master/examples/ws_broadcast_server.cc)
shows how to build a simple broadcast chat with silicon and it's websocketpp backend.
