#pragma once

#include <sstream>
#include <iostream>

#include <iod/sio_utils.hh>
#include <silicon/symbols.hh>
#include <silicon/optional.hh>

namespace sl
{
namespace rmq
{
  // parameters
  template <typename P>
  struct params_t
  {
    params_t(P p): params(p) {}
    P params;
  };

  template <typename... P>
  auto parameters(P... p)
  {
    typedef decltype(D(std::declval<P>()...)) sio_type;
    return params_t<sio_type>(D(p...));
  }

  // exchanges
  template <typename T>
  struct exchange: public T, public iod::assignable<exchange<T>>, public iod::Exp<exchange<T>>
  {
    using iod::assignable<exchange<T>>::operator=;
  };

  struct exchange_empty
  {
    char const * to_string() const { return ""; }
  };
  exchange<exchange_empty> EXCHANGE_EMPTY;
  auto exchange_to_symbol(exchange_empty) { return s::_empty; }

  struct exchange_direct
  {
    char const * to_string() const { return "amq.direct"; }
  };
  exchange<exchange_direct> EXCHANGE_DIRECT;
  auto exchange_to_symbol(exchange_direct) { return s::_direct; }

  // route composition
  template <typename E = exchange_direct,
            typename R = std::tuple<>,
            typename Q = std::tuple<>,
            typename P = iod::sio<>>
  struct route
  {
    typedef R path_type;
    typedef R routing_key_type;
    typedef Q queue_type;
    typedef P parameters_type;

    route() {}
    route(P p): params(p) {}

    static constexpr auto has_qn = !std::is_same<Q, std::tuple<>>::value;

    // route builder
    template <typename NR, typename NQ, typename NP>
    auto route_builder(NR /*r*/, NQ /*q*/, NP p) const
    {
      return route<E, NR, NQ, NP>(p);
    }

    // routing key append
    template <typename T>
    auto rk_append(iod::symbol<T> const & /*s*/) const
    {
      auto new_rk = std::tuple_cat(routing_key, std::make_tuple(T()));
      return route_builder(new_rk, queue_name, params);
    }

    // queue name append
    template <typename T>
    auto qn_append(iod::symbol<T> const & /*s*/) const
    {
      auto new_qn = std::tuple_cat(queue_name, std::make_tuple(T()));
      return route_builder(routing_key, new_qn, params);
    }

    // Set params.
    template <typename NP>
    auto set_params(params_t<NP> const & np) const
    {
      return route_builder(routing_key, queue_name, format_params(np));
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

    auto all_params() const
    {
      return params;
    }

    auto exchange_as_string() const { return exchange.to_string(); }

    template <typename NE>
    auto set_exchange(exchange<NE> const &) const
    {
      return route<NE, R, Q, P>(params);
    }

    std::string routing_key_as_string() const
    {
      std::stringstream ss;
      bool first = true;

      foreach(routing_key) | [&] (auto e)
      {
        iod::static_if<iod::is_symbol<decltype(e)>::value>(
              [&] (auto e2) { ss << (first ? std::string() : std::string(".")) << e2.name(); first = false; },
              [&] (auto)    { ss << std::string(".*"); }, e);
      };

      return ss.str();
    }

    std::string queue_name_as_string() const
    {
      std::stringstream ss;
      bool first = true;

      foreach(queue_name) | [&] (auto e)
      {
        iod::static_if<iod::is_symbol<decltype(e)>::value>(
              [&] (auto e2) { ss << (first ? std::string() : std::string(".")) << e2.name(); first = false; },
              [&] (auto)    { ss << std::string(".*"); }, e);
      };

      return ss.str();
    }

    std::string path_as_string(bool with_exchange = true) const
    {
      std::stringstream ss;

      if (with_exchange)
        ss << exchange_as_string() << ": ";

      ss << routing_key_as_string();

      //if (has_qn)
      //  ss << " -> " << queue_name_as_string();

      return ss.str();
    }

    std::string string_id() const
    {
      return path_as_string();
    }

    E exchange;
    R routing_key;
    Q queue_name;
    P params;
  };

  namespace internal
  {
    template <bool has_qn, typename B, typename R>
    auto make_route(B b, iod::symbol<R> const & s)
    {
      return iod::static_if<has_qn>(
            [&]() { return b.qn_append(s); },
            [&]() { return b.rk_append(s); });
    }

    template <bool has_qn, typename B, typename R, typename E>
    auto make_route(B b, iod::array_subscript_exp<R, E> const & a)
    {
      return iod::static_if<has_qn>(
            [&]() { return b.qn_append(a); },
            [&]() { return b.rk_append(a); });
    }

    template <bool, typename B, typename E>
    auto make_route(B b, exchange<E> const & e)
    {
      return b.set_exchange(e);
    }

    template <bool, typename B, typename P>
    auto make_route(B b, params_t<P> const & p)
    {
      return b.set_params(p);
    }

    template <bool, typename B, typename L, typename R>
    auto make_route(B b, iod::div_exp<L, R> const & r);

    template <bool, typename B, typename L, typename R>
    auto make_route(B b, iod::mult_exp<L, R> const & r)
    {
      auto lhs = make_route<B::has_qn>(b, r.lhs);
      return make_route<decltype(lhs)::has_qn>(lhs, r.rhs);
    }

    template <bool, typename B, typename L, typename R>
    auto make_route(B b, iod::logical_and_exp<L, R> const & r)
    {
      auto lhs = make_route<B::has_qn>(b, r.lhs);
      return make_route<decltype(lhs)::has_qn>(lhs, r.rhs);
    }

    template <bool, typename B, typename L, typename R>
    auto make_route(B b, iod::div_exp<L, R> const & r)
    {
      return make_route<true>(make_route<B::has_qn>(b, r.lhs), r.rhs);
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
    return rmq::internal::make_route<rmq::route<R1...>::has_qn>(r1, r2);
  }

  template <typename E, typename R, typename Q, typename P>
  auto make_route(rmq::route<E, R, Q, P> const & r)
  {
    return r;
  }

  template <typename E>
  auto make_route(E const & exp)
  {
    rmq::route<> b;
    return rmq::internal::make_route<decltype(b)::has_qn>(b, exp);
  }
}
