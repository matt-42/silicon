#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hello_api = http_api(
  GET / _text = [] () { return D(_message = "hello world."); }
);

int main(int argc, char* argv[])
{
  auto ctx = sl::mhd_json_serve(hello_api, 12345, _blocking,
                                _https_cert = "./server.pem",
                                _https_key = "./server.key");
}
