#include <iostream>
#include <silicon/cppnet_serve.hh>
#include <silicon/api.hh>

typedef boost::network::http::basic_request<sl::cppnet_server_tag> RQ;
typedef boost::network::http::basic_response<sl::cppnet_server_tag> RS;

auto api = sl::make_api(

  @test = [] () { return D(@message = "hello world."); },
  @test2(@name) = [] (const auto& p) { return iod::D(@message = std::string("hello " + p.name)); }
  // @test3 = [] (RS* r) {
  //   *r = RS::stock_reply(RS::ok, "{\"message\":\"hello world.\"}");
  // }

);

int main(int argc, char* argv[])
{
  sl::cppnet_json_serve(api, atoi(argv[1]));
}
