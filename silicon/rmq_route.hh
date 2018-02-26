#pragma once

#include <sstream>

#include <iod/sio_utils.hh>
#include <silicon/symbols.hh>
#include <silicon/optional.hh>

namespace sl
{
namespace rmq
{
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

  template <typename T>
  struct exchange: public T, public iod::assignable<exchange<T>>, public iod::Exp<exchange<T>>
  {
    using iod::assignable<exchange<T>>::operator=;
  };

  struct exchange_empty { char const * to_string() { return ""; } }; exchange<exchange_empty> RMQ_EMPTY;
  struct exchange_direct { char const * to_string() { return "amq.direct"; } }; exchange<exchange_direct> RMQ_DIRECT;

  template <typename E = exchange_direct,
            typename S = std::tuple<>,
            typename P = iod::sio<>>
  struct route
  {
    typedef S path_type;
    typedef P parameters_type;

    route() {}
    route(P p1): params(p1) {}

    template <typename NS, typename NP>
    auto route_builder(NS /*s*/, NP p) const
    {
      return route<E, NS, NP>(p);
    }

    template <typename T>
    auto path_append(iod::symbol<T> const & /*s*/) const
    {
      auto new_path = std::tuple_cat(path, std::make_tuple(T()));
      return route_builder(new_path, params);
    }

    template <typename NP>
    auto format_params(NP const & params) const
    {
      // Forward default values and set default string parameters.
      auto params2 = foreach(params.params) | [&] (auto& m)
      {
        return iod::static_if<!iod::is_symbol<decltype(m)>::value>(
                [&] (auto x)
                {
                  return x.symbol() = std::move(x.value());
                },
                [] (auto x)
                {
                  return x = std::string();
                }, m);
      };

      // Set the optional tag for the iod deserializer and default values.
      return foreach(params2) | [&] (auto& m)
      {
        return iod::static_if<is_optional<decltype(m.value())>::value>(
                [&] (auto x)
                {
                  return x.symbol()(s::_optional) = std::move(x.value().value);
                },
                [] (auto x)
                {
                  return x.symbol() = std::move(x.value());
                }, m);
      };
    }

    // Set get params.
    template <typename NP>
    auto set_params(params_t<NP> const & new_params) const
    {
      return route_builder(path, format_params(new_params));
    }

    auto all_params() const
    {
      return params;
    }

    auto exchange_as_string(E e) const { return e.to_string(); }
    auto exchange_as_string() const { return exchange_as_string(exchange); }

    std::string path_as_string(bool with_exchange = true) const
    {
      std::string s;
      bool first = true;

      if (with_exchange)
        s += exchange_as_string(exchange);

      foreach(path) | [&] (auto e)
      {
        iod::static_if<iod::is_symbol<decltype(e)>::value>(
              [&] (auto e2) { s += (first ? std::string() : std::string(".")) + e2.name(); first = false; },
              [&] (auto)    { s += std::string(".*"); }, e);
      };
      return s;
    }

    std::string string_id() const
    {
      return path_as_string();
    }

    E exchange;
    S path;
    P params;
  };

  namespace internal
  {
    template <typename B, typename S>
    auto make_route(B b, iod::symbol<S> const & s)
    {
      return b.path_append(s);
    }

    template <typename B, typename E>
    auto make_route(B b, exchange<E> const & e)
    {
      return b.set_exchange(e);
    }

    template <typename B, typename P>
    auto make_route(B b, params_t<P> const & new_params)
    {
      return b.set_params(new_params);
    }

    template <typename B, typename S, typename E>
    auto make_route(B b, iod::array_subscript_exp<S, E> const & e)
    {
      return b.path_append(e);
    }

    template <typename B, typename L, typename R>
    auto make_route(B b, iod::div_exp<L, R> const & r);

    template <typename B, typename L, typename R>
    auto make_route(B b, iod::mult_exp<L, R> const & r)
    {
      return make_route(make_route(b, r.lhs), r.rhs);
    }

    template <typename B, typename L, typename R>
    auto make_route(B b, iod::div_exp<L, R> const & r)
    {
      return make_route(make_route(b, r.lhs), r.rhs);
    }
  }
}
  template <typename... R1, typename... R2>
  auto route_cat(rmq::route<R1...> const & r1,
                 rmq::route<R2...> const & r2)
  {
    return r1.append(r2);
  }

  template <typename... R1, typename R2>
  auto route_cat(rmq::route<R1...> const & r1,
                 R2 const & r2)
  {
    return rmq::internal::make_route(r1, r2);
  }

  template <typename E, typename S, typename P>
  auto make_route(rmq::route<E, S, P> const & r)
  {
    return r;
  }

  template <typename E>
  auto make_route(E const & exp)
  {
    rmq::route<> b;
    return rmq::internal::make_route(b, exp);
  }
}
