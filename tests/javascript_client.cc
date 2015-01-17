#include <iostream>
#include <silicon/api.hh>
#include <silicon/javascript_client.hh>

using namespace sl;

int main()
{

  std::string js_client;
  
  auto hello_api = make_api(

    @test = [] () { return D(@message = "hello world."); },
    @test2 = [] () { return D(@message = "hello world."); },
    @my_scope = D(@test2(@name) = [] (auto p) {
      return D(@message = "hello " + p.name);
      }),

    @js_client = [&] () { return js_client; }

    );

  js_client = generate_javascript_client(hello_api);

  //std::cout << generate_javascript_client(hello_api) << std::endl;
}
