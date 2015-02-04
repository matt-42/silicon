---
layout: post
---

Websocketpp backend
================================


Websocketpp is a C++ implementation of the websocket
protocol. Websockets allow bidirectional communication between the
server and the client, in opposition to HTTP where all communications
are initiated by the client.

The websocketpp backend enable the server to remotely call function on
the set of client connected clients. It implements a very basic
protocol to serialize and send the calling data (function path and
arguments) back to the client.

Silicon provides an implementation of the javascript client that can
be automatically generated and served on a classic HTTP route.


## Implementation of a simple broadcast app

