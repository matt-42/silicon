#include <thread>
#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

int main()
{
  auto api = http_api(
      GET / _my_files = file_server("./files")
  );

  auto ctx = mhd_json_serve(api, 12345, _blocking);

}
