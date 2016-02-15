#pragma once

#include <type_traits>
#include <iod/sio.hh>
#include <iod/symbol.hh>
#include <iod/sio_utils.hh>
#include <iod/tuple_utils.hh>
#include <iod/callable_traits.hh>
#include <iod/di.hh>
#include <iod/bind_method.hh>
#include <silicon/di_factories.hh>
#include <silicon/http_route.hh>

namespace sl
{
  using namespace iod;
  using iod::D;

  template <typename U, typename G, typename P>
  struct procedure_args
  {
    // return
    auto to_sio() {}
  };

  // template <typename A>
  // auto procedure_arguments_remove_optionals_f()
  // {
  //   return foreach(A()) | [] (auto m)
  //   {
  //     return static_if<is_optional<decltype(m.value())>::value>(
  //       [] (auto m) { return m.symbol() = m.value().value; },
  //       [] (auto m) { return m.symbol() = m.value(); },
  //       m);
  //   };
  // }

  // template <typename A>
  // struct procedure_arguments_remove_optionals
  // {
  //   typedef decltype(procedure_arguments_remove_optionals_f<A>()) type; 
  // };

  // template <typename A>
  // using procedure_arguments_remove_optionals_t = typename procedure_arguments_remove_optionals<A>::type;

  template <typename Ro, typename A, typename Ret, typename F>
  struct procedure
  {
    typedef A arguments_type;
    typedef Ro route_type;
    typedef Ret return_type;
    typedef F function_type;

    typedef typename Ro::path_type path_type;

    procedure(F f, Ro route)
      : f_(f), default_args_(route.all_params()), route_(route)
    {
    }

    auto function() const { return f_; }
    auto default_arguments() const { return default_args_; }
    auto path() const { return route_.path; }

  private:
    F f_;
    arguments_type default_args_;
    Ro route_;
  };
    
  // Bind procedure arguments.
  template <typename F, typename... U>
  auto bind_procedure2(F f, std::tuple<U...>*)
  {
    return [f] (U... args)
       {
         return f(std::forward<U>(args)...);
       };
  }

  // Bind procedure arguments.
  template <typename F, typename... A>
  auto bind_procedure_arguments(F f, sio<A...>)
  {
    typedef sio<A...> params_type;
    //typedef set_default_string_parameter_t<sio<A...>> params_type;
    return bind_procedure2(f, (callable_arguments_tuple_t<decltype(&F::template operator()<params_type>)>*)0);
  }

  template <typename T> struct first_sio_of_tuple;
  template <typename... T> struct first_sio_of_tuple2;

  template <>
  struct first_sio_of_tuple2<>
  {
    typedef sio<> type;
  };
  
  template <typename... U, typename... T>
  struct first_sio_of_tuple2<sio<U...>, T...>
  {
    typedef sio<U...> type;
  };
  template <typename... U, typename... T>
  struct first_sio_of_tuple2<sio<U...>&&, T...>
  {
    typedef sio<U...> type;
  };
  template <typename... U, typename... T>
  struct first_sio_of_tuple2<const sio<U...>&, T...>
  {
    typedef sio<U...> type;
  };

  template <typename U, typename... T>
  struct first_sio_of_tuple2<U, T...> : public first_sio_of_tuple2<T...> {};

  template <typename... T>
  struct first_sio_of_tuple<std::tuple<T...>> : public first_sio_of_tuple2<T...> {};

  template <typename T>
  using first_sio_of_tuple_t = typename first_sio_of_tuple<T>::type;

  template <typename F, typename Ro>
  auto make_procedure2(F fun, Ro route)
  {
    static_assert(is_callable<F>::value, "F must be callable.");

    typedef first_sio_of_tuple_t<std::remove_reference_t<callable_arguments_tuple_t<F>>> A;
    typedef std::remove_reference_t<callable_return_type_t<F>> Ret;
    return procedure<Ro, A, Ret, F>(fun, route);
  }

  // If m.attributes not empty, bind procedure arguments.
  template <typename R, typename F>
  auto make_procedure(std::enable_if_t<!is_callable<decltype(std::declval<F>())>::value>*,
                      R r, F f)
  {
    auto binded_f = bind_procedure_arguments(f, r.all_params());
    return make_procedure2(binded_f, r);
  }

  // If m.value is callable (i.e. not templated), just forward it.
  template <typename R, typename F>
  auto make_procedure(std::enable_if_t<is_callable<decltype(std::declval<F>())>::value>*,
                      R r, F f)
  {
    return make_procedure2(f, r);
  }

  template <typename R, typename P>
  struct api_node
  {
    R route;
    P content;
  };

  template <typename R, typename C>
  auto make_api_node(R route, C content)
  {
    return api_node<R, C>{route, content};
  }

  // // parse_api: Transform the api object into a tree of route / procedures.
  // template <typename R, typename... T>
  // auto parse_api(std::tuple<T...> api, R route)
  // {
  //   return foreach(api) | [&] (auto m) // m should be iod::assign_exp
  //   {
  //     return static_if<is_tuple<decltype(m.right)>::value>(
  //       [&] (auto m) { // If sio, recursion.
  //         auto r = internal::make_http_route(route, m.left);
  //         return make_api_node(r, parse_api(m.right, r));
  //       },
  //       [&] (auto m) { // Else, register the procedure.
  //         auto r = internal::make_http_route(route, m.left);
  //         return make_api_node(r, make_procedure(0, r, m.right));
  //       }, m);
  //   };
  // }

  // parse_api: Transform the api object into a tree of route / procedures.
  template <typename R, typename... T>
  auto prefix_api(R route, std::tuple<T...> api)
  {
    return foreach(api) | [&] (auto m)
    {
      return static_if<is_tuple<decltype(m.content)>::value>(
        [&] (auto m) { // If tuple, plug it under route.
          auto r = route_cat(route, m.route);
          return make_api_node(r, prefix_api(r, m.content));
        },
        [&] (auto m) { // Else, register the procedure.
          auto r = route_cat(route, m.route);
          return make_api_node(r, make_procedure(0, r, m.content.function()));
        }, m);
    };
  }
  
  // parse_api: Transform the api object into a tree of route / procedures.
  template <typename R, typename... T>
  auto parse_api(std::tuple<T...> api, R route)
  {
    return foreach(api) | [&] (auto m) // m should be iod::assign_exp
    {
      return static_if<is_tuple<decltype(m.right)>::value>(
        [&] (auto m) { // If api_node, plug it under route.
          auto r = route_cat(route, m.left);
          return make_api_node(r, prefix_api(r, m.right));
        },
        [&] (auto m) { // Else, register the procedure.
          auto r = route_cat(route, m.left);
          return make_api_node(r, make_procedure(0, r, m.right));
        }, m);
    };
  }
  
  template <typename T, typename M>
  decltype(auto) instantiate_factory(M& middlewares_tuple)
  {
    return di_factories_call([](T t) { return t; },
                             middlewares_tuple);
  }

  template <typename R, typename... P>
  auto api(R root, P... procs)
  {
    return parse_api(std::make_tuple(procs...), root);
  }

  template <typename... P>
  auto http_api(P... procs)
  {
    return parse_api(std::make_tuple(procs...), http_route<>());
  }
  
}
