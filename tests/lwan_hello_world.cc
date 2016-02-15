#include <thread>
#include <iostream>
#include <silicon/backends/lwan.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hl_api = http_api(

  
  GET = [] () { return D(_message = "hello world."); },
  GET / _test = [] () { return D(_message = "hello world."); }
  //PUT / _test = [] () { return; }

  // GET / _test2 * get_parameters(_name = optional(std::string("John")))  =
  // [] (const auto& p) { return D(_message = "hello " + p.name); },
  
  // POST / _test3 / _name / _city
  //    * get_parameters(_name = optional(std::string("Rob")))
  //    * post_parameters(_city = optional(std::string("Paris")))
  // =
  // [] (const auto& p) { return D(_message = p.name + " lives in " + p.city); },


  // GET / _test4 / _name / _city * get_parameters(_name, _city)  =
  // [] (const auto& p) { return D(_message = p.name + " lives in " + p.city); },

  // POST / _test5 / _id[std::string()] / _city
  //   *get_parameters(_city)  *post_parameters(_name) =
  // [] (const auto& p) { return D(_message = p.name + " with id " + p.id +  " lives in " + p.city); },
  
  // POST / _test4 / _name / _city * get_parameters(_name) * post_parameters(_city)  =
  // [] (const auto& p) { return D(_message = p.name + " lives in " + p.city); }
  
  
);

int main(int argc, char* argv[])
{


  // Start server.
  std::thread t([&] () { lwan_json_serve(hl_api, 12345); });
  usleep(.1e6);

  auto c = libcurl_json_client(hl_api, "127.0.0.1", 12345);
  auto r1 = c.http_get.test();
  
  std::cout << iod::json_encode(r1) << std::endl;
  exit(0);
}
