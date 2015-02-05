#include <thread>
#include <iostream>
#include <silicon/backends/mimosa_serve.hh>
#include <silicon/api.hh>
#include <silicon/clients/client.hh>

using namespace sl;

int main()
{
  auto api = make_api(

    @my_tracking_id = [] (tracking_cookie c) {
      return D(@id = c.id());
    }
    
    );

  // Start server.
  std::thread t([&] () { mimosa_json_serve(api, 12345); });
  usleep(.1e6);

  // Test.
  auto c = json_client(api, "127.0.0.1", 12345);

  auto r1 = c.my_tracking_id();
  auto r2 = c.my_tracking_id();

  std::cout << r1.response.id << std::endl;
  std::cout << r2.response.id << std::endl;

  assert(r1.response.id == r2.response.id);

  exit(0);
}
