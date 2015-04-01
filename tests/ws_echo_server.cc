#include <silicon/api.hh>
#include <silicon/remote_api.hh>
#include <silicon/backends/websocketpp.hh>
#include "symbols.hh"

using namespace s;

int main(int argc, char* argv[])
{
  using namespace sl;
  auto rclient = make_wspp_remote_api(_echo(_text));

  auto server_api = make_api(_echo(_text) = [rclient] (auto p, wspp_connection& con)
  {
    std::cout << p.text << std::endl;
    rclient(con).echo("You just said: " + p.text);
  });

  wspp_json_serve(server_api, atoi(argv[1]));

}
