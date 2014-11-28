#include <iostream>
#include <fstream>

#include <silicon/mimosa_backend.hh>
#include <silicon/server.hh>

using namespace s;

int main()
{
  using namespace iod;

  auto server = silicon();
  server["/"]= "simple";
  server.serve(9999);
}
