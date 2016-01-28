#include <silicon/api.hh>
#include <silicon/backends/mhd.hh>
#include <silicon/middlewares/sqlite_connection.hh>
#include "symbols.hh"

using namespace s;

int main()
{
  using namespace sl;

  auto api = make_api(
    
    _get_user / _id[int()] = [] (auto p, sqlite_connection& c)
    {
      auto res = D(_name = std::string(), _age = int());
      if (!(c("SELECT name, age from users where id = ?")(p.id) >> res))
        throw error::not_found("User with id ", p.id, " not found.");

      return res;
    }
    )
    .bind_factories(
      sqlite_connection_factory("db.sqlite") // sqlite middleware.
      );

  sl::mhd_json_serve(api, 12345);

}
