#include <silicon/middlewares/hashmap_session.hh>
#include <thread>
#include <iostream>
#include <silicon/backends/lwan.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hl_api = http_api(
  GET / _test = [] () { return D(_message = "hello world."); }
);

int main(int argc, char* argv[])
{
  auto ctx = lwan_json_serve(hl_api, 12345, _non_blocking);

  auto c = libcurl_json_client(hl_api, "127.0.0.1", 12345);
  auto r1 = c.http_get.test();

  assert(r1.message == "hello world.");
  std::cout << iod::json_encode(r1) << std::endl;
}
