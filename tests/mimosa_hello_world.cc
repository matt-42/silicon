#include <iostream>
#include <silicon/api.hh>
#include <silicon/backends/mimosa_serve.hh>

using namespace sl;

auto hello_api = make_api(

  @test = [] () { return D(@message = "hello world."); },
  @test2(@name) = [] (auto p) {
    return D(@message = "hello " + p.name);
  }

);

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mimosa_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
