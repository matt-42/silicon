#include <chrono>

#include <silicon/api.hh>
#include <silicon/clients/rmq_client.hh>
#include "symbols.hh"
#include "backend_testsuite.hh"

using namespace std::chrono_literals;

constexpr static unsigned int nworkers = 10;
constexpr static unsigned int nmessages = 1000;

int main(int, char const * argv[])
{
  std::vector<std::thread> workers;
  
  for (unsigned int i = 0; i < nworkers; ++i)
  {
    std::thread worker([&]()
    {
      sl::rmq::consume<sl::rmq::utils::tcp_socket>(rmq_api, argv[1], atoi(argv[2]),
                                                   s::_username = std::string("guest"),
                                                   s::_password = std::string("guest"),
                                                   s::_durable = true,
                                                   s::_prefetch_count = 1);
    });
    worker.detach();

    workers.emplace_back(std::move(worker));
  }


  std::this_thread::sleep_for(2s);

  auto c = sl::rmq::json_producer(rmq_api, argv[1], atoi(argv[2]),
                                  s::_username = std::string("guest"),
                                  s::_password = std::string("guest"),
                                  s::_durable = true);

  for (unsigned int i = 0; i < nmessages; ++i)
  {
    c.direct.test();
    c.direct.test1(s::_str = "string");
    c.direct.test2.rk1();
    c.direct.test3.rk2(s::_str = "string");
    c.direct.test4.rk1();
    c.direct.test5.rk2(s::_str = "string");
  }

  std::this_thread::sleep_for(10s);

  assert(cpt0 == nmessages * nworkers);
  assert(cpt1 == nmessages * nworkers);
  assert(cpt2 == nmessages * nworkers);
  assert(cpt3 == nmessages * nworkers);
  assert(cpt4 == nmessages);
  assert(cpt5 == nmessages);
}
