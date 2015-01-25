#include <silicon/api.hh>
#include <silicon/remote_api.hh>
#include <silicon/websocketpp.hh>

int main(int argc, char* argv[])
{
  using namespace sl;
  auto client_api = make_remote_api(@echo(@text));

  typedef ws_client<decltype(client_api)> client;

  auto server_api = make_api(@echo(@text) = [] (auto p, wspp_connection& con, client& c)
  {
    std::cout << p.text << std::endl;
    c(con).echo("You just said: " + p.text);
  });

  wspp_json_serve(server_api, client_api, atoi(argv[1]));

}
