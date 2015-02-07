#include <iostream>
#include <silicon/backends/mhd_serve.hh>
#include <silicon/api.hh>

using namespace sl;

struct request_logger
{
  request_logger(std::string x) : xx(x)
  {
    std::cout << "Request start!" << std::endl;
  }

  ~request_logger()
  {
    std::cout << "Request end! " << xx << std::endl;
  }

  std::string xx;
  
  static auto instantiate() { return request_logger("xxxx"); }
};

auto hello_api = make_api(

  @test = [] () { return D(@message = "hello world."); }

  ).global_middlewares([] (request_logger&) {});

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mhd_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
