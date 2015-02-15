#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

struct request_logger
{
  request_logger()
  {
    time = get_time();
    std::cout << "Request start!" << std::endl;
  }

  ~request_logger()
  {
    std::cout << "Request took " << (get_time() - time) << " microseconds." << std::endl;
  }

  inline double get_time()
  {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return double(ts.tv_sec) * 1e6 + double(ts.tv_nsec) / 1e3;
  }

  static auto instantiate() { return request_logger(); }

private:
  double time;
};

auto hello_api = make_api(

  _test = [] () { return D(_message = "hello world."); }

  ).global_middlewares([] (request_logger&) {});

int main(int argc, char* argv[])
{
  if (argc == 2)
    sl::mhd_json_serve(hello_api, atoi(argv[1]));
  else
    std::cerr << "Usage: " << argv[0] << " port" << std::endl;
}
