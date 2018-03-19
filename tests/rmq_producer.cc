#include <silicon/api.hh>

#include <silicon/middleware_factories.hh>
#include <silicon/middlewares/sqlite_connection.hh>

#include <silicon/clients/rmq_client.hh>

#include "symbols.hh"
#include "backend_testsuite.hh"

int main(int /*argc*/, char* argv[])
{
  auto c = sl::rmq::json_producer(rmq_api, argv[1], atoi(argv[2]),
                                  s::_username = std::string("guest"),
                                  s::_password = std::string("guest"),
                                  s::_durable = true);
  c.direct.test();
  c.direct.test1(s::_str = "string");
  c.direct.test2.rk1();
  c.direct.test3.rk2(s::_str = "string");
  c.direct.test4.rk1();
  c.direct.test5.rk2(s::_str = "string");
};
