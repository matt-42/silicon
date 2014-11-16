#pragma once

#include <string>
#include <vector>

#include <iod/sio.hh>
#include <iod/sio_utils.hh>
#include <iod/json.hh>

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

  template <typename S>
  struct handler_creator
  {
    handler_creator(S* s, std::string name) : s_(s), name_(name) {}

    template <typename C>
    void operator=(C content);

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

}

#include <silicon/run_handler.hh>
