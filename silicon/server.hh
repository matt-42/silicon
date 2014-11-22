#pragma once

#include <string>
#include <vector>

#include <iod/sio.hh>
#include <iod/sio_utils.hh>
#include <iod/json.hh>
#include <iod/tuple_utils.hh>

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

    //virtual artumentargument_tree(F f);

    std::string name_;
  };

  template <typename M, typename C>
  struct handler : public handler_base<M>
  {
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

    template <typename... T>
    auto operator()(T...)
    {
      auto add_missing_string_value_types = [] (auto u)
      {
        typedef decltype(u) U;
        return static_if<std::is_base_of<symbol<U>, U>::value>(
          [&] (auto& u) { return u = std::string(); },
          [&] (auto& u) { return u; }, u);
      };

      return pre_typed_handler_creator<S, decltype(add_missing_string_value_types(std::declval<T>()))...>
                                                   (s_, name_);
    }
    
    S* s_;
    std::string name_;
  };

  template <typename M>
  struct server
  {
    typedef server<M> self_type;

    server(M&& middlewares) : middlewares_(middlewares)
    {}

    auto operator[](std::string route_path)
    {
      return handler_creator<self_type>(this, route_path);
    }

    template <typename F>
    auto add_procedure(std::string name, F f)
    {
      handlers.push_back(new handler<M, F>(name, f));
      return *this;
    }

    void handle(request& request,
                response& response)
      try
      {
        // get rq metadata.
        auto md = iod::D(s::_Handler_id = int());
        int pos;
        iod::json_decode(md, request.get_body_string(), pos);
        request.set_params_position(pos);

        if (md.handler_id < int(handlers.size()) and md.handler_id >= 0)
          (*handlers[md.handler_id])(middlewares_, request, response); // run the handler.
        else
          throw error::not_found("Procedure id = ", md.handler_id, " does not exist.");
        //throw new http_not_found();
      }
      catch(const error::error& e)
      {
        response.set_status(e.status());
        response << e.what();
      }

    void serve()
    {
      backend_.serve([this] (auto& req, auto& res) { this->handle(req, res); });
    }

    void serve_one()
    {
      //backend_.serve_one(*this);
    }

    const std::vector<handler_base<M>*>& get_handlers() const { return handlers; }

    //private:
    M middlewares_;
    backend backend_;
    std::vector<handler_base<M>*> handlers;
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


  template <typename S, typename... T>
  template <typename C, typename... U>
  void
  pre_typed_handler_creator<S, T...>::run(C content, std::tuple<U...>* a)
  {
    //void* x = *a;
    s_->add_procedure(name_,
                      [&content] (U&&... tail)
                      {
                        return content(std::forward<U>(tail)...);
                      });
  };

}

#include <silicon/run_handler.hh>
