#include <iostream>
#include <fstream>

#include <silicon/api.hh>
#include <silicon/microhttpd_serve.hh>

int main()
{

  using namespace sl;

  // Create an api
  auto api = make_api(

    @hello(@name) = [] (auto p) { return D(@message = "hello " + p.name); }

    );

  // Serve it using cppnetlib and the json protocol.
  microhttpd_json_serve(api, 9999);
}
