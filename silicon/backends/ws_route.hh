#pragma once

#include <iod/sio_utils.hh>
#include <silicon/symbols.hh>
#include <silicon/optional.hh>


namespace sl
{
  using namespace iod;
  using namespace s;

  template <typename P>
  struct params_t
  {
    params_t(P p) : params(p) {}
    P params;
  };
  
  template <typename... P>
  auto parameters(P... p)
  {
    typedef decltype(D(std::declval<P>()...)) sio_type;
    return post_params_t<sio_type>(D(p...));
  }

  
}
