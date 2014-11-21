#include <iostream>

#include <iod/sio_utils.hh>

#include <silicon/mimosa_backend.hh>
#include <silicon/sqlite.hh>
#include <silicon/sqlite_orm.hh>
#include <silicon/crud.hh>
#include <silicon/server.hh>

#include "symbols.hh"

using namespace s;

typedef decltype(iod::D(_Id(_Primary_key) = int(),
                        _Name = std::string(),
                        _Age = int(),
                        _Address = std::string(),
                        _Salary = float()
                   )) User;

int main(int argc, char* argv[])
{
  using namespace iod;
  using namespace s;
  
  sqlite_connection db;
  db.connect("./db.sqlite");

  //auto sql = sqlite_middleware("./db.sqlite");
  auto server = silicon(sqlite_middleware("./db.sqlite"),
                        sqlite_orm_middleware<User>("user"));

  setup_crud(server, get_middleware<sqlite_orm_middleware<User>>(server), _Prefix = "user");
  server.serve();
}
