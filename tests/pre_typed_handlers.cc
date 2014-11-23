#include <iostream>
#include <silicon/mimosa_backend.hh>
#include <silicon/javascript_client.hh>
#include <silicon/server.hh>
#include <silicon/sqlite.hh>

#include "symbols.hh"

int main()
{
  using namespace iod;
  using namespace s;
  auto server = silicon();
  
  server["h0"](_Id = int()) = [] (auto params)
  {
    return D(_Id = params.id);
  };

  server["h0"](_Id, _Name) = [] (auto params)
  {
    static_assert(std::is_same<std::string, decltype(params.name)>::value, "error");
  };
  
  server.serve(8888);
}
