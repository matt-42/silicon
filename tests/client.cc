#include <thread>
#include <silicon/client.hh>
#include <silicon/server.hh>

int main()
{
  using namespace iod;
  
  auto server = silicon.api(
    
    @hello_world(@name) = [] (auto p)
    {
      return D(@message = std::string("hello ") + p.name);
    }

  );

  std::thread t([&] () { server.serve(12345); });

  auto c = silicon_client(server.get_api(), "0.0.0.0", 12345);
  auto r = c.hello_world("John");
  
  std::cout << "Status: " << r.status << std::endl;
  std::cout << "Message: " << r.response.message << std::endl;

}
