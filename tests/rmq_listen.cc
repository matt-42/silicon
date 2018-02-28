#include <silicon/api.hh>
#include <silicon/api_description.hh>
#include <silicon/backends/rabbitmq.hh>

iod_define_symbol(rk1)
iod_define_symbol(rk2)
iod_define_symbol(qn1)
iod_define_symbol(qn2)

iod_define_symbol(message)

auto hello_api = sl::rmq::api(
  s::_rk1 = []() { },
  s::_rk2 & sl::rmq::parameters(s::_str = std::string()) = [](auto) { },

  s::_rk1 * s::_rk1 = []() { },
  s::_rk2 * s::_rk2 & sl::rmq::parameters(s::_str = std::string()) = [](auto) { },

  s::_rk1 * s::_rk1 / s::_qn1 = []() { },
  s::_rk2 * s::_rk2 / s::_qn2 & sl::rmq::parameters(s::_str = std::string()) = [](auto) { },

  s::_rk1 * s::_rk1 / s::_qn1 * s::_qn1 = []() { },
  s::_rk2 * s::_rk2 / s::_qn2 * s::_qn2 & sl::rmq::parameters(s::_str = std::string()) = [](auto) { }
);

int main(int /*argc*/, char* argv[])
{
  std::cout << sl::api_description(hello_api);

  auto ctx = sl::rmq::consume<sl::rmq::utils::tcp_socket>(hello_api, atoi(argv[2]),
                                                          s::_hostname = std::string(argv[1]),
                                                          s::_username = std::string("guest"),
                                                          s::_password = std::string("guest"),
                                                          s::_durable = true,
                                                          s::_prefetch_count = 1);

};
