#pragma once

#include <string>
#include <vector>

#include <iod/sio.hh>
#include <iod/sio_utils.hh>
#include <iod/json.hh>

iod_define_symbol(handler_id, _Handler_id);

namespace iod
{

  template <typename C>
  struct handler;

  struct http_error
  {
    virtual std::string what() const { return "Http error."; }
  };

  struct http_not_found : public http_error
  {
    virtual std::string what() const { return "Http error not found."; }
  };

  struct handler_base
  {
    handler_base(std::string n) : name_(n) {}
    virtual void operator()(request& request, response& response) = 0;

    //virtual artumentargument_tree(F f);

    std::string name_;
  };

  struct server;
  
  template <typename C>
  struct handler : public handler_base
  {
    handler(std::string n, C c) : handler_base(n), content_(c) {}

    virtual void operator()(request& request, response& response)
    {
      try
      {
        run_handler(*this, request, response);
      }
      catch(const http_error& e)
      {
        response << e.what();
      }
    }

    C content_;
  };
  
  struct handler_creator
  {
    handler_creator(server* s, std::string name) : s_(s), name_(name) {}

    template <typename C>
    void operator=(C content);

    server* s_;
    std::string name_;
  };

  struct server
  {
    server(int argc, char* argv[])
      : backend_(argc, argv)
    {}

    auto operator[](std::string route_path)
    {
      return handler_creator(this, route_path);
    }

    template <typename F>
    auto add_procedure(std::string name, F f)
    {
      handlers.push_back(new handler<F>(name, f));
      return *this;
    }

    void handle(request& request,
                response& response) const
    {
      // read body.
      std::istringstream& body_stream = request.get_body_stream();

      // get rq metadata.
      auto md = iod::D(s::_Handler_id = int());
      iod::json_decode(md, body_stream); // Fixme implement json decode on stream.

      if (md.handler_id < handlers.size() and md.handler_id >= 0)
        (*handlers[md.handler_id])(request, response); // run the handler.
      else
        response << "Http error: 404 not found.";
      //throw new http_not_found();
    }

    void serve()
    {
      backend_.serve([this] (auto& req, auto& res) { this->handle(req, res); });
    }

    void serve_one()
    {
      //backend_.serve_one(*this);
    }

    const std::vector<handler_base*>& get_handlers() const { return handlers; }

  private:
    backend backend_;
    std::vector<handler_base*> handlers;
  };

  
  template <typename C>
  void handler_creator::operator=(C content)
  {
    s_->add_procedure(name_, content);
  }

}

#include <silicon/run_handler.hh>
