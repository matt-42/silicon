#include <silicon/api.hh>
#include <silicon/api_description.hh>

#include <silicon/middleware_factories.hh>
#include <silicon/middlewares/sqlite_connection.hh>

#include <silicon/clients/rmq_client.hh>

#include "symbols.hh"
#include "backend_testsuite.hh"

int start_count = 0;
int end_count = 0;

struct consume_logger
{
  consume_logger()
  {
    time = get_time();
    std::cout << "Request start!" << std::endl;
    start_count++;
  }

  ~consume_logger()
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

  static consume_logger instantiate() { return consume_logger(); }

  double time;
};

int main(int /*argc*/, char* argv[])
{
  std::cout << sl::api_description(rmq_api);

  auto rmq_api_ga = sl::add_global_middlewares<consume_logger>::to(rmq_api);

  auto f = sl::middleware_factories(
    sl::sqlite_connection_factory("db.sqlite") // sqlite middleware.
    );

  sl::rmq::consume<sl::rmq::utils::tcp_socket>(rmq_api_ga, f,
                                               argv[1], atoi(argv[2]),
                                               s::_username = std::string("guest"),
                                               s::_password = std::string("guest"),
                                               s::_durable = true,
                                               s::_prefetch_count = 1);

};
