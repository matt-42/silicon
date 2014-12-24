#pragma once

#include <string>
#include <vector>

#include <iod/sio.hh>
#include <iod/sio_utils.hh>
#include <iod/json.hh>
#include <iod/tuple_utils.hh>
#include <silicon/procedure_desc.hh>

namespace iod
{

  template <typename M, typename C>
  struct handler;

  namespace error
  {
    struct error
    {
    public:
      error(int status, const std::string& what) : status_(status), what_(what) {}
      error(int status, const char* what) : status_(status), what_(what) {}
      auto status() const { return status_; }
      auto what() const { return what_; }
      
    private:
      int status_;
      std::string what_;
    };


    template <typename E>
    inline void format_error_(E&) {}

    template <typename E, typename T1, typename... T>
    inline void format_error_(E& err, T1 a, T... args)
    {
      err << a;
      format_error_(err, std::forward<T>(args)...);
    }

    template <typename... T>
    inline std::string format_error(T&&... args) {
      std::stringstream ss;
      format_error_(ss, std::forward<T>(args)...);
      return ss.str();
    }
    
#define SILICON_HTTP_ERROR(CODE, ERR)                           \
    template <typename... A> auto ERR(A&&... m) { return error(CODE, format_error(m...)); } \
    auto ERR(const char* w) { return error(CODE, w); } 

    SILICON_HTTP_ERROR(400, bad_request)
    SILICON_HTTP_ERROR(401, unauthorized)
    SILICON_HTTP_ERROR(403, forbidden)
    SILICON_HTTP_ERROR(404, not_found)

    SILICON_HTTP_ERROR(500, internal_server_error)
    SILICON_HTTP_ERROR(501, not_implemented)

#undef SILICON_HTTP_ERROR
  }

  template <typename M>
  struct handler_base
  {
    handler_base(std::string n) : name_(n) {}
    virtual void operator()(M& middlewares, request& request, response& response) = 0;

    virtual procedure_desc description() = 0;

    std::string name_;
  };

  template <typename M, typename C>
  struct handler : public handler_base<M>
  {
    typedef C content_type;
    handler(std::string n, C c) : handler_base<M>(n), content_(c) {}

    virtual void operator()(M& middlewares, request& request, response& response)
    {
      try
      {
        run_handler(*this, middlewares, request, response);
      }
      catch(const error::error& e)
      {
        response.set_status(e.status());
        response << e.what();
      }
      catch(const std::runtime_error& e)
      {
        std::cout << e.what() << std::endl;
        response.set_status(500);
        response << "Internal server error.";
      }

    }

    virtual procedure_desc description()
    {
      return std::move(procedure_desc(*this));
    }
    
    C content_;
  };

  template <typename F, typename... T>
  struct pre_typed_handler_creator
  {
    pre_typed_handler_creator(F f) : f_(f) {}

    template <typename C, typename... U>
    void run(C content, std::tuple<U...>*);

    template <typename C>
    void operator=(C content)
    {
      typedef decltype(D(std::declval<T>()...)) params_type;
      run(content, (callable_arguments_tuple_t<decltype(&C::template operator()<params_type>)>*)0);
    }

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

  template <typename F>
  struct handler_creator
  {
    handler_creator(F f) : f_(f) {}

    template <typename C>
    void operator=(C content);

    template <typename... T>
    auto operator()(T...)
    {
      return static_if<sizeof...(T) == 0>(
        [this] (auto) { return *this; },
        [] (auto f) { 
          return pre_typed_handler_creator<F, typename add_missing_string_value_types<T, std::is_base_of<symbol<T>, T>::value>::type...>
                                                   (f);
        }, f_);
    }
    
    F f_;
  };

  template <typename A, typename R, typename F>
  struct procedure
  {
    //typedef N name;
    typedef A arguments_type;
    typedef R result_type;
    typedef F function_type;

    procedure(F f) : f_(f) {}

    auto function() const {return f_; }

  private:
    F f_;
  };

  // Bind procedure arguments.
  template <typename F, typename... U>
  auto bind_procedure2(F f, std::tuple<U...>*)
  {
    return [f] (U&&... args)
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

  // template <typename M>
  // auto make_procedure(M m, std::enable_if<!is_callable<decltype(m.value())> >* = 0) // If m is a value, wrap it in a lambda
  // {
  //   auto f = [v = m.value()] () { return v; };
  //   return procedure<<sio<>, decltype(m.value()), decltype(f)>(f);
  // }

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
  
  template <typename M = std::tuple<>, typename API = iod::sio<> >
  struct server
  {
    typedef server<M> self_type;

    server() {} // Builds an empty server.
    
    server(M&& middlewares) : middlewares_(middlewares)
    {}

    server(M middlewares, API api)
      : middlewares_(middlewares),
        api_(api)
    {
      index_api(api_);
    }

    template <typename O>
    void index_api(O& o, std::string prefix = "")
    {
      foreach(o) | [this, prefix] (auto& f)
      {
        static_if<is_sio<decltype(f.value())>::value>(
          [&] (auto _this, auto f) { // If sio, recursion.
            _this->index_api(f.value(),
                             prefix + f.symbol().name() + std::string("_"));
          },
          [&] (auto _this, auto f) { // Else, register the procedure.
            typedef decltype(f.value().function()) F;
            std::string name = std::string("/") + prefix + f.symbol().name();
            _this->procedures_[name] =
              new handler<M, F>(f.symbol().name(), f.value().function());
          }, this, f);
      };
    }

    auto route(std::string route_path)
    {
      auto f = [this,route_path] (auto&& f) { this->add_route(route_path, f); };
      return handler_creator<decltype(f)>(f);
    }

    template <typename F>
    auto add_route(std::string name, F f)
    {
      extra_routes_[name] = new handler<M, F>(name, f);
      return *this;
    }
    
    void handle(request& request,
                response& response)
      try
      {
        handler_base<M>* h = nullptr;
        const std::string& location = request.location();

        auto r = procedures_.find(location);
        if (r != procedures_.end())
          h = r->second;
        else
        {
          auto r2 = extra_routes_.find(location);
          if (r2 != extra_routes_.end())
            h = r2->second;
          else
            throw error::not_found("Route ", request.location(), " does not exist.");
        }

        request.set_params_position(0);

        (*h)(middlewares_, request, response);
      }
      catch(const error::error& e)
      {
        response.set_status(e.status());
        response << e.what();
      }

    void serve(int port)
    {
      backend_.serve(port, [this] (auto& req, auto& res) { this->handle(req, res); });
    }

    void stop_after_next_request()
    {
      backend_.stop_after_next_request();
    }

    template <typename M2, typename API2>
    auto augment_server(M2 middlewares, API2 api)
    {
      auto new_middlewares = std::tuple_cat(middlewares_, middlewares);
      auto new_api = iod::cat(api_, api);

      return server<decltype(new_middlewares), decltype(new_api)>
        (new_middlewares, new_api);
    }
    
    template <typename... T>
    auto middlewares(T&&... middlewares)
    {
      return augment_server(std::make_tuple(middlewares...), iod::D());
    }

    template <typename... T>
    auto api(T&&... api)
    {
      auto api2 = parse_api(D(api...));
      return augment_server(std::make_tuple(), api2);
    }

    auto get_api() const
    {
      return api_;
    }

    auto& procedures() { return procedures_; }
    const auto& procedures() const { return procedures_; }

    M middlewares_;
    backend backend_;
    API api_;
    std::map<std::string, handler_base<M>*> procedures_;
    std::map<std::string, handler_base<M>*> extra_routes_;
  };

  server<> silicon;

  template <typename T, typename S>
  T& get_middleware(S& server)
  {
    return tuple_get_by_type<T>(server.middlewares_);
  }
  
  template <typename S>
  template <typename C>
  void handler_creator<S>::operator=(C content)
  {
    f_(content);
  }

  template <typename S, typename... T>
  template <typename C, typename... U>
  void
  pre_typed_handler_creator<S, T...>::run(C content, std::tuple<U...>*)
  {
    f_([content] (U&&... tail)
       {
         return content(std::forward<U>(tail)...);
       });
  };

}

#include <silicon/run_handler.hh>
