#include <iostream>
#include <silicon/backends/cppnet_serve.hh>
#include <silicon/api.hh>
#include "symbols.hh"

using namespace sl;
using namespace s;

auto hello_api = make_api(

  _test = [] () { return D(_message = "hello world."); },
  _test2(_name) = [] (const auto& p) { return D(_message = "hello " + p.name); }

);

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::cppnet_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
