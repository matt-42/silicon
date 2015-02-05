#include <iostream>
#include <silicon/backends/mhd_serve.hh>
#include <silicon/api.hh>

using namespace sl;

auto hello_api = make_api(

  @test = [] () { return D(@message = "hello world."); },
  @test2(@name) = [] (const auto& p) { return D(@message = "hello " + p.name); },
  @test3 = [] () { return "hello world."; }

);

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mhd_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
