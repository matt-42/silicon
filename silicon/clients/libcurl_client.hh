#include <memory>
#include <sstream>
#include <map>
#include <curl/curl.h>

#include <iod/json.hh>
#include <iod/sio_utils.hh>
#include <silicon/symbols.hh>
#include <silicon/http_route.hh>

namespace sl
{
  using namespace iod;

  template <typename R>
  struct response_parser
  {
    static auto run(long response_code, const std::string& body)
    {
      return D(s::_status = response_code, s::_error = body);
    }
  };

  template <typename... T>
  struct response_parser<sio<T...>>
  {
    static auto run(long response_code, const std::string& body)
    {
      sio<T...> result;

      std::string error_message;
      if (response_code == 200)
        json_decode(result, body);
      else
        error_message = body;

      return D(
        s::_status = response_code,
        s::_response = result,
        s::_error = error_message);
    }
  };

  struct libcurl_http_client;

  inline size_t curl_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

  struct libcurl_http_client
  {
    libcurl_http_client()
    {
      curl_global_init(CURL_GLOBAL_ALL);
      curl_ = curl_easy_init();
    }

    ~libcurl_http_client()
    {
      curl_easy_cleanup(curl_);      
    }

    libcurl_http_client& operator=(const libcurl_http_client&) = delete;

    void connect(const std::string& host, int port)
    {
      host_ = host;
      port_ = port;
    }

    template <typename R, typename S, typename A>
    auto remote_call(S route, const A& args)
    {
      
      // Generate url.
      std::stringstream url_ss;
      url_ss << "http://" << host_ << ":" << port_;

      // Path with parameters
      foreach(route.path) | [&] (auto m)
      {
        static_if<is_symbol<decltype(m)>::value>(
          [&] (auto m) { url_ss << "/" << m.name(); },
          [&] (auto m) { url_ss << "/" << args[m.symbol()]; },
          m);
      };

      // Get params
      static_if<(route.get_params._size > 0)>(
        [&] (auto args) {
          auto get_params = iod::intersect(args, route.get_params);
          if (get_params.size() > 0) url_ss << '?';
          foreach(get_params) | [&] (auto m) {
            std::stringstream value_ss;
            value_ss << m.value();
            char* escaped = curl_easy_escape(curl_, value_ss.str().c_str(),
                                             value_ss.str().size());
            url_ss << m.symbol().name() << '=' << escaped << '&';
            curl_free(escaped);
          };
        },
        [&] (auto args) {}, args);

      // Pass the url to libcurl.
      curl_easy_setopt(curl_, CURLOPT_URL, url_ss.str().c_str());

      std::cout << url_ss.str() << std::endl;
      // Set the HTTP verb.
      if (std::is_same<decltype(route.verb), http_post>::value) curl_easy_setopt(curl_, CURLOPT_POST, 1);
      if (std::is_same<decltype(route.verb), http_get>::value) curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1);
      if (std::is_same<decltype(route.verb), http_put>::value) curl_easy_setopt(curl_, CURLOPT_PUT, 1);
      
      // POST parameters.
      std::string rq_body;
      char* rq_body_encoded = nullptr;
      static_if<(route.post_params._size > 0)>(
        [&] (auto args) {
         auto post_params = iod::intersect(args, route.post_params);
         rq_body = json_encode(post_params);
         rq_body_encoded = curl_easy_escape(curl_ , rq_body.c_str(), rq_body.size());
         curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, rq_body.c_str());
        },
        [&] (auto args) {}, args);

      curl_easy_setopt(curl_, CURLOPT_COOKIEJAR, 0); // Enable cookies but do no write a cookiejar.

      body_buffer_.clear();
      curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, curl_write_callback);
      curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
      
      char errbuf[CURL_ERROR_SIZE];
      if (curl_easy_perform(curl_) != CURLE_OK)
      {
        std::stringstream errss;
        errss << "Libcurl error when sending request: " << errbuf;
        throw std::runtime_error(errss.str());
      }

      long response_code;
      curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);

      if (rq_body_encoded != nullptr) curl_free(rq_body_encoded);

      // Decode result.
      return response_parser<R>::run(response_code, body_buffer_);
    }

    void read(char* ptr, int size)
    {
      body_buffer_.append(ptr, size);
    }
    
    CURL* curl_;
    std::map<std::string, std::string> cookies_;
    std::string body_buffer_;
    std::string host_;
    int port_;
  };

  template <typename C, typename R, typename Ret>
  struct generic_client_call
  {
    generic_client_call(C c, R r) : client(c), route(r) {}

    template <typename... A>
    auto operator()(A... call_args)
    {
      return client->template remote_call<Ret>(route, D(call_args...));
    }

    C client;
    R route;
  };

  size_t curl_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
  {
    libcurl_http_client* client = (libcurl_http_client*)userdata;
    client->read(ptr, size * nmemb);
    return size * nmemb;
  }
  
  template <typename Ret, typename C, typename R>
  auto create_client_call(C& c, R route)
  {
    return generic_client_call<C, R, Ret>(c, route);
  }

  inline auto filter_symbols_from_tuple(std::tuple<> path)
  {
    return std::tuple<>();
  }

  template <typename T1, typename... T>
  auto filter_symbols_from_tuple(std::tuple<T1, T...> path)
  {
    return iod::foreach(path) | [] (auto e)
    {
      return static_if<is_symbol<decltype(e)>::value>(
        [&] () { return e; },
        [] () { }
        );
    };
  }

  template <typename T>
  auto symbol_tuple_to_sio(const std::tuple<>*, T content)
  {
    return content;
  }
  
  template <typename S1, typename T, typename... S>
  auto symbol_tuple_to_sio(const std::tuple<S1, S...>*, T content)
  {
    return D(S1() = symbol_tuple_to_sio((std::tuple<S...>*)0, content));
  }


  template <typename R, typename M>
  struct client_method_with_root : public R, 
                                   public M
  {
    using R::operator();
    client_method_with_root(R root, M methods) : R(root), M(methods) {}
  };

  template <typename R, typename M>
  auto make_client_methods_with_root(R root, M methods)
  {
    return client_method_with_root<R, M>(root, methods);
  }

  template <typename S>
  auto process_roots(S o)
  {
    auto res = sio_iterate(o, D(_root = int(), _methods = sio<>())) | [] (auto m, auto prev)
    {
      // if m.value() is a sio, recurse.
      return static_if<is_sio<decltype(m.value())>::value>(
        [&] (auto m) { return D(_root = prev.root,
                                _methods = cat(prev.methods,
                                               D(m.symbol() = process_roots(m.value()))));
        },

        // if not, process a leave.
        [&] (auto m) {
      
          // if m is root
          return static_if<std::is_same<decltype(m.symbol()), _silicon_root____t>::value>(
            [&] (auto m, auto prev) {
              return D(_root = m.value(), _methods = prev);
            },
            [&] (auto m, auto prev) {
              // else
              return D(_root = prev.root, _methods = cat(D(m), prev.methods));
            }, m, prev);
        }, m);
    };

    return static_if<std::is_same<decltype(res.root), int>::value>(
      [] (auto res) {
        return res.methods;
      },
      [] (auto res) {
        return make_client_methods_with_root(res.root, res.methods);
      }, res);
  }
  
  template <typename C, typename A, typename PR>
  auto generate_client_methods(C& c, A api, PR parent_route)
  {
    auto tu = foreach(api) | [&c] (auto m) {

      return static_if<is_tuple<decltype(m.content)>::value>(

        [&c] (auto m) { // If sio, recursion.
          return generate_client_methods(c, m.content, m.route);
        },

        [&c] (auto m) { // Else, register the procedure.
          typedef std::remove_reference_t<decltype(m.content)> V;
          typedef typename V::function_type F;
          typename V::arguments_type arguments;

          auto cc = create_client_call<callable_return_type_t<F>>(c, m.route);
          auto st = filter_symbols_from_tuple(m.route.path);
          return static_if<(std::tuple_size<decltype(st)>() !=
                            std::tuple_size<typename PR::path_type>())>(
            [&] () {
              return symbol_tuple_to_sio(&st, cc);
            },
            [&] () {
              auto p = std::tuple_cat(st, std::make_tuple(_silicon_root___));
              return symbol_tuple_to_sio(&p, cc);
            });
        }, m);

    };

    // if in the tuple tu, there is a method without a path, this is the root
    // and with must make it accessible via the operator() of the result object.
    auto tu2 = deep_merge_sios_in_tuple(tu);
    return process_roots(tu2);
  }

  template <typename A>
  auto libcurl_json_client(const A& api, std::string host, int port)
  {
    std::shared_ptr<libcurl_http_client> c(new libcurl_http_client());
    c->connect(host, port);
    return generate_client_methods(c, api.procedures(), http_route<>());
  }
}
