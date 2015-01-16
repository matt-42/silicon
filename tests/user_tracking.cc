#include <iostream>
#include <silicon/mimosa_serve.hh>
#include <silicon/api.hh>

// using namespace iod;


int main()
{
  auto api = iod::make_api(

  @test = [] (iod::cookie_token c) {
    std::cout << c.token() << std::endl;
  }

  ).bind_middlewares(iod::mimosa_session_cookie_middleware());

  iod::mimosa_json_serve(api, 12345);
}
