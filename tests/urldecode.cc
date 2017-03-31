#include <iostream>
#include <silicon/backends/urldecode.hh>
#include "symbols.hh"

using namespace s;
using namespace sl;

template <typename O>
bool is_error(std::string s, O& obj)
{
  bool err = false;
  try
  {
    urldecode(s, obj);
  }
  catch (error::error e)
  {
    std::cout << e.what() << std::endl;
    err = true;
  }
  catch (std::exception& e)
  {
    std::cout << e.what() << std::endl;
    err = true;
  }
  return err;
}

int main()
{
  { // Simple object
    const std::string s = "name=John&age=42";

    auto obj = D(s::_name = std::string(),
                 s::_age = int());

    urldecode(iod::stringview(s), obj);
    assert(obj.name == "John");
    assert(obj.age == 42);
  }

  { // Simple object with url escaped chars
    const std::string s = "name=Jo%20hn";
    auto obj = D(s::_name = std::string());
    urldecode(iod::stringview(s), obj);
    assert(obj.name == "Jo hn");
  }
  
  { // Simple Array.
    const std::string s = "age[0]=42&age[1]=22";

    auto obj = D(s::_age = std::vector<int>());

    urldecode(iod::stringview(s), obj);

    assert(obj.age.size() == 2);
    assert(obj.age[0] == 42);
    assert(obj.age[1] == 22);
  }

  { // Simple Array 2.
    const std::string s = "age[]=42&age[]=22";

    auto obj = D(s::_age = std::vector<int>());

    urldecode(iod::stringview(s), obj);

    assert(obj.age.size() == 2);
    assert(obj.age[0] == 42);
    assert(obj.age[1] == 22);
  }
  
  { // Nested objects
    const std::string s = "test1[name]=John&test1[age]=42&test2[name]=Bob&test2[age]=12";

    auto obj = D(s::_test1 = D(s::_name = std::string(),
                               s::_age = int()),
                 s::_test2 = D(s::_name = std::string(),
                               s::_age = int()));

    urldecode(iod::stringview(s), obj);

    assert(obj.test1.name == "John");
    assert(obj.test1.age == 42);
    assert(obj.test2.name == "Bob");
    assert(obj.test2.age == 12);
  }

  { // Nested array.
    const std::string s = "test1[0][0]=42";
    auto obj = D(s::_test1 = std::vector<std::vector<int>>());

    urldecode(iod::stringview(s), obj);
    assert(obj.test1[0][0] == 42);
  }

  { // Nested array.
    const std::string s = "test1[0][test2]=John";
    typedef decltype(D(s::_test2 = std::string())) elt_type;

    auto obj = D(s::_test1 = std::vector<elt_type>());

    urldecode(iod::stringview(s), obj);
    assert(obj.test1[0].test2 == "John");
  }
  
  { // Missing field
    const std::string s = "name=John";

    auto obj = D(s::_name = std::string(),
                 s::_age = int());

    assert(is_error(s, obj));
  }

  { // Missing nested field
    const std::string s = "age=42";
    typedef decltype(D(s::_test2 = std::string())) elt_type;

    auto obj = D(s::_name = elt_type(),
                 s::_age = int());

    assert(is_error(s, obj));
  }

  { // Bad interger formating
    const std::string s = "age=42s";
    typedef decltype(D(s::_test2 = std::string())) elt_type;

    auto obj = D(s::_age = int());

    assert(is_error(s, obj));
  }

  { // Missing field.
    const std::string s = "age=";
    auto obj = D(s::_age = int());
    assert(is_error(s, obj));
  }


  { // Missing =
    auto obj = D(s::_age = int());
    assert(is_error("age", obj));
    assert(is_error("age&&", obj));
    assert(is_error("", obj));
  }
  
}
