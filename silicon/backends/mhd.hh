#pragma once

#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <thread>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iod/json.hh>
#include <iod/utils.hh>

#include <silicon/file.hh>
#include <silicon/symbols.hh>
#include <silicon/error.hh>
#include <silicon/service.hh>
#include <silicon/response.hh>
#include <silicon/optional.hh>
#include <silicon/middlewares/tracking_cookie.hh>
#include <silicon/middlewares/get_parameters.hh>

namespace sl
{
  using namespace iod;
  
  struct mhd_request
  {
    inline const char* get_header(const char* k)
    {
      return MHD_lookup_connection_value(connection, MHD_HEADER_KIND, k);
    }

    MHD_Connection* connection;
    std::string body;
    std::string url;
  };

  struct mhd_response
  {
    inline mhd_response() : file_descriptor(-1) {}
    inline void set_header(std::string k, std::string v) { headers[k] = v; }
    inline void set_cookie(std::string k, std::string v) { cookies[k] = v; }

    int status;
    std::string body;
    int file_descriptor;
    std::unordered_map<std::string, std::string> cookies;
    std::unordered_map<std::string, std::string> headers;
  };

  template <typename F>
  int mhd_keyvalue_iterator(void *cls,
                            enum MHD_ValueKind kind,
                            const char *key, const char *value)
  {
    F& f = *(F*)cls;
    if (key and value and *key and *value)
    {
      f(key, value);
      return MHD_YES;
    }
    //else return MHD_NO;
      return MHD_YES;
  }

  struct mhd_json_service_utils
  {
    typedef mhd_request request_type;
    typedef mhd_response response_type;

    template <typename S, typename O, typename C>
    void decode_get_arguments(O& res, C* req) const
    {
      std::map<std::string, std::string> map;
      auto add = [&] (const char* k, const char* v) { map[k] = v; };
      MHD_get_connection_values(req->connection, MHD_GET_ARGUMENT_KIND,
                                &mhd_keyvalue_iterator<decltype(add)>,
                                &add);

      foreach(S()) | [&] (auto m)
      {
        auto it = map.find(m.symbol().name());
        if (it != map.end())
        {
          try
          {
            res[m.symbol()] = boost::lexical_cast<std::decay_t<decltype(res[m.symbol()])>>(it->second);
          }
          catch (const std::exception& e)
          {
            throw error::bad_request(std::string("Error while decoding the GET parameter ") +
                                     m.symbol().name() + ": " + e.what());
          }
        }
        else
        {
          if(!m.attributes().has(_optional))
            throw std::runtime_error(std::string("Missing required GET parameter ") + m.symbol().name());
        }
      };
    }

    template <typename P, typename O, typename C>
    void decode_url_arguments(O& res, const C& url) const
    {
      if (!url[0])
        throw std::runtime_error("Cannot parse url arguments, empty url");

      int c = 0;

      P path;

      foreach(P()) | [&] (auto m)
      {
        c++;
        iod::static_if<is_symbol<decltype(m)>::value>(
          [&] (auto m2) {
            while (url[c] and url[c] != '/') c++;
          }, // skip.
          [&] (auto m2) {
            int s = c;
            while (url[c] and url[c] != '/') c++;
            if (s == c)
              throw std::runtime_error(std::string("Missing url parameter ") + m2.symbol_name());

            try
            {
              res[m2.symbol()] = boost::lexical_cast<std::decay_t<decltype(m2.value())>>
              (std::string(&url[s], c - s));
            }
            catch (const std::exception& e)
            {
              throw error::bad_request(std::string("Error while decoding the url parameter ") +
                                       m2.symbol().name() + " with value " + std::string(&url[s], c - s) + ": " + e.what());
            }
            
          }, m);
      };
    }

    template <typename P, typename O>
    void decode_post_parameters_json(O& res, mhd_request* r) const
    {
      try
      {
        if (r->body.size())
          json_decode(res, r->body);
        else
          json_decode(res, "{}");
      }
      catch (const std::exception& e)
      {
        throw error::bad_request("Error when decoding procedure arguments: ", e.what());
      }
    }

    template <typename T, typename... D>
    void decode_post_parameter_urlencoded(sio<D...>*, T& res, const std::string& s) const
    {
      json_decode<sio<D...>>(res, s);
    }

    template <typename T>
    void decode_post_parameter_urlencoded(json_string*, T& res, const std::string& s) const
    {
      json_decode<json_string>(res, s);
    }
    
    template <typename T, typename U>
    void decode_post_parameter_urlencoded(T*, U& res, const std::string& s) const
    {
      res = boost::lexical_cast<T>(s);
    }

    template <typename P, typename O>
    void decode_post_parameters_urlencoded(O& res, mhd_request* r) const
    {
      std::map<std::string, std::string> map;

      const std::string& body = r->body;
      int last = 0;
      for (unsigned int i = 0; i < body.size();)
      {
        std::string key, value;

        for (; i < body.size() and body[i] != '='; i++);

        key = std::string(&body[last], i - last);

        i++; // skip =

        if (i >= body.size()) break;

        last = i;

        for (; i < body.size() and body[i] != '&'; i++);

        value = std::string(&body[last], i - last);
        value.resize(MHD_http_unescape(&value[0]));

        map.insert(std::make_pair(key, value));

        i++; // skip &;
        last=i;
      }
      
      foreach(P()) | [&] (auto m)
      {
        auto it = map.find(m.symbol().name());
        if (it != map.end())
        {
          try
          {
            decode_post_parameter_urlencoded((std::decay_t<decltype(m.value())>*) 0,
                                             res[m.symbol()], it->second);
          }
          catch (const std::exception& e)
          {
            throw error::bad_request(std::string("Error while decoding the POST parameter ") +
                                     m.symbol().name() + ": " + e.what());
          }
        }
        else
        {
          if(!m.attributes().has(_optional))
            throw std::runtime_error(std::string("Missing required POST parameter ") + m.symbol().name());
        }
            
      };
    }

    
    template <typename P, typename T>
    auto deserialize(request_type* r, P procedure, T& res) const
    {
      try
      {
        decode_url_arguments<typename P::path_type>(res, r->url);
        decode_get_arguments<typename P::route_type::get_parameters_type>(res, r);

        using post_t = typename P::route_type::post_parameters_type;
        if (post_t::size() > 0)
        {
          const char* encoding = MHD_lookup_connection_value(r->connection, MHD_HEADER_KIND, "Content-Type");
          if (!encoding)
            throw error::bad_request(std::string("Content-Type is required to decode the POST parameters"));
          
          if (encoding == std::string("application/x-www-form-urlencoded"))
            decode_post_parameters_urlencoded<post_t>(res, r);
          else if (encoding == std::string("application/json"))
            decode_post_parameters_json<post_t>(res, r);
          else
            throw error::bad_request(std::string("Content-Type not implemented: ") + encoding);
        }
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

    void serialize2(response_type* r, const file f) const
    {
      int fd = open(f.path().c_str(), O_RDONLY);

      if (fd == -1)
        throw error::not_found("File not found.");

      r->file_descriptor = fd;
      r->status = 200;
    }
    
    void serialize2(response_type* r, const string_ref res) const
    {
      r->status = 200;
      r->body = std::string(&res.front(), res.size());
    }
    
    void serialize2(response_type* r, const char* res) const
    {
      r->status = 200;
      r->body = res;
    }
    
    template <typename T>
    auto serialize2(response_type* r, const T& res) const
    {
      r->status = 200;
      r->body = json_encode(res);      
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
        r->set_header("Content-Type", std::string(res.get(_content_type, "")));
    }
    
  };

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
        resp->set_cookie(key, token);
      }
      else
      {
        token = token_;
      }

      return tracking_cookie(token);
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

    mhd_request rq{connection, *pp, url};
    mhd_response resp;

    try
    {
      service(std::string("/") + std::string(method) + url, &rq, &resp, connection);
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

    if (resp.file_descriptor > -1)
    {
      struct stat st;
      if (fstat(resp.file_descriptor, &st) != 0)
        throw error::not_found("Cannot fstat this file");
      response = MHD_create_response_from_fd(st.st_size, resp.file_descriptor);
    }
    else
    {
      const std::string& str = resp.body;
      response = MHD_create_response_from_buffer(str.size(),
                                                 (void*) str.c_str(),
                                                 MHD_RESPMEM_MUST_COPY);
    }
    
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
      std::string set_cookie_string = kv.first + '=' + kv.second + "; Path=/";
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

  template <typename S>
  struct silicon_mhd_ctx
  {
    silicon_mhd_ctx(MHD_Daemon* d, S* s, char* cert, char* key)
      : daemon_(d),
        service_(s),
        cert_(cert),
        key_(key) {}

    ~silicon_mhd_ctx()
    {
      MHD_stop_daemon(daemon_);
      if (cert_)
        free(cert_);
      if (key_)
        free(key_);
    }

    MHD_Daemon* daemon_;
    std::shared_ptr<S> service_;
    char* cert_, *key_;
  };

  /*! 
  ** Start the microhttpd json backend. This function is by default blocking.
  ** 
  ** @param api The api
  ** @param middleware_factories The tuple of middlewares
  ** @param port the port
  ** @param opts Available options are:
  **         _one_thread_per_connection: Spans one thread per connection.
  **         _linux_epoll: One thread per CPU core with epoll.
  **         _select: select instead of epoll (active by default).
  **         _nthreads: Set the number of thread. Default: The numbers of CPU cores.
  **         _non_blocking: Run the server in a thread and return in a non blocking way.
  **         _blocking: (Active by default) Blocking call.
  **         _https_key: path to the https key file.
  **         _https_cert: path to the https cert file.
  ** 
  ** @return If set as non_blocking, this function returns a
  ** silicon_mhd_ctx that will stop and cleanup the server at the end
  ** of its lifetime. If set as blocking (default), never returns
  ** except if an error prevents or stops the execution of the server.
  **
  */
  template <typename A, typename M, typename... O>
  auto mhd_json_serve(const A& api, const M& middleware_factories,
                      int port, O&&... opts)
  {

    int flags = MHD_USE_SELECT_INTERNALLY;
    auto options = D(opts...);
    if (options.has(_one_thread_per_connection))
      flags = MHD_USE_THREAD_PER_CONNECTION;
    else if (options.has(_select))
      flags = MHD_USE_SELECT_INTERNALLY;
    else if (options.has(_linux_epoll))
    {
#if MHD_VERSION >= 0x00095100
      flags = MHD_USE_EPOLL_INTERNALLY;
#else
      flags = MHD_USE_EPOLL_INTERNALLY_LINUX_ONLY;
#endif
    }

    auto read_file = [] (std::string path) {
      std::ifstream in(path);
      if (!in.good())
      {
        std::stringstream err_ss;
        err_ss << "Cannot read " << path << " " << strerror(errno);
        throw std::runtime_error(err_ss.str());
      }
      std::ostringstream ss{};
      ss << in.rdbuf();
      return ss.str();
    };

    std::string https_cert, https_key;
    if (options.has(_https_cert) || options.has(_https_key))
    {
      std::string cert_file = options.get(_https_cert, "");
      std::string key_file = options.get(_https_key, "");
#ifdef MHD_USE_TLS
      flags |= MHD_USE_TLS;
#else
      flags |= MHD_USE_SSL;
#endif
      
      if (cert_file.size() == 0) throw std::runtime_error("Missing HTTPS certificate file"); 
      if (key_file.size() == 0) throw std::runtime_error("Missing HTTPS key file");

      https_key = std::move(read_file(key_file));
      https_cert = std::move(read_file(cert_file));
      
    }

    char* https_cert_buffer = https_cert.size() ? strdup(https_cert.c_str()) : 0;
    char* https_key_buffer = https_key.size() ? strdup(https_key.c_str()) : 0;
  
    int thread_pool_size = options.get(_nthreads, std::thread::hardware_concurrency());

    auto m2 = std::tuple_cat(std::make_tuple(mhd_session_cookie()),
                                             middleware_factories);
    
    using service_t = service<mhd_json_service_utils,
                              decltype(m2),
                              mhd_request*, mhd_response*, MHD_Connection*>;
    
    auto s = new service_t(api, m2);
      
    MHD_Daemon* d;

    if (https_key.size() > 0)
    {
      if (MHD_is_feature_supported(MHD_FEATURE_SSL) == MHD_NO)
        throw std::runtime_error("microhttpd has not been compiled with SSL support.");
        
      if (flags != MHD_USE_THREAD_PER_CONNECTION)
        d = MHD_start_daemon(
                             flags,
                             port,
                             NULL,
                             NULL,
                             &mhd_handler<service_t>,
                             s,
                             MHD_OPTION_THREAD_POOL_SIZE, thread_pool_size,
                             MHD_OPTION_HTTPS_MEM_KEY, https_key_buffer,
                             MHD_OPTION_HTTPS_MEM_CERT, https_cert_buffer,
                             MHD_OPTION_END);
      else
        d = MHD_start_daemon(
                             flags,
                             port,
                             NULL,
                             NULL,
                             &mhd_handler<service_t>,
                             s,
                             MHD_OPTION_HTTPS_MEM_KEY, https_key_buffer,
                             MHD_OPTION_HTTPS_MEM_CERT, https_cert_buffer,
                             MHD_OPTION_END);
      
    }
    else // Without SSL
    {
      if (flags != MHD_USE_THREAD_PER_CONNECTION)
        d = MHD_start_daemon(
                             flags,
                             port,
                             NULL,
                             NULL,
                             &mhd_handler<service_t>,
                             s,
                             MHD_OPTION_THREAD_POOL_SIZE, thread_pool_size,
                             MHD_OPTION_END);
      else
        d = MHD_start_daemon(
                             flags,
                             port,
                             NULL,
                             NULL,
                             &mhd_handler<service_t>,
                             s,
                             MHD_OPTION_END);
      

    }

    if (d == NULL)
      throw std::runtime_error("Cannot start the microhttpd daemon");

    if (!options.has(_non_blocking))
    {
      while (true) usleep(1e6);
    }

    return silicon_mhd_ctx<service_t>
      (d, s, https_cert_buffer, https_key_buffer);
  }
  
  template <typename A, typename... O>
  auto mhd_json_serve(const A& api, int port, O&&... opts)
  {
    return mhd_json_serve(api, std::make_tuple(), port, opts...);
  }

  auto file_server(const std::string& fixed_base_path)
  {
    std::string base_path = fixed_base_path;
    if(!base_path.empty() && base_path[base_path.size() - 1] != '/'){
      base_path.push_back('/');
    }
    return [base_path](mhd_request* r){
      static char dot = '.', slash = '/';
      std::string path(r->url);
      size_t len = path.size();
      if(!path.empty() && path[0] == slash)
      {
        path.erase(0, 1);
      }
      if(path.empty())
      {
        throw error::bad_request("No file path given, directory listing not supported.");
      }
      if(len == 1 && path[0] == dot)
      {
        throw error::bad_request("Invalid URI ", path);
      }else if(len == 2 && path[0] == dot && path[1] == dot)
      {
        throw error::bad_request("Invalid URI ", path);
      }else{
        char prev0 = slash, prev1 = slash;
        for(size_t i = 0; i < len; ++i){
          if(prev0 == dot && prev1 == dot && path[i] == slash){
            throw error::bad_request("Unsupported URI, ../ is not allowed in the URI");
          }
          prev0 = prev1;
          prev1 = path[i];
        }
      }
      return file(base_path + path);
    };
  }
}
