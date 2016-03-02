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

    GET / _my_tracking_id = [] (tracking_cookie c) {
      return D(_id = c.id());
    }
    
    );

  // Start server.
  auto ctx = mhd_json_serve(api, 12345, _non_blocking);

  // Test.
  {
    auto c1 = libcurl_json_client(api, "127.0.0.1", 12345);
    auto c2 = libcurl_json_client(api, "127.0.0.1", 12345);
  
    auto r11 = c1.http_get.my_tracking_id();
    auto r21 = c2.http_get.my_tracking_id();
    auto r12 = c1.http_get.my_tracking_id();
    auto r22 = c2.http_get.my_tracking_id();
    auto r13 = c1.http_get.my_tracking_id();
    auto r23 = c2.http_get.my_tracking_id();

    std::cout << r11.response.id << std::endl;
    std::cout << r12.response.id << std::endl;
    std::cout << r13.response.id << std::endl;

    std::cout << r21.response.id << std::endl;
    std::cout << r22.response.id << std::endl;
    std::cout << r23.response.id << std::endl;
    
    assert(r11.response.id == r12.response.id);
    assert(r11.response.id == r13.response.id);

    assert(r21.response.id == r22.response.id);
    assert(r21.response.id == r23.response.id);

    assert(r21.response.id != r11.response.id);
  }
  
}
