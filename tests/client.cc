#include <thread>
#include <silicon/client.hh>
#include <silicon/server.hh>

auto proc1(decltype(D(@name = std::string())) params)
{
  return D(@test = params.name);
}

auto fun()
{
  return D(@message = std::string("fun: hello world"));
}

int main()
{
  using namespace iod;

  auto server = silicon.api(
    @hello_world(@name) = [] (auto p)
    {
      return D(@message = std::string("hello ") + p.name);
    }
    ,
    @test = &fun,
    @test2 = &proc1,
    @namespace1 = D(@test_in_nsp = &proc1),
    @testX = [] () {}
  );
 
  print_server_api(server);
 
  std::thread t([&] () { server.serve(12345); });

  auto c = silicon_client(server.get_api(), "0.0.0.0", 12345);

  usleep(.1e6);

  std::cout << c.test().response.message << std::endl;
  std::cout << c.test2("John").response.test << std::endl;
  std::cout << c.namespace1.test_in_nsp("Doe").response.test << std::endl;
}
