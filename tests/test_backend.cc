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



  for (int i = 0; i < 100000; i++)
  {
    iod::request request(R"json({"handler_id":3}{"mydata":123})json");
    iod::response response;
    //request.get_body_stream().seekg(0);
    s.handle(request, response);
  }

  //std::cout << response.get_body_string() << std::endl;
}
