#include <thread>
#include <iostream>

#include <silicon/api.hh>
#include <silicon/api_description.hh>
#include <silicon/mimosa_serve.hh>
#include <silicon/sqlite.hh>
#include <silicon/sqlite_orm.hh>
#include <silicon/crud.hh>
#include <silicon/client.hh>

typedef decltype(iod::D(@id(@primary_key) = int(),
                        @name = std::string(),
                        @age = int(),
                        @address = std::string()
                   )) User;


int main()
{
  using namespace sl;

  auto api = make_api(
    
    @user = crud<sqlite_orm_middleware<User>>() // Crud for the User object.
    )
    .bind_middlewares(
      sqlite_middleware("/tmp/sl_test_crud.sqlite", @synchronous = 1), // sqlite middleware.
      sqlite_orm_middleware<User>("users") // Orm middleware.
      );

  // Start server.
  std::thread t([&] () { mimosa_json_serve(api, 12345); });
  usleep(.1e6);

  std::cout << api_description(api) << std::endl;
  // api_description print:
  // user.get_by_id(id: int) -> {id: int, name: string, age: int, address: string}
  // user.create(name: string, age: int, address: string) -> {id: int}
  // user.update(id: int, name: string, age: int, address: string) -> void
  // user.destroy(id: int) -> void

  // Test.
  auto c = json_client(api, "127.0.0.1", 12345);

  // Insert.
  auto insert_r = c.user.create("matt", 12, "USA");
  std::cout << json_encode(insert_r) << std::endl;
  assert(insert_r.status == 200);
  int id = insert_r.response.id;

  // Get by id.
  auto get_r = c.user.get_by_id(id);
  assert(get_r.status == 200);
  std::cout << json_encode(get_r) << std::endl;;

  auto get_r2 = c.user.get_by_id(42);
  assert(get_r2.status == 404);
  std::cout << json_encode(get_r2) << std::endl;;

  // Update
  auto update_r = c.user.update(id, "john", 42, "Canada");
  auto get_r3 = c.user.get_by_id(id);
  assert(get_r3.status == 200);
  assert(get_r3.response.id == id);
  assert(get_r3.response.name == "john");
  assert(get_r3.response.age == 42);
  assert(get_r3.response.address == "Canada");

  std::cout << json_encode(get_r3) << std::endl;;

  // Destroy.
  auto destroy_r = c.user.destroy(id);
  assert(destroy_r.status == 200);

  auto get_r4 = c.user.get_by_id(id);
  assert(get_r4.status == 404);
  
  exit(0);
}
