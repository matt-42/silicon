#include <silicon/middlewares/hashmap_session.hh>
#include <thread>
#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"
#include "http_backend_testsuite.hh"

int main(int argc, char* argv[])
{
  auto ctx = mhd_json_serve(hl_api, std::make_tuple(hashmap_session_factory<session>()), 12345, _non_blocking);

  backend_testsuite(12345);
  exit(0);  
}
