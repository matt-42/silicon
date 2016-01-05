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
  
  template <typename Ro, typename A, typename Ret, typename F>
  struct procedure
  {
    typedef A arguments_type;
    typedef Ro route_type;
    typedef Ret return_type;
    typedef F function_type;

    typedef typename Ro::get_parameters_type get_arguments_type;
    typedef typename Ro::post_parameters_type post_arguments_type;
    typedef typename Ro::path_type path_type;

    procedure(F f, Ro route)
      : f_(f), default_args_(route.all_params()), route_(route)
    {}

    auto function() const { return f_; }
    auto default_arguments() const { return default_args_; }
    auto path() const { return route_.path; }

  private:
    F f_;
    A default_args_;
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

  // parse_api: Transform the api object into a tree of route / procedures.
  template <typename... T>
  auto parse_api(std::tuple<T...> api)
  {
    return foreach(api) | [] (auto m) // m should be iod::assign_exp
    {
      return static_if<is_tuple<decltype(m.right)>::value>(
        [] (auto m) { // If sio, recursion.
          return make_api_node(make_http_route(m.left), parse_api(m.right));
        },
        [] (auto m) { // Else, register the procedure.
          return make_api_node(make_http_route(m.left), make_procedure(0, make_http_route(m.left), m.right));
        }, m);
    };
  }
  
  template <typename R, typename P, typename... PA, typename... GA>
  auto apply_global_middleware2(R route, P proc, std::tuple<PA...>*, std::tuple<GA...>*)
  {
    return make_procedure(0, route, [=] (PA&&... pa, const GA&... ga)
    {
      return proc.function()(pa...);
    });
  }

  template <typename R, typename P, typename F>
  auto apply_global_middleware(R route, P proc, F m_tuple)
  {
    return apply_global_middleware2(route, proc,
                                    (callable_arguments_tuple_t<typename P::function_type>*)0,
                                    m_tuple);
  }

  template <typename P, typename F>
  auto apply_global_middleware_rec(P procedures, F m_tuple)
  {
    return foreach(procedures) | [&] (auto m)
    {
      return static_if<is_sio<decltype(m.content)>::value>(
        [&] (auto m) { // If sio, recursion.
          return make_api_node(m.route, apply_global_middleware_rec(m.content, m_tuple));
          //return m.symbol() = apply_global_middleware_rec(m.content, m_tuple);
        },
        [&] (auto m) { // Else, register the procedure.
          return make_api_node(m.route, apply_global_middleware(m.route, m.content, m_tuple));
          // return m.route = make_procedure(0, m.route,
          //                                 apply_global_middleware(m.content, m_tuple));
        }, m);
    };
  }

  template <typename P, typename M>
  struct api
  {
    typedef M middlewares_type;
    typedef P procedures_type;

    api(const P& procs,
        const M& middlewares)
      : procedures_(procs),
        middlewares_(middlewares)
    {
    }
    
    const auto& procedures() const
    {
      return procedures_;
    }

    auto& middlewares()
    {
      return middlewares_;
    }

    template <typename... X>
    auto bind_factories(X... m) const
    {
      auto nm = std::tuple_cat(middlewares_, std::make_tuple(m...));
      return api<P, decltype(nm)>(procedures_, nm);
    }

    template <typename... F>
    auto global_middlewares() const
    {
      auto new_procs = apply_global_middleware_rec(procedures_, (std::tuple<F...>*)0);
      return api<decltype(new_procs), M>(new_procs, middlewares_);
    }
    
    template <typename N>
    struct has_initialize_method
    {
      template <typename X>
      static char test(decltype(&X::initialize));

      template <typename X>
      static int test(...);

      static const int value = (sizeof(test<std::remove_reference_t<N>>(0)) == sizeof(char));
    };

    template <typename T>
    auto instantiate_factory()
    {
      return di_factories_call([] (T t) { return t; }, middlewares_);
    }

    auto initialize_factories()
    {
      foreach(middlewares_) | [this] (auto& m)
      {
        typedef std::remove_reference_t<decltype(m)> X;
        static_if<has_initialize_method<X>::value>(
          [this] (auto& m) {
            typedef std::remove_reference_t<decltype(m)> X_;
            di_factories_call(bind_method(m, &X_::initialize), middlewares_);
          },
          [] (auto& m) {}, m);
      };
    }

    P procedures_;
    M middlewares_;
  };

  template <typename T, typename A>
  auto instantiate_factory(A& api)
  {
    return api.template instantiate_factory<T>();
  }

  inline auto make_api()
  {
    auto a = sio<>();
    return api<decltype(a), std::tuple<>>(a, std::tuple<>());
  }
  
  template <typename... P>
  auto make_api(P... procs)
  {
    auto a = parse_api(std::make_tuple(procs...));
    return api<decltype(a), std::tuple<>>(a, std::tuple<>());
  }

}
