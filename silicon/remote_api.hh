#pragma once

#include <silicon/api.hh>

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
  // @procedure_name(@arg1 = arg_type1(), @arg2 = ...) = return_type();
  template <typename M>
  auto make_remote_procedure(M m)
  {
    typedef set_default_string_parameter_t<decltype(m.attributes())> args_type;
    return remote_procedure<args_type>();
  }
  
  template <typename... T>
  auto parse_remote_api(sio<T...> api)
  {
    return foreach(api) | [] (auto m)
    {
      return static_if<is_sio<decltype(m.value())>::value>(
        [] (auto m) { // If sio, recursion.
          return m.symbol() = parse_remote_api(m.value());
        },
        [] (auto m) { // Else, register the procedure.
          return m.symbol() = make_remote_procedure(m);
        }, m);
    };
  }
  
  template <typename P>
  struct remote_api
  {
    remote_api(const P& procs)
      : procedures_(procs)
    {
    }
    
    const auto& procedures() const
    {
      return procedures_;
    }
    
    P procedures_;
  };

  template <typename... P>
  auto make_remote_api(P... procs)
  {
    auto a = parse_remote_api(D(procs...));
    return api<decltype(a), std::tuple<>>(a, std::tuple<>());
  }
  
}
