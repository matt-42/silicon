#pragma once
#include <type_traits>
#include <iod/sio.hh>
#include <iod/symbol.hh>
#include <iod/sio_utils.hh>
#include <iod/tuple_utils.hh>
#include <iod/callable_traits.hh>

namespace sl
{
  using namespace iod;

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
    
    auto& procedures()
    {
      return procedures_;
    }

    auto& middlewares()
    {
      return middlewares_;
    }

    template <typename... X>
    auto bind_middlewares(X... m)
    {
      auto nm = std::tuple_cat(middlewares_, std::make_tuple(m...));
      return api<P, decltype(nm)>(procedures_, nm);
    }

    P procedures_;
    M middlewares_;
  };

  template <typename... P>
  auto make_api(P... procs)
  {
    auto a = parse_api(D(procs...));
    return api<decltype(a), std::tuple<>>(a, std::tuple<>());
  }

}
