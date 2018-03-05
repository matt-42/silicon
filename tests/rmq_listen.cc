#include <silicon/api.hh>
#include <silicon/api_description.hh>
#include <silicon/backends/rabbitmq.hh>
#include <silicon/clients/rmq_client.hh>
#include "symbols.hh"
#include "backend_testsuite.hh"

int main(int /*argc*/, char* argv[])
{
  std::cout << sl::api_description(rmq_api);

  auto ctx = sl::rmq::consume<sl::rmq::utils::tcp_socket>(rmq_api, argv[1], atoi(argv[2]),
                                                          s::_username = std::string("guest"),
                                                          s::_password = std::string("guest"),
                                                          s::_durable = true,
                                                          s::_prefetch_count = 1);

};
