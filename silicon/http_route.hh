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
  auto post_parameters(sio<P...> o) {
    return post_params_t<sio<P...>>(o);
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

  
  template <typename T> struct http_verb : public T, public iod::assignable<http_verb<T>>, public iod::Exp<http_verb<T>> {
    using iod::assignable<http_verb<T>>::operator=;
  };
  struct http_get { const char* to_string() { return "GET"; } }; static http_verb<http_get> GET;
  struct http_post { const char* to_string() { return "POST"; } };   static http_verb<http_post> POST;
  struct http_put { const char* to_string() { return "PUT"; } };    static http_verb<http_put> PUT;
  struct http_delete { const char* to_string() { return "DELETE"; } }; static http_verb<http_delete> DELETE;
  struct http_verb_any {  const char* to_string() { return "GET"; } }; static http_verb<http_verb_any> ANY;

  auto http_verb_to_symbol(http_get) { return _http_get; }
  auto http_verb_to_symbol(http_post) { return _http_post; }
  auto http_verb_to_symbol(http_put) { return _http_put; }
  auto http_verb_to_symbol(http_delete) { return _http_delete; }
  
  template <typename V = http_verb_any,
            typename S = std::tuple<>,
            typename P1 = iod::sio<>,
            typename P2 = iod::sio<>,
            typename P3 = iod::sio<>>
  struct http_route
  {
    typedef http_route<V, S, P1, P2, P3> self;
    typedef S path_type;
    typedef P1 url_parameters_type;
    typedef P2 get_parameters_type;
    typedef P3 post_parameters_type;
    typedef decltype(iod::cat(std::declval<P1>(),
                              iod::cat(std::declval<P2>(), std::declval<P3>())))
      parameters_type;
    
    http_route() {}
    http_route(P1 p1, P2 p2, P3 p3)
      : url_params(p1),
        get_params(p2),
        post_params(p3)
    {}

    template <typename NS, typename NP1, typename NP2, typename NP3>
    auto http_route_builder(NS s, NP1 p1, NP2 p2, NP3 p3) const
    {
      return http_route<V, NS, NP1, NP2, NP3>(p1, p2, p3);
    }

    template <typename OV, typename OS, typename OP1, typename OP2, typename OP3>
    auto append(const http_route<OV, OS, OP1, OP2, OP3>& o) const
    {
      auto verb = static_if<std::is_same<http_verb_any, OV>::value>(
        [] () { return http_verb<V>(); },
        [] () { return http_verb<OV>(); });
        
      return http_route_builder(std::tuple_cat(path, OS()),
                                  iod::cat(url_params, o.url_params),
                                  iod::cat(get_params, o.get_params),
                                  iod::cat(post_params, o.post_params)).set_verb(verb);
    }

    // Add a path symbol.
    template <typename T>
    auto path_append(const iod::symbol<T>& s) const
    {
      auto new_path = std::tuple_cat(path, std::make_tuple(T()));
      return http_route_builder(new_path, url_params, get_params, post_params);
    }

    // Add a parameter.
    template <typename T, typename U>
    auto path_append(const iod::array_subscript_exp<T, U>& v) const
    {
      auto var = typename T::template variable_type<U>(v.member);
      auto new_path = std::tuple_cat(path, std::make_tuple(var));
      auto new_url_params = iod::cat(url_params, iod::D(var));
      return http_route_builder(new_path, new_url_params, get_params, post_params);
    }

    template <typename P>
    auto format_params(const P& params) const
    {
      // Forward default values and set default string parameters.
      auto params2 = foreach(params.params) | [&] (auto& m) {
        return static_if<!iod::is_symbol<decltype(m)>::value>(
          [] (auto m) {
            return m.symbol() = std::move(m.value());
          },
          [] (auto m) {
            return m = std::string();
          }, m);
      };
      
      // Set the optional tag for the iod deserializer and default values.
      return foreach(params2) | [&] (auto& m) {
        return static_if<is_optional<decltype(m.value())>::value>(
          [] (auto m) {
            return m.symbol()(_optional) = std::move(m.value().value);
          },
          [] (auto m) {
            return m.symbol() = std::move(m.value());
          }, m);
      };

    }
    
    // Set post params.
    template <typename P>
    auto set_params(const post_params_t<P>& new_post_params) const
    {
      return http_route_builder(path, url_params, get_params, format_params(new_post_params));
    }

    // Set get params.
    template <typename P>
    auto set_params(const get_params_t<P>& new_get_params) const
    {
      return http_route_builder(path, url_params, format_params(new_get_params), post_params);
    }

    template <typename NV>
    auto set_verb(const http_verb<NV>&) const
    {
      return http_route<NV, S, P1, P2, P3>(url_params, get_params, post_params);
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

    std::string string_id() const
    {
      return path_as_string();
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

  template <typename... R1, typename... R2>
  auto route_cat(const http_route<R1...>& r1,
                 const http_route<R2...>& r2)
  {
    return r1.append(r2);
  }
  
  template <typename... R1, typename R2>
  auto route_cat(const http_route<R1...>& r1,
                 const R2& r2)
  {
    return internal::make_http_route(r1, r2);
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
