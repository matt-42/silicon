#include <thread>
#include <iostream>
//#include <silicon/backends/mhd.hh>
#include <silicon/backends/lwan.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include <silicon/middlewares/hashmap_session.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

struct session
{
  int id;
};

int main()
{
  auto api = http_api(

    GET / _set_id / _id[int()] = [] (auto p, session& s)
    {
      s.id = p.id;
      return D(_id = s.id);
    },

    GET / _get_id = [] (session& s) {
      return D(_id = s.id);
    }
    
    );

  // Start server.
  auto ctx = lwan_json_serve(api, std::make_tuple(hashmap_session_factory<session>()), 12345, _non_blocking);

  // Test.
  {
    auto c1 = libcurl_json_client(api, "127.0.0.1", 12345); 
    auto c2 = libcurl_json_client(api, "127.0.0.1", 12345);
 
    auto r1 = c1.http_get.set_id(_id = 42);
    auto r2 = c2.http_get.set_id(_id = 51);

    auto r3 = c1.http_get.get_id();
    auto r4 = c2.http_get.get_id();

    std::cout << r3.response.id << std::endl;
    std::cout << r4.response.id << std::endl;

    assert(r3.response.id == 42);
    assert(r4.response.id == 51);
  }
}
