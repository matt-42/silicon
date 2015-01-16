#include <iostream>
#include <silicon/mimosa_serve.hh>
#include <silicon/api.hh>

auto api = sl::make_api(

  @test = [] () { return D(@message = "hello world."); },
  @test2(@name) = [] (const auto& p) { return iod::D(@message = std::string("hello " + p.name)); }

);

int main(int argc, char* argv[])
{
  sl::mimosa_json_serve(api, atoi(argv[1]));
}
