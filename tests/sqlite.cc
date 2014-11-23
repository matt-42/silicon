#include <iostream>

#include <iod/sio_utils.hh>

//#include <silicon/microhttpd_backend.hh>
#include <silicon/mimosa_backend.hh>
#include <silicon/sqlite.hh>
#include <silicon/server.hh>

#include "symbols.hh"

#define silicon_params(...) decltype(iod::D(__VA_ARGS__))

using namespace s;

typedef decltype(iod::D(_Id = int(),
                        _Name = std::string(),
                        _Age = int(),
                        _Address = std::string(),
                        _Salary = float()
                   )) User;

int main()
{
  using namespace iod;
  
  sqlite_connection db;
  db.connect("./db.sqlite");

  db("SELECT name, age FROM user where age > ?", 10)
    (_Name = std::string(), _Age = int())
    | [] (auto& r)
  {
    std::cout << r.name << " " << r.age << std::endl;
  };
  
  db("SELECT * FROM user") | [] (User& u)
  {
    foreach (u) | [] (auto& m) { std::cout << m.symbol().name() << ": " << m.value() << std::endl; };
    std::cout << std::endl;
  };

  auto server = silicon(sqlite_middleware("./db.sqlite"));

  server["test1"] = "test";
  server["test2"] = [] (decltype(iod::D(_Id = int())) params, sqlite_connection& db)
  {
    std::vector<User> v;
    db("SELECT * FROM user where id = ? limit 1", params.id).append_to(v);
    if (v.size() == 0)
      throw error::not_found("User with id = ", params.id, " does not exist.");
    else
      return v[0];
  };
  server.serve(8888);
}
