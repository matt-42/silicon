#include <iostream>
#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;


int start_count = 0;
int end_count = 0;

struct request_logger
{
  request_logger()
  {
    time = get_time();
    std::cout << "Request start!" << std::endl;
    start_count++;
  }

  ~request_logger()
  {
    std::cout << "Request took " << (get_time() - time) << " microseconds." << std::endl;
    end_count++;
  }

  inline double get_time()
  {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return double(ts.tv_sec) * 1e6 + double(ts.tv_nsec) / 1e3;
  }

  static request_logger instantiate() { return request_logger(); }

private:
  double time;
};

auto hello_api = make_api(

  _test / _test[int()] = [] (const auto& params) {

    std::stringstream ss;
    ss << "hello " << params.test;
    return D(_message = ss.str());
  },

  _test2 = [] () {

    return D(_message = "hello world");
  }
  

  ).template global_middlewares<request_logger>();

int main(int argc, char* argv[])
{
  std::thread t([&] () { mhd_json_serve(hello_api, 12345); });
  usleep(.1e6);

  // Test.
  auto c = libcurl_json_client(hello_api, "127.0.0.1", 12345);

  c.test2();

  assert(start_count == 1);
  assert(end_count == 1);

  c.test2();
  assert(start_count == 2);
  assert(end_count == 2);

  c.test(_test = 12);
  assert(start_count == 3);
  assert(end_count == 3);

  
  exit(0);
}
