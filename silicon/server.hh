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
  
  template <typename F>
  struct handler_creator
  {
    handler_creator(F f) : f_(f) {}

    template <typename C>
    void operator=(C content);

    template <typename U, bool>
    struct add_missing_string_value_types { typedef U type; };
    template <typename U>
    struct add_missing_string_value_types<U, true> {
      typedef typename U::template variable_type<std::string> type;
    };
    
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
          [&] (auto _this) {
            _this->index_api(f.value(), prefix + f.symbol().name() + std::string("_"));
          },
          [&] (auto _this) {
            iod::apply_members(f.attributes(),
                               (*_this)[prefix + f.symbol().name()])
              = f.value();
          }, this);
      };
    }

    auto operator[](std::string procedure_name)
    {
      auto f = [this,procedure_name] (auto&& f) { this->add_procedure(procedure_name, f); };
      return handler_creator<decltype(f)>(f);
    }

    auto route(std::string route_path)
    {
      auto f = [this,route_path] (auto&& f) { this->add_route(route_path, f); };
      return handler_creator<decltype(f)>(f);
    }
    
    template <typename F>
    auto add_procedure(std::string name, F f)
    {
      handlers_.push_back(new handler<M, F>(name, f));
      return *this;
    }

    template <typename F>
    auto add_route(std::string name, F f)
    {
      routes_[name] = new handler<M, F>(name, f);
      return *this;
    }
    
    void handle(request& request,
                response& response)
      try
      {
        handler_base<M>* h = nullptr;
        const std::string& location = request.location();
        if (location.size() > 3 and
            location[0] == '/' and
            location[1] == '_' and
            location[2] == '_') // Routing with /__handler_id
        {
          int hid = 0;
          unsigned i = 3;
          while (i < location.size() and location[i] >= '0' and location[i] <= '9')
            hid = hid * 10 + location[i++] - '0';
          
          request.set_params_position(0);

          if (hid < int(handlers_.size()) and hid >= 0)
            h = handlers_[hid];
          else
            throw error::not_found("Procedure id = ", hid, " does not exist.");
        }
        else // Classic routing
        {
          auto r = routes_.find(request.location());
          if (r != routes_.end())
            h = r->second;
          else
            throw error::not_found("Route ", request.location(), " does not exist.");
        }

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

    // template <typename... T>
    // response call_procedure(sio<T...>& params)
    // {
    //   // serialize to json.
    //   std::string body = json_encode(params);

    //   // Create a request object.
    //   auto r = request::create_request(body);
    //   // find the handler h.
    //   h(middlewares_, request, response);
    //   return response;
    // }

    const std::vector<handler_base<M>*>& get_handlers() const { return handlers_; }

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
      return augment_server(std::make_tuple(), iod::D(api...));
    }
    
    M middlewares_;
    backend backend_;
    API api_;
    std::vector<handler_base<M>*> handlers_;
    std::map<std::string, handler_base<M>*> routes_;
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
