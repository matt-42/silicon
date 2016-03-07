---
layout: documentation
title: Getting Started
---

Getting Started
=========================

To get started with silicon you need **the compiler Clang++** on your system (g++ fails
at compiling silicon and I do not have access to other compilers).

###Installation

The following will install Silicon and the [Iod](https://github.com/matt-42/iod) library.

```
git clone https://github.com/matt-42/silicon.git
cd silicon;
./install.sh __YOUR__INSTALL__PREFIX__
```

Since Silicon in only an abstraction and does not implement a HTTP
server, you will need to install
[libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) if it is
not already on your system.


### Next Step

You can now have a look at [the hello world example](/docs/hello_world.html) and discover how to
build and start a simple web server with silicon.
