#pragma once

#include <type_traits>
#include <iod/sio.hh>
#include <iod/callable_traits.hh>
#include <iod/tuple_utils.hh>

#include <silicon/server.hh>

namespace iod
{

  template <typename M, typename C>
  struct handler;

  // Simple object handler.
  template <typename M, typename F>
  std::enable_if_t<not iod::is_callable<F>::value>
  run_handler(const handler<M, F>& handler,
              M&,
              request&, response& response)
  {
    response << handler.content_;
  }

  // ===============================
  // Helper to build the tuple of arguments that
  // run_handler will apply to the handler.
  // ===============================

  template <typename T, typename C>
  inline decltype(auto) make_handler_argument(T*, C& ctx)
  {
    return tuple_get_by_type<T>(ctx);
  }

  template <typename... ARGS, typename C>
  inline sio<ARGS...> make_handler_argument(sio<ARGS...>*,
                                            C& ctx)
  {
    sio<ARGS...> res;
    try
    {
      int tail_pos = 0;
      iod::json_decode(res, tuple_get_by_type<request>(ctx).get_params_string(), tail_pos);
      tuple_get_by_type<request>(ctx).set_tail_position(tail_pos);
    }
    catch (const std::runtime_error& e)
    {
      throw error::bad_request("Error in procedure arguments: ", e.what());
    }
    return res;
  }

  template <typename M, typename C, typename... T>
  auto call_middleware_instantiate(M& factory, C&& ctx, std::tuple<T...>*)
  {
    return factory.instantiate(tuple_get_by_type<T>(ctx)...);
  }

  template <typename E, typename C, typename... T>
  auto call_middleware_instantiate_static(C&& ctx, std::tuple<T...>*)
  {
    return E::instantiate(tuple_get_by_type<T>(ctx)...);
  }

  template <typename M>
  struct is_middleware_instance
  {
    template <typename X>
    static char test(typename X::middleware_type*);

    template <typename X>
    static int test(...);

    static const int value = (sizeof(test<std::remove_reference_t<M>>(0)) == sizeof(char));
  };

  template <typename M>
  struct has_instantiate_static_method
  {
    template <typename X>
    static char test(decltype(&X::instantiate));

    template <typename X>
    static int test(...);

    static const int value = (sizeof(test<std::remove_reference_t<M>>(0)) == sizeof(char));
  };


  template <typename B>
  struct dependencies_of_
  {
    static auto instantiate()
    {
      return dependencies_of_<B>();
    }
     
    std::tuple<> deps;
  };
  
  template <typename... D>
  struct dependencies_of_<std::tuple<D...>>
  {
    dependencies_of_(D... d) : deps(d...) {}

    static auto instantiate(D&&... deps)
    {
      return dependencies_of_<std::tuple<D...>>(deps...);
    };

    std::tuple<std::remove_reference_t<D>...> deps;
  };

  template <typename... T>
  auto filter_middleware_deps(std::tuple<T...> t)
  {
    return foreach(t) | [] (auto& m) -> decltype(auto) {
      typedef decltype(m) U;
      typedef std::remove_reference_t<decltype(m)> _U;
      return static_if<is_middleware_instance<_U>::value or
                       has_instantiate_static_method<_U>::value>(
                         []() -> U { return *(std::remove_reference_t<U>*)0; },
                         []() {});
    };
  }

  template <typename F>
  using dependencies_of =
    dependencies_of_<decltype(filter_middleware_deps(std::declval<callable_arguments_tuple_t<F>>()))>;
    
  template <typename E, typename F, typename C>
  auto instantiate_middleware(F& factories, C&& ctx);



  template<unsigned N, unsigned SIZE, typename F, typename A, typename P>
  inline
  auto
  tuple_iterate_loop(std::enable_if_t<N == SIZE>*, F, A&&, P&& prev)
  {
    return prev;
  }
  
  template<unsigned N, unsigned SIZE, typename F, typename A, typename P>
  inline
  auto
  tuple_iterate_loop(std::enable_if_t<N < SIZE>*, F f, A&& t, P&& prev)
  {
    return tuple_iterate_loop<N + 1, SIZE>(0, f, t, f(std::get<N>(t), prev));
  }

  template <typename T, typename P>
  struct tuple_iterate_caller
  {
    tuple_iterate_caller(T&& t, P&& prev_init) : t_(t), prev_init_(prev_init) {}

    template <typename F>
    auto operator|(F f)
    {
      const int size = std::tuple_size<std::remove_reference_t<T>>::value;
      return tuple_iterate_loop<0, size>(0, f, t_, prev_init_);
    }

    const T t_;
    P prev_init_;
  };
  
  template <typename T, typename C>
  auto tuple_iterate(T&& t, C&& init)
  {
    return tuple_iterate_caller<T, C>(t, init);
  }
  
  // Instantiate the list middleware from the list of middleware types A...
  // returns the concatenation of the list and ctx.
  template <typename F, typename C, typename... A>
  auto instantiate_middleware_list(F& factories, C&& ctx, std::tuple<A...>*)
  {
    std::tuple<std::remove_reference_t<A>*...> tmp;
    return tuple_iterate(tmp, ctx) | [&] (auto* e, auto& prev) { // Foreach instantiate argument e.
      typedef std::remove_reference_t<std::remove_pointer_t<decltype(e)>> E;
      return static_if<is_middleware_instance<E>::value or
                       has_instantiate_static_method<E>::value>( // if e is a middleware
        // Instantiate it.
        [&] (auto& factories, auto& prev) {
          return instantiate_middleware<E>(factories, prev); },
        // Otherwise, return the previous list of middleware.
        [&] (auto&, auto& prev) { return prev; },
        factories, prev);
    };
  }

  // Instantiate middleware of type E.
  // returns the concatenation of E (if not already in ctx), E's dependencies not already in ctx and ctx.
  template <typename E, typename F, typename C>
  auto instantiate_middleware(F& factories, C&& ctx)
  {

    auto* args  = static_if<has_instantiate_static_method<E>::value>(
      // If the argument type provide a static instantiate method, use it.
      [] (auto&, auto* e) {
        typedef std::remove_pointer_t<decltype(e)> E2;
        typedef iod::callable_arguments_tuple_t<decltype(&E2::instantiate)> ARGS;
        return (ARGS*)0;
      },
      // Otherwise, use the matching factory to instantiate it.
      [] (auto&, auto* e) {
        typedef std::remove_pointer_t<decltype(e)> E2;
        typedef typename E2::middleware_type M;
        typedef iod::callable_arguments_tuple_t<decltype(&M::instantiate)> ARGS;
        return (ARGS*)0;
      },
      ctx, (E*)0);

    // typedef typename E::middleware_type M;
    // typedef iod::callable_arguments_tuple_t<decltype(&M::instantiate)> ARGS;
    
    typedef std::remove_reference_t<std::remove_pointer_t<decltype(args)>> ARGS;
    auto deps = instantiate_middleware_list(factories, ctx, (ARGS*)0);
      
    return static_if<tuple_embeds<decltype(deps), E>::value or
                     tuple_embeds<decltype(deps), E&>::value>(// if middleware E already in ctx.
      [&] (auto&, auto&) { return deps; }, // return ctx (do not instantiate it twice).
      // Otherwise, first instantiate its dependencies and then instantiate E.
      [&] (auto&, auto&) {

        auto new_middleware = static_if<has_instantiate_static_method<E>::value>(
          // If the argument type provide a static instantiate method, use it.
          [&] (auto& deps, auto* e) -> auto {
            typedef std::remove_pointer_t<decltype(e)> E2;
            typedef iod::callable_arguments_tuple_t<decltype(&E2::instantiate)> ARGS;
            return (call_middleware_instantiate_static<E>(deps, (ARGS*)0));
          },
          // Otherwise, use the matching factory to instantiate it.
          [&] (auto& deps, auto* e) -> auto {
            typedef std::remove_pointer_t<decltype(e)> E2;
            typedef typename E2::middleware_type M;
            typedef iod::callable_arguments_tuple_t<decltype(&M::instantiate)> ARGS;
            
            return call_middleware_instantiate(tuple_get_by_type<M>(factories), deps, (ARGS*)0);
          },
          deps, (E*)0);
        
        //void* x = new_middleware;
        return std::tuple_cat(deps, std::make_tuple(new_middleware));
      },
      factories, ctx);
      
  }

  
  template <typename E,typename F, typename D>
  auto instantiate_middleware_with_deps(F& factories, D&& deps)
  {
    return static_if<has_instantiate_static_method<E>::value>(
      // If the argument type provide a static instantiate method, use it.
      [&] (auto&& deps, auto* e) -> auto {
        typedef std::remove_pointer_t<decltype(e)> E2;
        typedef iod::callable_arguments_tuple_t<decltype(&E2::instantiate)> ARGS;
        return (call_middleware_instantiate_static<E>(std::forward<D>(deps), (ARGS*)0));
      },
      // Otherwise, use the matching factory to instantiate it.
      [&] (auto&& deps, auto* e) -> auto {
        typedef std::remove_pointer_t<decltype(e)> E2;
        typedef typename E2::middleware_type M;
        typedef iod::callable_arguments_tuple_t<decltype(&M::instantiate)> ARGS;
        return call_middleware_instantiate(tuple_get_by_type<M>(factories), std::forward<D>(deps), (ARGS*)0);
      },
      std::forward<D>(deps), (E*)0);
  }
  
  template <typename F, typename... T, typename... U>
  decltype(auto) create_handler_ctx(std::enable_if_t<(sizeof...(T) > sizeof...(U))>*,
                          std::tuple<T...>* ctx, F& factories, U&&... args)
  {
    return create_handler_ctx(0, ctx, factories, args...,
                              std::move(instantiate_middleware_with_deps<std::tuple_element_t<sizeof...(U), std::tuple<T...>>>
                                        (factories, std::forward_as_tuple(args...)))
      );
  };
  
  template <typename F, typename... T, typename... U>
  decltype(auto) create_handler_ctx(std::enable_if_t<sizeof...(T) == sizeof...(U)>*,
                          std::tuple<T...>*, F&, U&&... args)
  {
    return std::tuple<T...>(std::forward<U>(args)...);
  };

  template <typename... T, typename M, typename F>
  auto apply_handler_arguments(M& middleware_factories, request& request_, response& response_,
                               F& f, std::tuple<T...>* params)
  {
    auto ctx = std::forward_as_tuple(request_, response_);

    typedef decltype(instantiate_middleware_list(middleware_factories, ctx, params)) ctx2_type;
    ctx2_type ctx2 = create_handler_ctx(0, (ctx2_type*)0, middleware_factories, request_, response_);

    return f(make_handler_argument((std::remove_reference_t<T>*)0, ctx2)...);
  }

  template <typename F, typename... A, typename... B>
  auto call_with_di2(F fun, std::tuple<A...>*, B&&... args)
  {
    return fun(tuple_get_by_type<A>(std::forward_as_tuple(args...))...);
  }

  static const struct call_with_di_t
  {
    call_with_di_t() {}
    template <typename F, typename... A>
    auto operator()(F fun, A&&... args) const
    {
      return call_with_di2(fun, (callable_arguments_tuple_t<F>*)0, std::forward<A>(args)...);
    }
  } call_with_di;
  
  // Callable handlers.
  template <typename M, typename F>
  std::enable_if_t<iod::is_callable<F>::value>
  run_handler(const handler<M, F>& handler,
              M& middlewares,
              request& request_,
              response& response_)
  {
    iod::static_if<callable_traits<F>::arity == 0>(
      [&] (auto& handler) {
        // No argument. Send the result of the handler.
        response_ << handler.content_();
      },
      [&] (auto& handler) {
        // make argument tuple.
        typedef iod::callable_arguments_tuple_t<F> T;

        iod::static_if<tuple_embeds<T, response&>::value>( // If handler take response as argument.

          // handlers taking response as argument are responsible of writing the response, 
          [&] (auto& handler, auto& request_, auto& response_) {
            apply_handler_arguments(middlewares, request_, response_, handler.content_, (T*)0);
          },

          // automatic serialization of return values of handlers not taking response as argument.
          [&] (auto& handler, auto& request_, auto& response_) {
            static_if<std::is_same<callable_return_type_t<F>, void>::value>(
              [&] (auto& response_) {
                apply_handler_arguments(middlewares, request_, response_, handler.content_, (T*)0);
              },
              [&] (auto& response_) {
                response_ << apply_handler_arguments(middlewares, request_, response_, handler.content_, (T*)0);
              }, response_);
          },
          handler, request_, response_);
      }, handler
      );
  }

}
