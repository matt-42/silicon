#include <functional>
#include <type_traits>
#include <boost/type_traits.hpp>

template <typename T>
struct has_call_operator
{
  template<typename C> 
  static char test(decltype(&C::operator()));

  // template<typename C>
  // static char (&test(...))[2];

  template<typename C>
  static int test(...);

  static const bool value = (sizeof( test<T>(0)  ) == 1);

};

template <typename F, typename X = void>
struct S
{
};

template <typename F>
struct S<F, std::enable_if_t<has_call_operator<F>::value>>
{
  typedef typename S<decltype(&F::operator())>::arg arg;
  //typedef F arg;
};

template <typename C, typename R, typename A, typename... ARGS>
struct S<R (C::*)(A, ARGS...) const>
{
  typedef A arg;
};

template <typename R, typename A>
struct S<R(A)>
{
  typedef A arg;
};

//struncg
template <typename R, typename A>
void f(R f(A))
{
}

template <typename F, typename A>
void g(F(A))
{
}

float fun(int a) { return 0; }

int main()
{
  auto l = [] (int a, float b) -> float { return float(12); };
  //g();
  //g(fun);

  typedef decltype(l) L;
  S<int> s;
  //S<decltype(l)> t;

  //S<decltype(&L::operator())>::arg x;
  //S<decltype(fun)>::arg x;
  S<L>::arg x;
  //void* x = S<std::function<int(float)>>::arg();
  //boost::function_traits<L>::arg1_type x;
  //boost::function_traits<decltype(&L::operator())>::arg1_type x;
  void* xx = x;
}
