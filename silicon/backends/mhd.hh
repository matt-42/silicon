#pragma once

#include <thread>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <silicon/symbols.hh>
#include <silicon/error.hh>
#include <silicon/service.hh>
#include <silicon/response.hh>
#include <silicon/middlewares/tracking_cookie.hh>
#include <silicon/middlewares/get_parameters.hh>
#include <iod/json.hh>

namespace sl
{

  struct mhd_request
  {
    mhd_request(MHD_Connection* con, const std::string b) : connection(con), body(b) {}

    MHD_Connection* connection;
    std::string body;
  };

  struct mhd_response
  {
    int status;
    std::string body;
    std::vector<std::pair<std::string, std::string>> cookies;
    std::vector<std::pair<std::string, std::string>> headers;
  };
    
  struct mhd_json_service_utils
  {
    typedef mhd_request request_type;
    typedef mhd_response response_type;

    template <typename T>
    auto deserialize(request_type* r, T& res) const
    {
      try
      {
        json_decode(res, r->body);
      }
      catch (const std::runtime_error& e)
      {
        throw error::bad_request("Error when decoding procedure arguments: ", e.what());
      }

    }

    void serialize2(response_type* r, const std::string res) const
    {
      r->status = 200;
      r->body = res;
    }

    void serialize2(response_type* r, const char* res) const
    {
      r->status = 200;
      r->body = res;
    }
    
    template <typename T>
    auto serialize2(response_type* r, const T& res) const
    {
      std::string str = json_encode(res);
      r->status = 200;
      r->body = str;      
    }

    template <typename T>
    auto serialize(response_type* r, const T& res) const
    {
      serialize2(r, res);
    }

    template <typename T>
    auto serialize2(response_type* r, const response_<T>& res) const
    {
      serialize2(r, res.body);
      if (res.has(_content_type))
        r->headers.push_back(std::make_pair("Content-Type", res.get(_content_type, "")));
    }
    
  };

  template <typename F>
  int mhd_keyvalue_iterator(void *cls,
                            enum MHD_ValueKind kind,
                            const char *key, const char *value)
  {
    F& f = *(F*)cls;
    f(key, value);
    return MHD_YES;
  }

  struct mhd_session_cookie
  {
    
    inline tracking_cookie instantiate(mhd_request* req, mhd_response* resp)
    {
      std::string key = "sl_token";
      std::string token;
      const char* token_ = MHD_lookup_connection_value(req->connection, MHD_COOKIE_KIND, key.c_str());
      
      if (!token_)
      {
        token = generate_secret_tracking_id();
        resp->cookies.push_back(std::make_pair(key, token));
      }
      else
        token = token_;

      return tracking_cookie(token);
    }

  };

  struct mhd_get_parameters_factory
  {
    auto instantiate(MHD_Connection* connection)
    {
      get_parameters map;
      auto add = [&] (const char* k, const char* v) { map[k] = v; };
      MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND,
				&mhd_keyvalue_iterator<decltype(add)>,
				&add);
      return std::move(map);
    }
  };
  
  template <typename S>
  int mhd_handler(void * cls,
                  struct MHD_Connection * connection,
                  const char * url,
                  const char * method,
                  const char * version,
                  const char * upload_data,
                  size_t * upload_data_size,
                  void ** ptr)
  {
    static int dummy;
    MHD_Response* response;
    int ret;

    std::string* pp = (std::string*)*ptr;
    if (!pp)
    {
      pp = new std::string;
      *ptr = pp;
      return MHD_YES;
    }
    if (*upload_data_size)
    {
      pp->append(std::string(upload_data, *upload_data_size));
      *upload_data_size = 0;
      return MHD_YES;
    }

    auto& service = * (S*)cls;

    mhd_request rq(connection, *pp);
    mhd_response resp;

    try
    {
      service(url, &rq, &resp, connection);
    }
    catch(const error::error& e)
    {
      resp.status = e.status();
      std::string m = e.what();
      resp.body = m.data();
    }
    catch(const std::runtime_error& e)
    {
      std::cout << e.what() << std::endl;
      resp.status = 500;
      resp.body = "Internal server error.";
    }

    const std::string& str = resp.body;
    response = MHD_create_response_from_data(str.size(),
                                             (void*) str.c_str(),
                                             MHD_NO,
                                             MHD_YES);

    for(auto kv : resp.headers)
    {
      if (MHD_NO == MHD_add_response_header (response,
                                             kv.first.c_str(),
                                             kv.second.c_str()))
        throw error::internal_server_error("Failed to set http header");
    }

    // Set cookies.
    for(auto kv : resp.cookies)
    {
      std::string set_cookie_string = kv.first + '=' + kv.second;
      if (MHD_NO == MHD_add_response_header (response,
                                             MHD_HTTP_HEADER_SET_COOKIE,
                                             set_cookie_string.c_str()))
        throw error::internal_server_error("Failed to set session cookie header");
    }
    
    MHD_add_response_header (response,
                             MHD_HTTP_HEADER_SERVER,
                             "silicon");

    ret = MHD_queue_response(connection,
                             resp.status,
                             response);
    MHD_destroy_response(response);
    delete pp;
    return ret;
  }

  template <typename A, typename... O>
  void mhd_json_serve(const A& api, int port, O&&... opts)
  {

    int flags = MHD_USE_SELECT_INTERNALLY;
    auto options = D(opts...);
    if (options.has(_one_thread_per_connection))
      flags = MHD_USE_THREAD_PER_CONNECTION;
    else if (options.has(_select))
      flags = MHD_USE_SELECT_INTERNALLY;
    else if (options.has(_linux_epoll))
      flags = MHD_USE_EPOLL_INTERNALLY_LINUX_ONLY;

    int thread_pool_size = options.get(_nthreads, std::thread::hardware_concurrency();

    auto api2 = api.bind_factories(mhd_session_cookie(), mhd_get_parameters_factory());
    auto s = service<mhd_json_service_utils, decltype(api2), mhd_request*, mhd_response*, MHD_Connection*>(api2);
    typedef decltype(s) S;
      
    struct MHD_Daemon * d;

    if (flags != MHD_USE_THREAD_PER_CONNECTION)
      d = MHD_start_daemon(
        flags,
        port,
        NULL,
        NULL,
        &mhd_handler<S>,
        &s,
        MHD_OPTION_THREAD_POOL_SIZE, std::thread::hardware_concurrency(),
        MHD_OPTION_END);
    else
      d = MHD_start_daemon(
        flags,
        port,
        NULL,
        NULL,
        &mhd_handler<S>,
        &s,
        MHD_OPTION_END);
      
    
    if (d == NULL)
      return;

    while (getc (stdin) != 'q');
    MHD_stop_daemon(d);
  }
  

}
