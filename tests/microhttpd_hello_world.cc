#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

auto hello_api = make_api(

  
  GET / _test = [] () { return D(_message = "hello world."); },

  _test2 * get_parameters(_name = optional(std::string("John")))  =
  [] (const auto& p) { return D(_message = "hello " + p.name); },
  
  _test3 / _name / _city
     * get_parameters(_name = optional(std::string("Rob")))
     * post_parameters(_city = optional(std::string("Paris")))
  =
  [] (const auto& p) { return D(_message = p.name + " lives in " + p.city); },


  _test4 / _name / _city * get_parameters(_name, _city)  =
  [] (const auto& p) { return D(_message = p.name + " lives in " + p.city); },

  _test5 / _id[std::string()] / _city
    *get_parameters(_city)  *post_parameters(_name) =
  [] (const auto& p) { return D(_message = p.name + " with id " + p.id +  " lives in " + p.city); },
  
  _test4 / _name / _city * get_parameters(_name) * post_parameters(_city)  =
  [] (const auto& p) { return D(_message = p.name + " lives in " + p.city); }
  
  // _test2(_name) = [] (const auto& p) { return D(_message = "hello " + p.name); },
  // _test3 = [] () { return "hello world."; },
  // _test4 = [] () { return response(_content_type = "text/html", _body = D(_message = "hello world.")); }
  
);

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mhd_json_serve(hello_api, atoi(argv[1])//, _nthreads=3, _linux_epoll
      );
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
