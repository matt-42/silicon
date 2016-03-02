#include <iostream>
#include <silicon/api.hh>
#include <silicon/backends/mhd.hh>
#include <silicon/clients/javascript_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
    return 0;
  }

  std::string js_client;
  
  auto hello_api = http_api(

    GET / _test = [] () { return D(_message = "hello world."); },
    GET / _test2 = [] () { return D(_message = "hello world."); },
    GET / _my_scope = http_api(
      _test2 / _name[std::string()] = [] (auto p) {
        return D(_message = "hello " + p.name);
      }),

    GET / _js_client = [&] () { return js_client; }

    );

  js_client = generate_javascript_client(hello_api);

  auto ctx = sl::mhd_json_serve(hello_api, atoi(argv[1]), _non_blocking);

  std::cout << js_client << std::endl;
  
}
