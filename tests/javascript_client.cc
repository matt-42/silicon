#include <iostream>
#include <silicon/api.hh>
#include <silicon/backends/mimosa_serve.hh>
#include <silicon/clients/javascript_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

int main(int argc, char* argv[])
{

  std::string js_client;
  
  auto hello_api = make_api(

    _test = [] () { return D(_message = "hello world."); },
    _test2 = [] () { return D(_message = "hello world."); },
    _my_scope = D(_test2(_name) = [] (auto p) {
      return D(_message = "hello " + p.name);
      }),

    _js_client = [&] () { return js_client; }

    );

  js_client = generate_javascript_client(hello_api);

  if (argc == 2)
    sl::mimosa_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
  
}
