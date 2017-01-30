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
      GET / _my_files / _my_file[std::string()] = file_server("./files")
  );

  // Start server.
  auto ctx = mhd_json_serve(api, 12345, _non_blocking);

  // Test.
  {
    auto c1 = libcurl_json_client(api, "127.0.0.1", 12345);

    auto r11 = c1.http_get.my_files(_my_file = std::string("hello.txt"));

//    r11.response.status;
//    std::cout << r11.response.id << std::endl;
//    assert(r11.response.id == "huh");
  }

}