#include <iostream>
#include <fstream>

#include <silicon/mimosa_backend.hh>
#include <silicon/server.hh>

auto fun()
{
  return std::string("hello world");
}

int main()
{
  using namespace iod;

  auto server = silicon.api(
    @hello = [] () { return "hello world"; },
    @hello2 = &fun
    );
  //server["/"]= "simple";
  server.serve(9999);
}
