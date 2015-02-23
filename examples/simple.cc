#include <iostream>
#include <fstream>

#include <silicon/api.hh>
#include <silicon/backends/mhd.hh>

#include "symbols.hh"

int main()
{

  using namespace sl;

  // Create an api
  auto api = make_api(

    _hello(_name) = [] (auto p) { return D(_message = "hello " + p.name); }

    );

  // Serve it using cppnetlib and the json protocol.
  mhd_json_serve(api, 9999);
}
