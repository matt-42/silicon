#pragma once

#include <string>
#include <vector>

#include <iod/sio.hh>
#include <iod/sio_utils.hh>
#include <iod/json.hh>
#include <iod/tuple_utils.hh>
#include <silicon/procedure_desc.hh>

iod_define_symbol(handler_id, _Handler_id);

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
    inline void format_error_(E& err) {}

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

  template <typename S, typename... T>
  struct pre_typed_handler_creator
  {
    pre_typed_handler_creator(S* s, std::string name) : s_(s), name_(name) {}

    template <typename C, typename... U>
    void run(C content, std::tuple<U...>*);

    template <typename C>
    void operator=(C content)
    {
      typedef decltype(D(std::declval<T>()...)) params_type;
      run(content, (callable_arguments_tuple_t<decltype(&C::template operator()<params_type>)>*)0);
    }

    S* s_;
    std::string name_;
  };
  
  template <typename S>
  struct handler_creator
  {
    handler_creator(S* s, std::string name) : s_(s), name_(name) {}

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
      return pre_typed_handler_creator<S, typename add_missing_string_value_types<T, std::is_base_of<symbol<T>, T>::value>::type...>
                                                   (s_, name_);
    }
    
    S* s_;
    std::string name_;
  };

  template <typename S>
  struct route_handler_creator
  {
    route_handler_creator(S* s, std::string route) : s_(s), route_(route) {}

    template <typename C>
    void operator=(C content);
    
    S* s_;
    std::string route_;
  };
  
  template <typename M>
  struct server
  {
    typedef server<M> self_type;

    server(M&& middlewares) : middlewares_(middlewares)
    {}

    auto operator[](std::string procedure_name)
    {
      return handler_creator<self_type>(this, procedure_name);
    }


    auto route(std::string route_path)
    {
      return route_handler_creator<self_type>(this, route_path);
    }
    
    template <typename F>
    auto add_procedure(std::string name, F f)
    {
      handlers.push_back(new handler<M, F>(name, f));
      return *this;
    }

    template <typename F>
    auto add_route(std::string name, F f)
    {
      routes[name] = new handler<M, F>(name, f);
      return *this;
    }
    
    void handle(request& request,
                response& response)
      try
      {
        handler_base<M>* h = nullptr;
        
        if (request.location() == "/") // Routing with handler_id
        {
          // get rq metadata.
          auto md = iod::D(s::_Handler_id = int());
          int pos;
          iod::json_decode(md, request.get_body_string(), pos);
          request.set_params_position(pos);

          if (md.handler_id < int(handlers.size()) and md.handler_id >= 0)
            h = handlers[md.handler_id];
          else
            throw error::not_found("Procedure id = ", md.handler_id, " does not exist.");
        }
        else // Classing routing
        {
          auto r = routes.find(request.location());
          if (r != routes.end())
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

    const std::vector<handler_base<M>*>& get_handlers() const { return handlers; }

    M middlewares_;
    backend backend_;
    std::vector<handler_base<M>*> handlers;
    std::map<std::string, handler_base<M>*> routes;
  };

  template <typename T, typename S>
  T& get_middleware(S& server)
  {
    return tuple_get_by_type<T>(server.middlewares_);
  }

  template <typename... M>
  auto silicon(M&&... middlewares)
  {
    return server<decltype(std::make_tuple(middlewares...))>
      (std::make_tuple(middlewares...));
  }
  
  template <typename S>
  template <typename C>
  void handler_creator<S>::operator=(C content)
  {
    s_->add_procedure(name_, content);
  }

  template <typename S>
  template <typename C>
  void route_handler_creator<S>::operator=(C content)
  {
    s_->add_route(route_, content);
  }

  template <typename S, typename... T>
  template <typename C, typename... U>
  void
  pre_typed_handler_creator<S, T...>::run(C content, std::tuple<U...>* a)
  {
    s_->add_procedure(name_,
                      [&content] (U&&... tail)
                      {
                        return content(std::forward<U>(tail)...);
                      });
  };

}

#include <silicon/run_handler.hh>
