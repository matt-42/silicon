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
  auto api = make_api(

    _my_tracking_id = [] (tracking_cookie c) {
      return D(_id = c.id());
    }
    
    );

  // Start server.
  auto ctx = mhd_json_serve(api, 12345);

  // Test.
  {
    auto c = libcurl_json_client(api, "127.0.0.1", 12345);
  
    auto r1 = c.my_tracking_id();
    auto r2 = c.my_tracking_id();
    auto r3 = c.my_tracking_id();

    std::cout << r1.response.id << std::endl;
    std::cout << r2.response.id << std::endl;
    std::cout << r3.response.id << std::endl;

    assert(r1.response.id == r2.response.id);
    assert(r1.response.id == r3.response.id);
  }
  
}
