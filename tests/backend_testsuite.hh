#include <silicon/middlewares/hashmap_session.hh>
#include <atomic>
#include <thread>
#include <iostream>
#include <silicon/api.hh>
#include <silicon/clients/libcurl_client.hh>
#include <silicon/backends/rabbitmq.hh>
#include <silicon/clients/rmq_client.hh>

#include "symbols.hh"

using namespace s;

using namespace sl;


struct session
{
  int id;
};

auto hl_api = http_api(


  // get
  GET / _test = [] () { return D(_message = "get"); },

  // get params
  GET / _test2 * get_parameters(_id = int(0)) = [] (auto p) { return D(_id = p.id); },
  
  // post params
  POST / _test2 * post_parameters(_id = int(0), _name = std::string()) = [] (auto p) { return D(_id = p.id, _name = p.name ); },

  // post object params
  POST / _test4 * post_parameters(_id = D(_name = std::string())) = [] (auto p) { return D(_name = p.id.name ); },


  // post array params
  POST / _test5 * post_parameters(_id = std::vector<int>()) = [] (auto p) { return D(_name = p.id ); },
  
  // url params
  GET / _test3 / _id[int(0)] = [] (auto p) { return D(_id = p.id); },

  // cookie
  GET / _set_id / _id[int(0)] = [] (auto p, session& s) {
    s.id = p.id;
    return D(_id = s.id);
  },

  GET / _get_id = [] (session& s) {
    return D(_id = s.id);
  }
  
);

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

template <typename T>
void backend_testsuite(T port)
{
  auto c1 = libcurl_json_client(hl_api, "127.0.0.1", port, _post_encoding = _x_www_form_urlencoded);
  auto c2 = libcurl_json_client(hl_api, "127.0.0.1", port);

  auto r1 = c1.http_get.test();
  assert(r1.status == 200);
  assert(r1.response.message == "get");

  auto r3 = c1.http_get.test2(_id = 12);
  assert(r3.status == 200);
  assert(r3.response.id == 12);

  auto r4 = c1.http_post.test2(_id = 12, _name = "s pa ces");
  assert(r4.status == 200);
  assert(r4.response.id == 12);
  assert(r4.response.name == "s pa ces");

  auto r41 = c2.http_post.test4(_id = D(_name = "John"));
  assert(r41.status == 200);
  assert(r41.response.name == "John");
  
  auto r5 = c1.http_get.test3(_id = 12);
  assert(r5.status == 200);
  assert(r5.response.id == 12);

  auto r6 = c1.http_get.set_id(_id = 42);
  auto r7 = c2.http_get.set_id(_id = 51);
  auto r8 = c1.http_get.get_id();
  auto r9 = c2.http_get.get_id();

  assert(r8.response.id == 42);
  assert(r9.response.id == 51);

}
