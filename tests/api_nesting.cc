#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hello_api = make_api(

  _test2 = [] () { return D(_message = "hello world."); },

  _test = std::make_tuple(
    PUT / _test2 = [] () { return D(_message = "hello world from the nested api."); },

    _test3 = std::make_tuple(
      GET / _test4 = [] () { return D(_message = "hello world from the nested nested api."); }
      )

    )
);

int main(int argc, char* argv[])
{
  std::thread t([&] () { mimosa_json_serve(hello_api, 12345); });
  usleep(.1e6);

  if (argc == 2)
    sl::mhd_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
