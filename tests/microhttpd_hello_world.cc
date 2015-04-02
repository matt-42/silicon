#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hello_api = make_api(

  _test = [] () { return D(_message = "hello world."); },
  _test2(_name) = [] (const auto& p) { return D(_message = "hello " + p.name); },
  _test3 = [] () { return "hello world."; },
  _test4 = [] () { return response(_content_type = "text/html", _body = D(_message = "hello world.")); }
  
);

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mhd_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
