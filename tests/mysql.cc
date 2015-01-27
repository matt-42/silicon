#include <iostream>
#include <silicon/mysql.hh>

/* Mysql setup script:

CREATE TABLE users
(
id int,
name varchar(255),
age int
);
INSERT into users(id, name, age) values (1, "John", 42);
INSERT into users(id, name, age) values (2, "Bob", 24);
*/

int main()
{
  using namespace sl;

  try
  {
    auto m = mysql_middleware("localhost", "silicon", "my_silicon", "silicon_test");
    auto c = m.instantiate();

    
    auto res = D(@id = int(), @name = std::string(), @age = int());

    auto print = [] (decltype(res) res) {
      std::cout << res.id << " - " << res.name << " - " << res.age << std::endl;
    };

    int count = 0;
    c("SELECT count(*) from users") >> count;
    std::cout << count << " users" << std::endl;

    if (!(c("SELECT * from users") >> res))
      throw error::not_found("User not found.");

    print(res);
    c("SELECT * from users") | print;    
  }
  catch(const std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }
}
