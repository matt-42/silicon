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
    return params_t<sio_type>(D(p...));
  }


  template <typename S = std::tuple<>,
            typename P = iod::sio<>>
  struct ws_route
  {

    typedef S path_type;
    typedef P parameters_type;
    
    ws_route() {}
    ws_route(P p1)
      : params(p1)
    {}

    template <typename NS, typename NP>
    auto ws_route_builder(NS /*s*/, NP p1) const
    {
      return ws_route<NS, NP>(p1);
    }

    template <typename OS, typename OP1>
    auto append(const ws_route<OS, OP1>& o) const
    {
      return ws_builder(std::tuple_cat(path, OS()),
                        iod::cat(params, o.params));
    }

    // Add a path symbol.
    template <typename T>
    auto path_append(const iod::symbol<T>& /*s*/) const
    {
      auto new_path = std::tuple_cat(path, std::make_tuple(T()));
      return ws_route_builder(new_path, params);
    }

    template <typename P1>
    auto format_params(const P1& params) const
    {
      // Forward default values and set default string parameters.
      auto params2 = foreach(params.params) | [&] (auto& m) {
        return static_if<!iod::is_symbol<decltype(m)>::value>(
          [&] (auto x) {
            return x.symbol() = x.value();
          },
          [] (auto x) {
            return x = std::string();
          }, m);
      };
      
      // Set the optional tag for the iod deserializer and default values.
      return foreach(params2) | [&] (auto& m) {
        return static_if<is_optional<decltype(m.value())>::value>(
          [&] (auto x) {
            return x.symbol()(_optional) = x.value().value;
          },
          [] (auto x) {
            return x.symbol() = x.value();
          }, m);
      };

    }
    
    // Set post params.
    template <typename P1>
    auto set_params(const params_t<P1>& new_params) const
    {
      return ws_route_builder(path, format_params(new_params));
    }
    
    auto all_params() const
    {
      return params;
    }

    std::string string_id() const
    {
      std::string s;
      foreach(path) | [&] (auto e)
      {
        s += std::string("/") + e.name();
      };

      return s;
    }

    S path;
    P params;
  };
  

  template <typename S, typename P, typename SS>
  auto route_cat(ws_route<S, P> r,  const iod::symbol<SS>& s)
  {
    return r.path_append(s);
  }

  template <typename S, typename P, typename T>
  auto route_cat(ws_route<S, P> r,  const params_t<T>& s)
  {
    return r.set_params(s);
  }
  
  template <typename S, typename P, typename L, typename R>
  auto route_cat(ws_route<S, P> r, const iod::div_exp<L, R>& e)
  {
    return route_cat(route_cat(r, e.lhs), e.rhs);
  }

  template <typename S, typename P, typename L, typename R>
  auto route_cat(ws_route<S, P> r, const iod::mult_exp<L, R>& e)
  {
    return route_cat(route_cat(r, e.lhs), e.rhs);
  }

  template <typename E>
  auto make_ws_route(const E& e)
  {
    return route_cat(ws_route<>(), e);
  }
  
  template <typename... P>
  auto ws_api(P... procs)
  {
    return parse_api(std::make_tuple(procs...), ws_route<>());
  }
  
}
