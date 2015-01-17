#include <thread>
#include <silicon/client.hh>
#include <silicon/api.hh>
#include <silicon/api_description.hh>
#include <silicon/microhttpd_serve.hh>
#include <silicon/mimosa_serve.hh>


int main()
  try
  {
    using namespace sl;

    auto api = make_api(
      @hello_world(@name) = [] (auto p)
      {
        return D(@message = std::string("Hello ") + p.name);
      },

      @scope = D(

        @test(@name) = [] (auto p)
        {
          return D(@message = std::string("Hello ") + p.name);
        }
        
        )
      );
 
    std::cout << api_description(api) << std::endl;

    std::thread t([&] () { mimosa_json_serve(api, 12345); });

    // Wait for the server to start.
    usleep(.1e6);

    auto c = json_client(api, "127.0.0.1", 12345);

    auto r1 = c.hello_world("John");
    assert(r1.response.message == "Hello John");
    assert(r1.status == 200);
    
    std::cout << c.hello_world("John").response.message << std::endl;
    std::cout << c.scope.test("John").response.message << std::endl;

    exit(0);
  }
  catch (std::runtime_error e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
