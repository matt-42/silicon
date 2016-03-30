#include <silicon/api.hh>
#include <silicon/backends/rabbitmq.hh>

iod_define_symbol(test)
iod_define_symbol(test1)
iod_define_symbol(test2)
iod_define_symbol(message)

auto hello_api = sl::http_api(
		s::_test / s::_test1 = [] () { return D(s::_message = "test/test1"); },
		s::_test / s::_test2 = [] () { return D(s::_message = "test/test1"); }
);

int main(int argc, char* argv[])
{
	unsigned short port = 32773;
	auto ctx = sl::rmq_serve(hello_api, port, s::_hostname		= std::string("192.168.99.100"),
											  s::_exchange		= std::string("amq.direct"),
											  s::_username		= std::string("guest"),
											  s::_password		= std::string("guest"));
};
