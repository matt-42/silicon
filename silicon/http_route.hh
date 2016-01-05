#pragma once

#include <iod/sio_utils.hh>
#include <silicon/symbols.hh>
#include <silicon/optional.hh>

namespace sl
{
  using namespace iod;
  using namespace s;

  template <typename P>
  struct post_params_t
  {
    post_params_t(P p) : params(p) {}
    P params;
  };

  template <typename P>
  struct get_params_t
  {
    get_params_t(P p) : params(p) {}
    P params;
  };

  template <typename... P>
  auto post_parameters(P... p) {
    typedef decltype(D(std::declval<P>()...)) sio_type;
    return post_params_t<sio_type>(D(p...));
  }

  template <typename... P>
  auto get_parameters(P... p) {
    typedef decltype(D(std::declval<P>()...)) sio_type;
    return get_params_t<sio_type>(D(p...));
  }

  // -------------------------------------------
  // set_default_string_parameter_t

  template <typename U, bool>
  struct add_missing_string_value_types { typedef U type; };
  template <typename U>
  struct add_missing_string_value_types<U, true> {
    typedef typename U::template variable_type<std::string> type;
  };
  
  template <typename T>
  struct set_default_string_parameter
  {
    typedef T type;
  };

  template <typename... T>
  struct set_default_string_parameter<iod::sio<T...>>
  {
    typedef sio<typename add_missing_string_value_types<T, std::is_base_of<symbol<T>, T>::value>::type...> type;
  };
  template <typename T>
  using set_default_string_parameter_t = typename set_default_string_parameter<T>::type;
  // -------------------------------------------

  
  template <typename T> struct http_verb : public T {};
  struct http_get {};    static http_verb<http_get> GET;
  struct http_post {};   static http_verb<http_post> POST;
  struct http_put {};    static http_verb<http_put> PUT;
  struct http_delete {}; static http_verb<http_delete> DELETE;
  struct http_verb_any {}; static http_verb<http_verb_any> ANY;

  template <typename V = http_verb_any,
            typename S = std::tuple<>,
            typename P1 = iod::sio<>,
            typename P2 = iod::sio<>,
            typename P3 = iod::sio<>>
  struct http_route
  {

    typedef S path_type;
    typedef P1 url_parameters_type;
    typedef P2 get_parameters_type;
    typedef P3 post_parameters_type;
    
    http_route() {}
    http_route(S s, P1 p1, P2 p2, P3 p3)
      : path(s),
        url_params(p1),
        get_params(p2),
        post_params(p3)
    {}

    template <typename NS, typename NP1, typename NP2, typename NP3>
    auto http_route_builder(NS s, NP1 p1, NP2 p2, NP3 p3)
    {
      return http_route<V, NS, NP1, NP2, NP3>(s, p1, p2, p3);
    }
    
    // Add a path symbol.
    template <typename T>
    auto path_append(const iod::symbol<T>& s)
    {
      auto new_path = std::tuple_cat(path, std::make_tuple(T()));
      return http_route_builder(new_path, url_params, get_params, post_params);
    }

    // Add a parameter.
    template <typename T, typename U>
    auto path_append(const iod::array_subscript_exp<T, U>& v)
    {
      auto var = typename T::template variable_type<U>(v.member);
      auto new_path = std::tuple_cat(path, std::make_tuple(var));
      auto new_url_params = iod::cat(url_params, iod::D(var));
      return http_route_builder(new_path, new_url_params, get_params, post_params);
    }

    template <typename P>
    auto format_params(const P& params)
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
            return x.symbol()(_optional) = std::string(x.value());
          },
          [] (auto x) {
            return x.symbol() = x.value();
          }, m);
      };

    }
    
    // Set post params.
    template <typename P>
    auto set_params(const post_params_t<P>& new_post_params)
    {
      return http_route_builder(path, url_params, get_params, format_params(new_post_params));
    }

    // Set get params.
    template <typename P>
    auto set_params(const get_params_t<P>& new_get_params)
    {
      return http_route_builder(path, url_params, format_params(new_get_params), post_params);
    }

    template <typename NV>
    auto set_verb(const http_verb<NV>&)
    {
      return http_route<NV, S, P1, P2, P3>(path, url_params, get_params, post_params);
    }
    
    auto all_params() const
    {
      return iod::cat(url_params, iod::cat(get_params, post_params));
    }

    auto verb_as_string(http_get) const { return "/GET"; }
    auto verb_as_string(http_post) const { return "/POST"; }
    auto verb_as_string(http_put) const { return "/PUT"; }
    auto verb_as_string(http_delete) const { return "/DELETE"; }
    auto verb_as_string(http_verb_any) const { return "/*"; }
    auto verb_as_string() const { return verb_as_string(verb); }

    auto has_verb(http_verb_any) const { return false; }
    template <typename VV>
    auto has_verb(VV) const { return true; }
    auto has_verb() const { return has_verb(verb); }

    std::string path_as_string(bool with_verb = true) const
    {
      std::string s;
      if (with_verb) s += verb_as_string(verb);
      foreach(path) | [&] (auto e)
      {
        static_if<is_symbol<decltype(e)>::value>(
          [&] (auto e2) { s += std::string("/") + e2.name(); },
          [&] (auto) { s += std::string("/*"); }, e);
      };
      return s;
    }

    V verb;
    S path;
    P1 url_params;
    P2 get_params;
    P3 post_params;
  };
  
  namespace internal
  {

    template <typename B, typename S>
    auto make_http_route(B b, const iod::symbol<S>& s)
    {
      return b.path_append(s);
    }


    template <typename B, typename V>
    auto make_http_route(B b, const http_verb<V>& v)
    {
      return b.set_verb(v);
    } 
   
    template <typename B, typename S, typename V>
    auto make_http_route(B b, const iod::array_subscript_exp<S, V>& v)
    {
      return b.path_append(v);
    }

    template <typename B, typename P>
    auto make_http_route(B b, const get_params_t<P>& new_get_params)
    {
      return b.set_params(new_get_params);
    }

    template <typename B, typename P>
    auto make_http_route(B b, const post_params_t<P>& new_post_params)
    {
      return b.set_params(new_post_params);
    }

    template <typename B, typename L, typename R>
    auto make_http_route(B b, const iod::div_exp<L, R>& r);
    
    template <typename B, typename L, typename R>
    auto make_http_route(B b, const iod::mult_exp<L, R>& r)
    {
      return make_http_route(make_http_route(b, r.lhs), r.rhs);
    }

    template <typename B, typename L, typename R>
    auto make_http_route(B b, const iod::div_exp<L, R>& r)
    {
      return make_http_route(make_http_route(b, r.lhs), r.rhs);
    }
    
  }

  template <typename V, typename S, typename P1,
            typename P2, typename P3>
  auto make_http_route(const http_route<V, S, P1, P2, P3>& r)
  {
    return r;
  }
  
  template <typename E>
  auto make_http_route(const E& exp)
  {
    http_route<> b;
    return internal::make_http_route(b, exp);
  }
  
}
