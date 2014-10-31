#include <iostream>
#include <silicon/test_backend.hh>
#include <silicon/server.hh>


iod_define_symbol(mydata, _Mydata);

int main(int argc, char* argv[])
{
  using namespace iod;
  using s::_Mydata;
  using s::_Handler_id;

  iod::server s(argc, argv);

  // A simple hello world handler.
  // With silicon
  s["h1"] = std::string("Hello world!");

  s["h2"] = [] () { return "I'm a lambda"; };

  s["h3"] = [] (decltype(D(_Mydata = int())) data)
  { 
    return data.mydata;
  };

  s["h4"] = [] (response& resp,
                decltype(D(_Mydata = int())) data)
  {
    resp << data.mydata;
  };


  std::istringstream request(R"json({"handler_id":2}{"mydata":123})json");
  std::ostringstream response;
  s.handle(request, response);

  std::cout << response.str() << std::endl;
}
