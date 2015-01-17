#include <iostream>
#include <silicon/mimosa_serve.hh>
#include <silicon/api.hh>

auto api = sl::make_api(

  @test = [] () { return D(@message = "hello world."); },
  @test2(@name) = [] (const auto& p) { return D(@message = "hello " + p.name); }

);

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mimosa_json_serve(api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
