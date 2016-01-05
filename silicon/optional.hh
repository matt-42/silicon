#pragma once

namespace sl
{
  template <typename T>
  struct optional_ : public T
  {
    using T::T;
    using T::operator=;
    optional_() {}
    optional_(T v) : T(v) {}
    // T value;
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
