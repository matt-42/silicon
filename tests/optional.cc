#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hello_api = http_api(

  
  GET / _test * get_parameters(_id = optional(12)) = [] (auto p) { return D(_id = p.id ); },
  POST / _test * post_parameters(_id = optional(22)) = [] (auto p) { return D(_id = p.id ); },
  POST / _test2 * post_parameters(_id = optional("test")) = [] (auto p) { return D(_id = p.id ); }
);

int main(int argc, char* argv[])
{
  auto server = sl::mhd_json_serve(hello_api, 12345, _non_blocking);

  auto c = libcurl_json_client(hello_api, "127.0.0.1", 12345);

  auto r1 = c.http_get.test();
  assert(r1.response.id == 12);

  auto r2 = c.http_get.test(_id = 42);
  assert(r2.response.id == 42);
  
  auto r3 = c.http_post.test();
  std::cout << json_encode(r3) << std::endl;
  assert(r3.response.id == 22);

  auto r4 = c.http_post.test(_id = 42);
  assert(r4.response.id == 42);

  auto r5 = c.http_post.test2();
  assert(r5.response.id == "test");

  auto r6 = c.http_post.test2(_id = "toto");
  assert(r6.response.id == "toto");  
}
