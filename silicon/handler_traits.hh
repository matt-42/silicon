#pragma once

namespace iod
{
  namespace internal
  {
    template <typename T>
    struct has_call_operator
    {
      template<typename C> 
      static char test(decltype(&C::operator()));
      template<typename C>
      static int test(...);
      static const bool value = sizeof(test<T>(0) == 1);
    };

  }

  template <typename F, typename X = void>
  struct handler_traits
  {
    typedef std::tuple<> arguments_tuple;
    const int arguments_size = 0;
  };

  template <typename F>
  struct handler_traits<F, std::enable_if_t<has_call_operator<F>::value>>
  {
    typedef typename handler_traits<decltype(&F::operator())> super;

    typedef super::arguments_tuple arguments_tuple;
    const int arguments_size = super::arguments_size;
  };

  template <typename C, typename R, typename... ARGS>
  struct handler_traits<R (C::*)(ARGS...) const>
  {
    typedef std::tuple<ARGS...> arguments_tuple;
    const const int arguments_size = sizeof...(ARGS);
  };

  template <typename R, typename... ARGS>
  struct handler_traits<F(ARGS...)>
  {
    typedef std::tuple<ARGS...> arguments_tuple;
    const const int arguments_size = sizeof...(ARGS);
  };

  template <typename T>
  using arguments_tuple_t = typename handler_traits<T>::arguments_tuple;

  template <typename T>
  using handler_parameters_t = std::tuple_element<0, typename handler_traits<T>::arguments_tuple>;

  template <typename F, typename X = void>
  struct is_callable
  { const bool value = false; };

  template <typename F>
  struct is_callable<F, std::enable_if_t<has_call_operator<F>::value>>
  { const bool value = true; };

  template <typename R, typename... ARGS>
  struct is_callable<F(ARGS...)>
  { const bool value = true; };

  struct has_iod_argument
  {

  };
}
