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

namespace sl
{
  using namespace iod;
  using iod::D;

  template <typename A, typename R, typename F>
  struct procedure
  {
    typedef A arguments_type;
    typedef R return_type;
    typedef F function_type;

    procedure(F f) : f_(f) {}

    auto function() const {return f_; }

  private:
    F f_;
  };
  
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
  struct set_default_string_parameter<sio<T...>>
  {
    typedef sio<typename add_missing_string_value_types<T, std::is_base_of<symbol<T>, T>::value>::type...> type;
  };
  template <typename T>
  using set_default_string_parameter_t = typename set_default_string_parameter<T>::type;
  
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
    typedef set_default_string_parameter_t<sio<A...>> params_type;
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

  template <typename S, typename F>
  auto make_procedure(void*, S, F fun)
  {
    static_assert(is_callable<F>::value, "F must be callable.");

    typedef first_sio_of_tuple_t<std::remove_reference_t<callable_arguments_tuple_t<F>>> A;
    typedef std::remove_reference_t<callable_return_type_t<F>> R;
    return procedure<A, R, F>(fun);
  }

  // If m.attributes not empty, bind procedure arguments.
  template <typename M>
  auto make_procedure(std::enable_if_t<!is_callable<decltype(std::declval<M>().value())>::value>*, M m)
  {
    auto f = bind_procedure_arguments(m.value(), m.attributes());
    return make_procedure(0, m.symbol(), f);
  }

  // If m.value is callable (i.e. not templated), just forward it.
  template <typename M>
  auto make_procedure(std::enable_if_t<is_callable<decltype(std::declval<M>().value())>::value >*, M m)
  {
    return make_procedure(0, m.symbol(), m.value());
  }

  template <typename... T>
  auto api_set_default_string_parameters(sio<T...> api)
  {
    return foreach(api) | [] (auto m)
    {
      set_default_string_parameter_t<decltype(m.attributes())> args;
      return apply(args, m.symbol()) = m.value();
    };
  }

  template <typename... T>
  auto parse_api(sio<T...> api)
  {
    return foreach(api) | [] (auto m)
    {
      return static_if<is_sio<decltype(m.value())>::value>(
        [] (auto m) { // If sio, recursion.
          return m.symbol() = parse_api(m.value());
        },
        [] (auto m) { // Else, register the procedure.
          return m.symbol() = make_procedure(0, m);
        }, m);
    };
  }

  template <typename P, typename... PA, typename... GA>
  auto apply_global_middleware2(P proc, std::tuple<PA...>*, std::tuple<GA...>*)
  {
    return [=] (PA&&... pa, GA... ga)
    {
      return proc(pa...);
    };
  }

  template <typename P, typename F>
  auto apply_global_middleware(P proc, F m_tuple)
  {
    return apply_global_middleware2(proc, (callable_arguments_tuple_t<P>*)0,
                                    m_tuple);
  }

  template <typename P, typename F>
  auto apply_global_middleware_rec(P procedures, F m_tuple)
  {
    return foreach(procedures) | [&] (auto m)
    {
      return static_if<is_sio<decltype(m.value())>::value>(
        [&] (auto m) { // If sio, recursion.
          return m.symbol() = apply_global_middleware_rec(m.value(), m_tuple);
        },
        [&] (auto m) { // Else, register the procedure.
          return m.symbol() = make_procedure(0, m.symbol(),
                                             apply_global_middleware(m.value().function(), m_tuple));
        }, m);
    };
  }

  template <typename P, typename M>
  struct api
  {
    typedef M middlewares_type;

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
    auto a = parse_api(D(procs...));
    return api<decltype(a), std::tuple<>>(a, std::tuple<>());
  }

}
