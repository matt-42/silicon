#include <thread>
#include <silicon/clients/libcurl_client.hh>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/api_description.hh>
#include "symbols.hh"


int main()
  try
  {
    using namespace sl;
    using namespace s;

    auto api = http_api(
      POST / _hello_world / _age[int()] * get_parameters(_name) * post_parameters(_city) = [] (auto p)
      {
        std::stringstream ss;
        ss << "Hello " << p.name << " " << p.age << " " << p.city;
        return D(_message = ss.str());
      },

      _scope = http_api(

        POST / _test2 / _age[int()] / _city[std::string()] * get_parameters(_name) = [] (auto p)
        {
          std::stringstream ss;
          ss << "test2: Hello " << p.name << " " << p.age << " " << p.city;
          return D(_message = ss.str());
        }
        
        )
      );
 
    std::cout << api_description(api) << std::endl;

    auto server = mhd_json_serve(api, 12345, _non_blocking);

    auto c = libcurl_json_client(api, "127.0.0.1", 12345);

    auto r1 = c.http_post.hello_world(_name = "John==&", _age = 32, _city = "Paris");
    assert(r1.response.message == "Hello John==& 32 Paris");
    assert(r1.status == 200);
    
    auto r2 = c.http_post.scope.test2(_name = "John", _age = 42, _city = "Nantes");
    assert(r2.response.message == "test2: Hello John 42 Nantes");

    exit(0);
  }
  catch (std::runtime_error e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
