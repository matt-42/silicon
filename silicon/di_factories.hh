#pragma once

#include <utility>
#include <iod/di.hh>
#include <iod/tuple_utils.hh>

namespace sl
{

  template <typename F, typename... M2, typename... A2, std::size_t... I>
  decltype(auto) di_factories_call(F f_, std::index_sequence<I...>, std::tuple<M2...>& middlewares, A2&&... args)
  {
    return iod::di_call(f_, std::get<I>(middlewares)..., std::forward<A2>(args)...);
  }
  
  template <typename F, typename... M2, typename... A2>
  decltype(auto) di_factories_call(F f_, std::tuple<M2...>& middlewares, A2&&... args)
  {
    return di_factories_call(f_, std::make_index_sequence<sizeof...(M2)>(),
                             middlewares, std::forward<A2>(args)...);
    // return iod::di_call(f_, iod::tuple_get_by_type<M2>(middlewares)..., std::forward<A2>(args)...);
  }

  
}
