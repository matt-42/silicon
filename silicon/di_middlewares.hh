#include <iod/di.hh>
#include <iod/tuple_utils.hh>

namespace sl
{
  template <typename F, typename... M2, typename... A2>
  auto di_middlewares_call(F f_, std::tuple<M2...>& middlewares, A2&&... args)
  {
    return iod::di_call(f_, iod::tuple_get_by_type<M2>(middlewares)..., args...);
  }

}
