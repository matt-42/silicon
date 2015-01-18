#pragma once

#include <boost/network/protocol/http/server.hpp>
#include <boost/network/protocol/http/request.hpp>

#include <iod/json.hh>
#include <silicon/error.hh>
#include <silicon/service.hh>

namespace sl
{
  using namespace iod;

  namespace bnh = boost::network::http;

  typedef boost::network::http::tags::http_server cppnet_server_tag;
  typedef boost::network::http::basic_request<cppnet_server_tag> cppnet_request_type;
  typedef boost::network::http::basic_response<cppnet_server_tag> cppnet_response_type;

  // template <typename... O>
  // struct cppnet_session_cookie
  // {
  //   cppnet_session_cookie(O&&... options)
  //     : options_(D(options...))
  //   {
  //   }

  //   std::string generate_new_token()
  //   {
  //     std::ostringstream os;
  //     std::random_device rd;
  //     os << std::hex << rd() << rd() << rd() << rd();
  //     return os.str();
  //   }
    
  //   tracking_cookie instantiate(cppnet_request_type* req, cppnet_response_type* resp)
  //   {
  //     std::string token;
  //     std::string key = "sl_token";
  //     auto* cookie = new mimosa::http::Cookie;

  //     auto& cookies = req->cookies();
  //     auto it = cookies.find(key);
  //     if (it == cookies.end())
  //     {
  //       token = generate_new_token();
  //       cookie->setKey(key);
  //       cookie->setValue(token);
  //       cookie->setSecure(options_.has(_secure));
  //       cookie->setHttpOnly(options_.has(_http_only));
  //       if (options_.has(_expires)) cookie->setExpires(options_.get(_expires, [] () { return ""; })());
  //       cookie->setPath(options_.get(_path, "/"));
  //       resp->addCookie(cookie);
  //     }
  //     else
  //       token = it->second;

  //     return tracking_cookie(token);
  //   }

  //   sio<O...> options_;
  // };
  
  template <typename RQ, typename RS>
  struct cppnet_json_service_utils
  {
    typedef RQ request_type;
    typedef RS response_type;

    template <typename T>
    auto deserialize(const request_type& r, T& res) const
    {
      try
      {
        json_decode(res, r.body);
      }
      catch (const std::runtime_error& e)
      {
        throw error::bad_request("Error when decoding procedure arguments: ", e.what());
      }

    }

    template <typename T>
    auto serialize(response_type& r, const T& res) const
    {
      std::string str = json_encode(res);
      r = response_type::stock_reply(response_type::ok, str);
    }

  };

  template <typename S, typename RQ, typename RS>
  struct cppnet_handler {

    typedef RQ request_type;
    typedef RS response_type;
    
    cppnet_handler(S service) : service_(service) {}

    void operator() (request_type& request,
                     response_type& response)
    {

      try
      {
        service_(std::string(request.destination), request, response);
      }
      catch(const error::error& e)
      {
        auto s = (typename response_type::status_type)e.status();
        response = response_type::stock_reply(s, e.what());
      }
      catch(const std::runtime_error& e)
      {
        std::cerr << e.what() << std::endl;
        response = response_type::stock_reply(response_type::internal_server_error, "Internal server error.");
      }
    }
      
    void log(std::string const &info) {
      std::cerr << "ERROR: " << info << '\n';
    }

  private:
    S service_;
    
  };


  template <typename A>
  void cppnet_json_serve(const A& api, int port)
  {
    namespace http = boost::network::http;

    typedef boost::network::http::basic_request<cppnet_server_tag> RQ;
    typedef boost::network::http::basic_response<cppnet_server_tag> RS;

    auto service = make_service<cppnet_json_service_utils<RQ, RS>>(api);
    typedef decltype(service) S;
    typedef cppnet_handler<S, RQ, RS> H;
    typedef bnh::server<H> server_type;

    H handler(service);
 
    namespace utils = boost::network::utils;
    typename server_type::options options(handler);
    server_type server_(
      options.address("0.0.0.0")
      .thread_pool(boost::make_shared<boost::network::utils::thread_pool>(3))
      .port(boost::lexical_cast<std::string>(port))
      .reuse_address(true));
    server_.run();
  }
  
}
