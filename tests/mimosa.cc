#include <iostream>
#include <silicon/mimosa_backend.hh>
#include <silicon/javascript_client.hh>
#include <silicon/server.hh>


iod_define_symbol(mydata, _Mydata);
iod_define_symbol(message, _Message);

std::string simple_function_handler()
{
  return "ok\n";
}

int main(int argc, char* argv[])
{
  using namespace iod;
  using s::_Mydata;
  using s::_Handler_id;

  auto s = silicon();

  // A simple hello world handler.
  s["h1"] = "Hello world!";

  s["h2"] = D(_Message = "hello_world");
  
  s["h3"] = [] () { return "I'm a lambda"; };

  s["h4"] = [] (decltype(D(_Mydata = int())) data)
  { 
    return data;
  };

  s["h5"] = [] (response& resp,
                decltype(D(_Mydata = int())) data)
  {
    resp << data.mydata;
  };

  s["h6"] = simple_function_handler;

  std::cout << generate_javascript_client(s) << std::endl;
  s.serve();
}
