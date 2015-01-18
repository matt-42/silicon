#include <silicon/api.hh>
#include <silicon/mhd_serve.hh>
#include <silicon/sqlite.hh>

int main()
{
  using namespace sl;

  auto api = make_api(
    
    @get_user(@id = int()) = [] (auto p, sqlite_connection& c)
    {
      auto res = D(@name = std::string(), @age = int());
      if (!(c("SELECT name, age from users where id = ?", p.id) >> res))
        throw error::not_found("User with id ", p.id, " not found.");

      return res;
    }
    )
    .bind_middlewares(
      sqlite_middleware("db.sqlite") // sqlite middleware.
      );

  sl::mhd_json_serve(api, 12345);

}
