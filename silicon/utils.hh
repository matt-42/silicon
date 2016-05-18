#pragma once

#include <tuple>
#include <boost/utility/string_ref.hpp>
#include <iod/utils.hh>

namespace sl
{
  
  template <typename T>
  auto symbol_tuple_to_sio(const std::tuple<>*, T content)
  {
    return content;
  }
  
  template <typename S1, typename T, typename... S>
  auto symbol_tuple_to_sio(const std::tuple<S1, S...>*, T content)
  {
    return D(S1() = symbol_tuple_to_sio((std::tuple<S...>*)0, content));
  }

  inline auto filter_symbols_from_tuple(std::tuple<> path)
  {
    return std::tuple<>();
  }

  template <typename T1, typename... T>
  auto filter_symbols_from_tuple(std::tuple<T1, T...> path)
  {
    return iod::tuple_filter<iod::is_symbol>(path);
  }

  using boost::string_ref;
}
