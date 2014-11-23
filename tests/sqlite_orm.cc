#include <iostream>

#include <iod/sio_utils.hh>

#include <silicon/mimosa_backend.hh>
#include <silicon/sqlite.hh>
#include <silicon/sqlite_orm.hh>
#include <silicon/server.hh>

#include "symbols.hh"

using namespace s;

typedef decltype(iod::D(_Id(_Primary_key) = int(),
                        _Name = std::string(),
                        _Age = int(),
                        _Address = std::string(),
                        _Salary = float()
                   )) User;

int main()
{
  using namespace iod;
  
  auto server = silicon(sqlite_middleware("./db.sqlite"),
                        sqlite_orm_middleware<User>("user"));

  server["test2"](_Id = int()) = [] (auto p,
                                     sqlite_orm<User>& users)
  {
    User u;
    if (!users.find_by_id(p.id, u))
      throw error::not_found(" Id ", p.id, " not found.");

    auto new_user = sqlite_orm<User>::O_WO_PKS("Robert", 45, "LA", 345678);
    int id = users.insert(new_user);
    users.find_by_id(id, u);
    u.name = "Robert3";
    users.update(u);

    u.name = "";
    users.find_by_id(id, u);
    users.destroy(D(_Id = id));
    return u;
  };

  server.serve(8888);
}
