#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hello_api = http_api(
  GET / _test = [] () { return D(_message = "hello world."); }
);

int main(int argc, char* argv[])
{
  auto ctx = sl::mhd_json_serve(hello_api, 12345, _non_blocking);

  auto c = libcurl_json_client(hello_api, "127.0.0.1", 12345);
  auto r1 = c.http_get.test();
  std::cout << iod::json_encode(r1) << std::endl;
}
