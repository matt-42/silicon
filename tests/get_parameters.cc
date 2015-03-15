#include <thread>
#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/middlewares/get_parameters.hh>
#include <silicon/api.hh>
#include <silicon/clients/client.hh>
#include "symbols.hh"

using namespace s;
using namespace sl;

int main()
{
  auto api = make_api(

    _test = [] (get_parameters& params) {
      return D(_id = params["id"]);
    }
    
    );

  // Start server.
  mhd_json_serve(api, 12345);
}
