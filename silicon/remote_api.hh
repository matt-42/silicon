#pragma once

#include <silicon/api.hh>
#include <silicon/backends/ws_api.hh>

namespace sl
{
  using namespace iod;
  using iod::D;

  template <typename A>
  struct remote_procedure
  {
    typedef A arguments_type;
  };

  // Make a remote procedure.
  // @procedure_name * parameters(@arg1 = arg_type1(), @arg2 = ...);
  // Todo: handle return type route  = return_type();
  template <typename M>
  auto make_remote_procedure(M m)
  {
    typedef set_default_string_parameter_t<decltype(m.attributes())> args_type;
    return remote_procedure<args_type>();
  }
  
  template <typename... T>
  auto parse_ws_remote_api(sio<T...> api)
  {
    // Todo: handle nesting.
    return foreach(api) | [] (auto m)
    {
      return make_ws_route(m);
    };
  }

  template <typename... P>
  auto make_ws_remote_api(P... procs)
  {
    return std::make_tuple(make_ws_route(procs)...);
  }

}
