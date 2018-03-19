#include <atomic>
#include <thread>
#include <iostream>

#include <silicon/middlewares/sqlite_connection.hh>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include <silicon/backends/rabbitmq.hh>
#include <silicon/clients/rmq_client.hh>

#include "symbols.hh"

using namespace s;

using namespace sl;

std::atomic_int cpt0;
std::atomic_int cpt1;
std::atomic_int cpt2;
std::atomic_int cpt3;
std::atomic_int cpt4;
std::atomic_int cpt5;

auto rmq_api = sl::rmq::api(
  s::_test = [](sl::sqlite_connection &)
  {
    std::atomic_fetch_add(&cpt0, 1);
  },
  s::_test1 & sl::rmq::parameters(s::_str = std::string()) = [](auto,
                                                                sl::sqlite_connection &)
  {
    std::atomic_fetch_add(&cpt1, 1);
  },
  s::_test2 * s::_rk1 = [](sl::sqlite_connection &)
  {
    std::atomic_fetch_add(&cpt2, 1);
  },
  s::_test3 * s::_rk2 & sl::rmq::parameters(s::_str = std::string()) = [](auto,
                                                                          sl::sqlite_connection &)
  {
    std::atomic_fetch_add(&cpt3, 1);
  },
  s::_test4 * s::_rk1 / s::_qn1 = [](sl::sqlite_connection &)
  {
    std::atomic_fetch_add(&cpt4, 1);
  },
  s::_test5 * s::_rk2 / s::_qn2 & sl::rmq::parameters(s::_str = std::string()) = [](auto,
                                                                                    sl::sqlite_connection &)
  {
    std::atomic_fetch_add(&cpt5, 1);
  }
);
