#include <silicon/api.hh>
#include <silicon/backends/rabbitmq.hh>

iod_define_symbol(test)
iod_define_symbol(test1)
iod_define_symbol(test2)
iod_define_symbol(message)

auto hello_api = sl::http_api(
		s::_test / s::_test1 * sl::get_parameters(s::_id = int()) = [] (auto				params)
		{
			std::cout << "coucou test/test1 " << params.id << std::endl;
			return D(s::_message = "test/test1");
		},
		s::_test / s::_test2 * sl::get_parameters(s::_str = std::string()) = [] (auto		params)
		{
			std::cout << "coucou test/test2 " << params.str << std::endl;
			return D(s::_message = "test/test1");
		}
);

int main(int argc, char* argv[])
{
	auto ctx = sl::rmq::serve(hello_api, atoi(argv[2]), s::_hostname		= std::string(argv[1]),
														s::_username		= std::string("guest"),
														s::_password		= std::string("guest"),
														s::_exchange		= std::string("amq.direct"));
};
