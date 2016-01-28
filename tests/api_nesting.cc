#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

// auto hello_api = make_api(

//   _test2 = [] () { return D(_message = "hello world."); },

//   _test = std::make_tuple(
//     PUT / _test2 = [] () { return D(_message = "hello world from the nested api."); },

//     _test3  / _id[int()] = std::make_tuple(
//       GET / _test4  = [] (auto p) { std::cout << p.id << std::endl;
//                                    return D(_message = "hello world from the nested nested api."); },
//       GET * get_parameters(_name = std::string())  = [] (auto p) { std::cout << p.name << std::endl;
//                                    return D(_message = "hello world from the nested nested api."); }
//       // POST / _test5 = [] (auto p) { return D(_message = "hello world from the nested nested api."); }
//       )

//     )
// );

auto hello_api = make_api(

  GET
  = [] (auto p) { return D(_message = "This is the root of the server"); },
  _test2 = [] () { return D(_message = "hello world."); },

  _test3 = std::make_tuple(
    POST / _id[int()] * post_parameters(_name = std::string())
      = [] (auto p) { return D(_message = "This is the root of test3"); },

    GET / _test2 = [] () { return D(_message = "hello world from test3."); }
    )
);

int main(int argc, char* argv[])
{

  auto ctx = mhd_json_serve(hello_api, 12345);

  auto c = libcurl_json_client(hello_api, "127.0.0.1", 12345);
  auto r1 = c(_id = 23, _name = "John==&");
  auto r2 = c.test2();
  auto r3 = c.test3(_id = 12, _name = "H");
  auto r4 = c.test3.test2();
  
  std::cout << json_encode(r1) << std::endl;
  std::cout << json_encode(r2) << std::endl;
  std::cout << json_encode(r3) << std::endl;
  std::cout << json_encode(r4) << std::endl;
}
