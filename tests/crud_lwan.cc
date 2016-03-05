#include <thread>
#include <iostream>

#include <silicon/api.hh>
#include <silicon/middleware_factories.hh>
//#include <silicon/api_description.hh>
#include <silicon/backends/lwan.hh>
#include <silicon/middlewares/sqlite_connection.hh>
#include <silicon/middlewares/sqlite_orm.hh>
#include <silicon/clients/libcurl_client.hh>
#include <silicon/sql_crud.hh>

#include "symbols.hh"

using namespace s;

typedef decltype(iod::D(_id(_auto_increment, _primary_key) = int(),
                        _name = std::string(),
                        _age = int(),
                        _address = std::string(),
                        _city(_read_only) = std::string()
                   )) User;


int main()
{
  using namespace sl;
  
  auto api = http_api(
    
    _user = sql_crud<sqlite_orm<User>>(
      _before_create = [] (User& u, sqlite_connection& c) { u.city = "Paris"; }
      ) // Crud for the User object.
    );

  auto factories = middleware_factories(
      sqlite_connection_factory("/tmp/sl_test_crud.sqlite", _synchronous = 1), // sqlite middleware.
      sqlite_orm_factory<User>("users") // Orm middleware.
      );

  // std::cout << api_description(api) << std::endl;

  // Start server.
  std::thread t([&] () { lwan_json_serve(api, factories, 12345); });
  usleep(.5e6);

  // Test.
  auto c = libcurl_json_client(api, "127.0.0.1", 12345);

  // // Insert.
  auto insert_r = c.http_post.user.create(_name = "matt", _age =  12, _address = "USA");
  std::cout << json_encode(insert_r) << std::endl;
  assert(insert_r.status == 200);
  int id = insert_r.response.id;

  // Get by id.
  auto get_r = c.http_get.user.get_by_id(_id = id);
  std::cout << json_encode(get_r) << std::endl;;
  assert(get_r.status == 200);
  assert(get_r.response.name == "matt");
  assert(get_r.response.age == 12);
  assert(get_r.response.city == "Paris");
  assert(get_r.response.address == "USA");

  auto get_r2 = c.http_get.user.get_by_id(_id = 42);
  std::cout << get_r2.error << std::endl;;
  assert(get_r2.status == 404);

  // Update
  auto update_r = c.http_post.user.update(_id = id, _name = "john", _age = 42, _address = "Canada 42");
  auto get_r3 = c.http_get.user.get_by_id(_id = id);
  std::cout << json_encode(get_r3) << std::endl;;  
  assert(get_r3.status == 200);
  assert(get_r3.response.id == id);
  assert(get_r3.response.name == "john");
  assert(get_r3.response.age == 42);
  assert(get_r3.response.address == "Canada 42");

  std::cout << json_encode(get_r3) << std::endl;;

  // Destroy.
  auto destroy_r = c.http_post.user.destroy(_id = id);
  assert(destroy_r.status == 200);

  auto get_r4 = c.http_get.user.get_by_id(_id = id);
  assert(get_r4.status == 404);

  std::cout << "CRUD_LWAN PASSED." << std::endl;
  exit(0);
}
