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
  };

  template <typename... O>
  auto response(const O&... o)
  {
    return response_<decltype(D(o...))>(D(o...));
  }
}
