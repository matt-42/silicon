
#include <sys/time.h>
#include <sys/resource.h>

#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hello_api = http_api
  (
   GET / _test = [] (mhd_request* r, prefix_path p) {
     return p.string();
   }
   );

int main(int argc, char* argv[])
{      
  auto ctx = sl::mhd_json_serve(hello_api, 12345, _non_blocking);

  auto c = libcurl_json_client(hello_api, "localhost", 12345);

  assert(c.http_get.test().response == "/test");
}
