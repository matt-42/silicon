---
layout: post
title: symbols
---


Symbols and  Statically Introspectable Objects (SIOs)
=============================

The Silicon framework design is derived from the static
metaprogramming paradigm of the [IOD
framework](https://github.com/matt-42/iod).  The core of this
framework relies on the symbol concept: A set of C++ variable
bearing the static introspection of the library. With symbols,
declaring a staticaly introspectable object (sio) is one C++
statement:

```c++
auto o = D(_name = "John", _age = 42);
// o.name == "John"
// o.age == 42
```

Here, the symbols _name and _age hold the following semantics:
The resulting object will have two members o.name and o.age. The
first is a string representing "John" and the second is an int
representing 42.

_name and _age are plain C++ variables and the previous code will not
compile without their definitions:

```c++
// Guards avoid multiple definitions of symbols.
#ifndef IOD_SYMBOL_NAME
#define IOD_SYMBOL_NAME
iod_define_symbol(name);
#endif

#ifndef IOD_SYMBOL_AGE
#define IOD_SYMBOL_AGE
iod_define_symbol(age);
#endif
```

Furthermore, to avoid name conflicts, iod places all the symbols in
the namespace s. Then, using namespace s allows to skip the s:: prefix.


## Introspection

The big advantage of SIOs over plain C++ object is their static
introspection. The IOD foreach construct allows to apply a function to
each member of an object. A member gives access to its associated
value and symbol.

For example, one can easily build a basic serializer:

```c++
// Define an object
auto o = D(_name = "John", _age = 42, _city = "NYC");

// Output the structure of the object to std::cout:
// name:John
// age:42
// city:NYC
foreach(o) | [] (auto& m) { std::cout << m.symbol().name() << ":"
                                      << m.value() << std::end; }

```

## Automatic declaration of symbols with the @ syntaxic sugar [optional]

Silicon makes heavy use of iod symbols. To encounter the burden of
predeclaring every symbols and prefixing them with s::, IOD provides a
small compiler that implements the following syntaxic sugar:


```c++
#include <iod/symbol.hh>

int main()
{
  @test;
}
```

is equivalent to the following thanks to the iodc compiler:

```c++
#include <iod/symbol.hh>

#ifndef IOD_SYMBOL_TEST
#define IOD_SYMBOL_TEST
iod_define_symbol(test);
#endif


int main()
{
  s::_test;
}
```

The cmake add_iod_executable automate the compilation of programs using
the @ construct:

```cmake
// A CMakeLists.txt to compile a silicon server with the microhttpd backend.
cmake_minimum_required(VERSION 2.8)

find_package(Iod REQUIRED)
find_package(Silicon REQUIRED)

include(${IOD_USE_FILE})

include_directories(${IOD_INCLUDE_DIR} ${SILICON_INCLUDE_DIR})

add_iod_executable(hello_world main.cc)
target_link_libraries(hello_world microhttpd)
```
