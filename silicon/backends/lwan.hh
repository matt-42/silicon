#pragma once

#include <cstring>
#include <vector>
#include <lwan/lwan.h>

#include <iod/json.hh>
#include <iod/utils.hh>

#include <silicon/symbols.hh>
#include <silicon/utils.hh>
#include <silicon/error.hh>
#include <silicon/service.hh>
#include <silicon/response.hh>
#include <silicon/optional.hh>
#include <silicon/middlewares/tracking_cookie.hh>
#include <silicon/middlewares/get_parameters.hh>

namespace sl
{

  struct lwan_request
  {
    lwan_request_t* lwan_req;
  };

  struct lwan_response
  {
    lwan_response(lwan_request_t* _lwan_req, lwan_response_t* _lwan_resp)
      : lwan_req(_lwan_req),
        lwan_resp(_lwan_resp),
        status(0),
        headers(NULL),
        headers_size(20),
        headers_cpt(0)
    {
      
      headers = (lwan_key_value_t*) coro_malloc(lwan_req->conn->coro, headers_size * sizeof(lwan_key_value_t));
      assert(headers != NULL);
      for (int i = 0; i < headers_size; i++)
      {
        headers[i].key = NULL;
        headers[i].value = NULL;
      }
      lwan_resp->headers = headers;
    }

    void add_header(std::string k, std::string v)
    {
      if (headers_cpt == headers_size)
      {
        int old_size = headers_size;
        lwan_key_value_t* old_headers = headers;

        headers_size *= 2;
        headers = (lwan_key_value_t*) coro_malloc(lwan_req->conn->coro, headers_size * sizeof(lwan_key_value_t));
        assert(headers != NULL);        
        memcpy(headers, old_headers, old_size * sizeof(lwan_key_value_t));

        for (int i = old_size; i < headers_size; i++)
        {
          headers[i].key = NULL;
          headers[i].value = NULL;
        }
        
      }

      headers[headers_cpt].key = coro_strdup(lwan_req->conn->coro, k.c_str());
      headers[headers_cpt].value = coro_strdup(lwan_req->conn->coro, v.c_str());
      headers_cpt++;
    }

    void add_cookie(std::string k, std::string v)
    {
      add_header("Set-Cookie", k + '=' + v);
    }
    
    lwan_request_t* lwan_req;
    lwan_response_t* lwan_resp;
    int status;
    std::string body;

    lwan_key_value_t* headers;
    int headers_size;
    int headers_cpt;
  };
  
  struct lwan_json_service_utils
  {
    typedef lwan_request request_type;
    typedef lwan_response response_type;

    template <typename S, typename O, typename C>
    void decode_get_arguments(O& res, C* req) const
    {
      foreach(S()) | [&] (auto m)
      {
        const char* param = lwan_request_get_query_param(req->lwan_req, m.symbol().name());
        if (param)
        {
          try
          {
            res[m.symbol()] = boost::lexical_cast<std::decay_t<decltype(res[m.symbol()])>>(param);
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
    void decode_post_parameters(O& res, lwan_request_t* r) const
    {
      foreach(P()) | [&] (auto m)
      {
        const char* value = lwan_request_get_post_param(r, m.symbol_name());

        if (value)
        {
          try
          {
            static_if<is_sio<std::decay_t<decltype(m.value())>>::value>(
              [&] (auto m, auto& res) { json_decode<std::decay_t<decltype(m.value())>>(res[m.symbol()], value); },
              [&] (auto m, auto& res) { res[m.symbol()] = boost::lexical_cast<std::decay_t<decltype(m.value())>>(value); }, m, res);
          }
          catch (const std::exception& e)
          {
            throw error::bad_request(std::string("Error while decoding the post parameter ") +
                                     m.symbol().name() + " with value '" + value + "': " + e.what());
          }
        }
        else
        {
          if (!m.attributes().has(_optional))
            throw std::runtime_error(std::string("Missing post parameter ") + m.symbol_name());
        }
      };
    }
    
    template <typename P, typename T>
    auto deserialize(request_type* r, P procedure, T& res) const
    {
      try
      {
        
        decode_url_arguments<typename P::path_type>(res, std::string("/") + std::string(r->lwan_req->url.value, r->lwan_req->url.len));
        decode_get_arguments<typename P::route_type::get_parameters_type>(res, r);

        decode_post_parameters<typename P::route_type::post_parameters_type>(res, r->lwan_req);
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
        r->lwan_resp->mime_type = &res.get(_content_type, string_ref("")).front();
    }
    
  };
  


  struct lwan_session_cookie
  {
    
    inline tracking_cookie instantiate(lwan_request* req, lwan_response* resp)
    {
      std::string key = "sl_token";
      std::string token;
      const char* token_ = lwan_request_get_cookie(req->lwan_req, key.c_str());
      
      if (!token_)
      {
        token = generate_secret_tracking_id();
        resp->add_cookie(key, token);
      }
      else
      {
        token = token_;
      }

      return tracking_cookie(token);
    }

  };
  
  
  template <typename S>
  static lwan_http_status_t
  lwan_silicon_handler(lwan_request_t *request,
                       lwan_response_t *response, void *data)
  {
    auto& service = * (S*)data;

    std::string method;
    if (request->flags & REQUEST_METHOD_GET) method = "GET";
    if (request->flags & REQUEST_METHOD_POST) method = "POST";

    std::string url(request->url.value, request->url.len);
    lwan_request rq{request};
    lwan_response resp(request, response);

    try
    {
      service(std::string("/") + std::string(method) + "/" + url, &rq, &resp);
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

    // resp.body = "hello world";
    // resp.status = 200;
    
    if (!response->mime_type)
      response->mime_type = "text/plain";
    strbuf_set(response->buffer, resp.body.c_str(), resp.body.size());
    // strbuf_set(response->buffer, "Hello world", resp.body.size());

    return lwan_http_status_t(resp.status);
  }
  

  inline void
  lwan_unset_urlmap_data_rec(lwan_trie_t *trie, lwan_trie_node_t *node)
  {
    if (!node)
      return;

    int32_t nodes_unset = node->ref_count;

    for (lwan_trie_leaf_t *leaf = node->leaf; leaf;) {
      lwan_trie_leaf_t *tmp = leaf->next;

      lwan_url_map_t* urlmap = (lwan_url_map_t*) leaf->data;
      urlmap->data = NULL;

      leaf = tmp;
    }

    for (int32_t i = 0; nodes_unset > 0 && i < 8; i++) {
      if (node->next[i]) {
        lwan_unset_urlmap_data_rec(trie, node->next[i]);
        --nodes_unset;
      }
    }

  }

  inline void lwan_unset_urlmap_data(lwan_trie_t *trie)
  {
    if (!trie || !trie->root)
      return;  

    lwan_unset_urlmap_data_rec(trie, trie->root);    
  }

  template <typename A, typename M, typename... O>
  void lwan_json_serve(const A& api, const M& middleware_factories,
                       int port, O&&... opts)
  {

    auto m2 = std::tuple_cat(std::make_tuple(lwan_session_cookie()),
                                             middleware_factories);
    
    using service_t = service<lwan_json_service_utils, A, decltype(m2),
                              lwan_request*, lwan_response*>;
    
    auto s = service_t(api, m2);

    const lwan_url_map_t default_map[] = {
      { .prefix = "/", .handler = &lwan_silicon_handler<service_t>, .data = &s},
      { .prefix = NULL }
    };

    lwan_t l;
    lwan_config_t c;


    c = *lwan_get_default_config();
    std::stringstream listen_str;
    listen_str << "*:" << port;
    c.listener = strdup(listen_str.str().c_str());
    
    lwan_init_with_config(&l, &c);

    lwan_set_url_map(&l, default_map);
    lwan_main_loop(&l);

    lwan_unset_urlmap_data(&l.url_map_trie);
    lwan_shutdown(&l);

  }

  template <typename A, typename... O>
  void lwan_json_serve(const A& api, int port, O&&... opts)
  {
    return lwan_json_serve(api, std::make_tuple(), port, opts...);
  }
  
}
