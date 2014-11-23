#include <iostream>
#include <silicon/mimosa_backend.hh>
#include <silicon/javascript_client.hh>
#include <silicon/server.hh>
#include <silicon/sqlite.hh>

#include "symbols.hh"

struct t3
{
  t3(int _x) : x(_x) { std::cout << "building t3" << std::endl; }
  t3(const t3& t) : x(t.x) { std::cout << "copying t3" << std::endl; }
  static t3 instantiate() { return t3{42}; };
  int x;
};

#define MIDDLEWARE(N)                                                   \
struct t##N                                                             \
{                                                                       \
  t##N(int _x) : x(_x) { std::cout << "building t" #N << std::endl; }   \
  t##N(const t##N& t) : x(t.x) { std::cout << "copying t" #N << std::endl; } \
  static t##N instantiate() { return t##N{42}; };                       \
  int x;                                                                \
};

MIDDLEWARE(4)
MIDDLEWARE(5)
MIDDLEWARE(6)
MIDDLEWARE(7)
MIDDLEWARE(8)
MIDDLEWARE(9)

struct m1;
struct t1
{
  t1(int _x) : x(_x) { std::cout << "building t1" << std::endl; }
  t1(const t1& t) : x(t.x) { std::cout << "copying t1" << std::endl; }
  typedef m1 middleware_type;
  int x;
};

struct m1
{
  m1() { std::cout << "building m1" << std::endl; }
  t1 instantiate() { return t1{42}; };
};


struct m2;
struct t2
{
  t2(int _x) : x(_x) { std::cout << "building t2" << std::endl; }
  t2(const t2& t) : x(t.x) { std::cout << "copying t2" << std::endl; }

  typedef m2 middleware_type;
  int x;
};

struct m2
{
  m2() { std::cout << "building m2" << std::endl; }
  t2 instantiate(t1& t1) { return t2(21 + t1.x); };
};

int main()
{
  using namespace iod;
  auto server = silicon(m1(), m2());
  
  server["h0"](_Id = int()) = [] (t1& i1,
                                  auto params)
  {
    return D(_Id = i1.x + params.id);
  };

  server["h1"](_Id = int()) = [] (t1& i1, t2& i2, t3& i3,
                                  t4&, t5&, t6&, t7&,
                                  auto params)
    {
      std::cout << i1.x << " " << i2.x <<  " " << i3.x << std::endl;
      return D(_Id = i1.x + params.id);
    };

  auto sub_handler = [] (t2& i2, std::string& s) { std::cout << i2.x <<  " -> " << s << std::endl; };
 

  server["h2"] = [&] (t1& i1,
                     dependencies_of<decltype(sub_handler)>& sub_deps)
  {
    t2 i2(22);
    std::string s = "test";
    apply(sub_handler, i2, s, sub_deps, call_with_di);
    return D(_Id = i1.x);
  };
  
  server.serve(8888);
}
