#pragma once

#include <iod/sio.hh>
#include <iod/sio_utils.hh>

namespace sl
{
  template <typename O>
  struct response_ : public O
  {
  public:
    response_(O o) : O(o) {}
    // using O::O;
  };

  template <typename... O>
  auto response(const O&... o)
  {
    // auto x = D(o...);
    // response_<decltype(x)> res(x);
    return response_<decltype(D(o...))>(D(o...));
  }
}
