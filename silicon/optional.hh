#pragma once

#ifndef IOD_SYMBOL_optional
#define IOD_SYMBOL_optional
iod_define_symbol(optional)
#endif

namespace sl
{
  template <typename T>
  struct optional_
  {
    optional_() {}
    optional_(T v) : value(v) {}

    operator T() { return value; }

    T value;
  };

  template <typename T>
  struct is_optional
  {
    template<typename C> 
    static char test(optional_<C>*);
    static int test(...);
    static const bool value = sizeof(test((std::decay_t<T>*)0)) == 1;
  };

  template <typename T>
  auto optional(T v)
  {
    return optional_<T>(v);
  }
}
