#include <iostream>
#include <silicon/api.hh>
#include <silicon/backends/mhd.hh>
#include <silicon/clients/javascript_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
    return 0;
  }

  std::string js_client;

  std::string homepage = R"html(
<script src="/js_client"></script>
<script>
sl.http_get.test().then(function (e) { console.log(e); });
sl.http_post.my_scope.test2({name: "John", test: "Paris", id: 32}).then(function (e) { console.log(e); });
</script>
)html";
  auto hello_api = http_api(

    GET / _test = [] () { return D(_message = "hello world."); },
    GET / _test2 = [] () { return D(_message = "hello world."); },
    _my_scope = http_api(
      POST / _test2 / _name[std::string()] * post_parameters(_test = optional(std::string("432"))) * get_parameters(_id = std::string())= [] (auto p) {
        return D(_message = "hello " + p.name + p.test + p.id);
      }),

    GET / _js_client = [&] () { return js_client; },
    GET  = [&] { return homepage; }

    );

  js_client = generate_javascript_client(hello_api);

  std::cout << js_client << std::endl;
  
  auto ctx = sl::mhd_json_serve(hello_api, atoi(argv[1]), _blocking);

  
}
