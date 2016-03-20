#include <sstream>
#include <cstring>

#include <iod/foreach.hh>
#include <silicon/clients/templates/javascript.hh>
#include <silicon/clients/templates/javascript_websocket.hh>
#include <silicon/api_description.hh>
#include <silicon/api_to_sio.hh>

namespace sl
{
  using namespace iod;

  std::string javascript_api_base();

  namespace tpl
  {
    struct path_generator
    {

      path_generator(std::string j)
        : default_join(j)
      {
      }
        
      path_generator(std::vector<std::string>& s, std::string j)
        : stack_(s), default_join(j)
      {}

      // Convert the path to a string.
      auto operator()(std::string s = "", std::string join = "", bool skip_first = false)
      {
        if (join.size() == 0)
          join = default_join;

        std::string res;
        bool first = true;
        for (int i = skip_first ? 1 : 0; i < stack_.size(); i++)
        {
          auto x = stack_[i];
          if (!first) res += join;
          first = false;
          res += x;
        }

        if (s.size() != 0)
        {
          if (res.size() != 0)
            res += join;
          res += s;
        }
        return res;
      }

      // Push a path element.
      auto operator+(std::string s)
      {
        path_generator n(stack_, default_join);
        n.stack_.push_back(s);
        return n;
      }

      auto root() const { return stack_[0]; }

      std::vector<std::string> stack_;
      std::string default_join;
    };


    auto read_until(const char*& c, const char* pattern, bool required = true)
    {
      const char* end = c;
      bool end_found = false;
      while (*end and !(end_found = !strncmp(end, pattern, strlen(pattern)))) end++;
      if (!end_found and required) throw std::runtime_error(std::string("Missing ") + pattern);

      std::string res(c, end);
      if (end_found)
        c = end + strlen(pattern);
      if (!end_found)
        c = end;
      return res;
    }

    template <typename F>
    void tpl_generator_stm(const std::string& content, const char*& it, F& default_cb) { default_cb(content); } 

    template <typename F, typename G, typename... A>
    void tpl_generator_stm(const std::string& content, const char*& it, F& default_cb,
                             std::string m, G& cb, A&&... args)
    {
      if (content == m) cb(it);
      else tpl_generator_stm(content, it, default_cb, args...);
    }
   
    template <typename F, typename... A>
    void tpl_generator(const std::string& tpl, F& default_cb, A&&... args)
    {
      const char* c = tpl.data();
      while (*c)
      {
        std::string s1 = read_until(c, "{{", false);
        default_cb(s1);
        if (*c)
        {
          std::string s2 = read_until(c, "}}");
          tpl_generator_stm(s2, c, default_cb, args...);
        }
      }
    }

    inline std::string return_type_string(void*) { return "void"; }
    inline std::string return_type_string(std::string*) { return "string"; }
    template <typename... T>
    std::string return_type_string(sio<T...>*) { return "object"; }
    
    template <typename P, typename Ro, typename W>
    void generate_procedure(P p, Ro route, std::string tpl, W& write, path_generator path)
    {
      typedef std::remove_reference_t<decltype(p.value().content.function())> F;
      typedef std::remove_reference_t<std::remove_const_t<callable_return_type_t<F>>> R;

      tpl_generator(
        tpl, write,

        "procedure_path", [&] (const char*&) {
          write(path(p.symbol().name()));
        },
        "procedure_description", [&] (const char*&) {
          write(procedure_description(p.value().route, p.value().content));
        },
        "procedure_url_string", [&] (const char*&) {
          write(path(p.symbol().name(), "/", true));
        },
        "procedure_url", [&] (const char*&) {
          auto url = p.value().route;
          write("{");
          foreach(url.path) | [&] (auto e)
          {
            std::string name =
              static_if<is_symbol<decltype(e)>::value>(
                [&] (auto e) { return e.name(); },
                [&] (auto e) { return e.symbol().name(); }, e);
            
            write(name + ": { is_param: " +
                  (!is_symbol<decltype(e)>::value ? "true" : "false") + "},");
          };
          write("}");
        },
        
        "return_type", [&] (const char*&) {
          write(return_type_string((R*)0));
        },
        "root_scope", [&] (const char*&) {
          write(path.root());
        }
        
        );
    }

    template <typename P, typename... Ro, typename W>
    void generate_procedure(P p, const http_route<Ro...>& route, std::string tpl, W& write, path_generator path)
    {
      typedef std::remove_reference_t<decltype(p.value().content.function())> F;
      typedef std::remove_reference_t<std::remove_const_t<callable_return_type_t<F>>> R;

      tpl_generator(
        tpl, write,

        "procedure_path", [&] (const char*&) {
          write(path(p.symbol().name()));
        },
        "procedure_description", [&] (const char*&) {
          write(procedure_description(p.value().route, p.value().content));
        },
        "procedure_url_string", [&] (const char*&) {
          write(path(p.symbol().name(), "/", true));
        },
        "http_method", [&] (const char*&) {
          write(p.value().route.verb.to_string());
        },
        "procedure_url", [&] (const char*&) {
          auto url = p.value().route;
          write("{");
          foreach(url.path) | [&] (auto e)
          {
            std::string name =
              static_if<is_symbol<decltype(e)>::value>(
                [&] (auto e) { return e.name(); },
                [&] (auto e) { return e.symbol().name(); }, e);
            
            write(name + ": { is_param: " +
                  (!is_symbol<decltype(e)>::value ? "true" : "false") + "},");
          };
          write("}");
        },
        "params_schema", [&] (const char*&) {
          auto url = p.value().route;
          write("{get:{");
          foreach(url.get_params) | [&] (auto e)
          {
            write(std::string(e.symbol().name()) + ": { required: " +
                  (!has_symbol<decltype(e.attributes()), _optional_t>::value ? "true" : "false") + "},");
          };
          write("},post:{");
          foreach(url.post_params) | [&] (auto e)
          {
            write(std::string(e.symbol().name()) + ": { required: " +
                  (!has_symbol<decltype(e.attributes()), _optional_t>::value ? "true" : "false") + "},");
          };
          write("}}");
        },
        
        "return_type", [&] (const char*&) {
          write(return_type_string((R*)0));
        },
        "root_scope", [&] (const char*&) {
          write(path.root());
        }
        
        );
    }
    
    template <typename S, typename W>
    void generate_scope(S& api, std::string tpl, W& write, path_generator path)
    {
      tpl_generator(
        tpl,
        write,

        "scope_path", [&] (const char*&) {
          write(path());
        },

        "procedure", [&] (const char*& c) {
          std::string proc_tpl = read_until(c, "{{end_procedure}}");

          return ::iod::foreach(api) | [&] (auto m)
          {
            return static_if<is_sio<decltype(m.value())>::value>(
              [] (auto m) {
              },
              [&] (auto m) {
                generate_procedure(m, m.value().route, proc_tpl, write, path);
              }, m);
          };
          
        },

        "child_scopes", [&] (const char*& c) {
          return foreach(api) | [&] (auto m)
          {
            return static_if<is_sio<decltype(m.value())>::value>(
              [&] (auto m) {
                auto new_path = path + m.symbol().name();
                generate_scope(m.value(), tpl, write, new_path);
              },
              [&] (auto m) {
              }, m);
          };
        },
        "root_scope", [&] (const char*&) {
          write(path.root());
        }
        
      
        );

      
    }

    template <typename A, typename W, typename... T>
    void generate_client(const A& api, const std::string& tpl, W& write,
                         path_generator path, T... _options)
    {
      tpl_generator(
        tpl,
        write,

        "scope",
        [&] (const char*& c) {
          std::string scope_tpl = read_until(c, "{{end_scope}}");
          generate_scope(api, scope_tpl, write, path);
        },

        "root_scope", [&] (const char*&) {
          write(path());
        }
        
        );      
    
    }

  }
  
  template <typename A, typename... T>
  std::string generate_javascript_client(const A& api, T... _options)
  {
    auto path = tpl::path_generator(".") + "sl";

    auto api_sio = http_api_to_sio(api);
    
    std::stringstream ss;
    auto write = [&] (const auto& s) { ss << s; };
    tpl::generate_client(api_sio, javascript_client_template, write, path);
    return ss.str();
  }

  template <typename A, typename... T>
  std::string generate_javascript_websocket_client(const A& api, T... _options)
  {
    auto path = tpl::path_generator(".") + "sl";

    auto api_sio = ws_api_to_sio(api);
    
    std::stringstream ss;
    auto write = [&] (const auto& s) { ss << s; };
    tpl::generate_client(api_sio, javascript_websocket_client_template, write, path);
    return ss.str();
  }
}
