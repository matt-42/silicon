#include <iostream>
#include <silicon/api.hh>
#include <silicon/backends/mimosa.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hello_api = make_api(

  _test = [] () { return D(_message = "hello world."); },
  _test2(_name) = [] (auto p) {
    return D(_message = "hello " + p.name);
  }

);

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mimosa_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
