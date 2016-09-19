#include <cassert>
#include <vector>
#include <iostream>
#include <silicon/procedure_desc.hh>
#include "symbols.hh"
int main()
{
  using namespace s;
  using namespace sl;

  {
    typedef decltype(D(_test1 = int(), _test2 = float())) T;

    T o1;
    std::vector<T> o2;

    assert(type_string(&o1) == "{test1: int, test2: float}");
    assert(type_string(&o2) == "vector of {test1: int, test2: float}");
  }

  {
    typedef decltype(D(_test1 = {D(_test2 = int())})) T;

    T o1;
    std::vector<T> o2;

    assert(type_string(&o1) == "{test1: vector of {test2: int}}");
    assert(type_string(&o2) == "vector of {test1: vector of {test2: int}}");
  }
  // std::cout << type_string(&o1) << std::endl;
  // std::cout << type_string(&o2) << std::endl;
  //assert(type_string(0a)

  return 0;
}
