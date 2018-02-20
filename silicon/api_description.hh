#pragma once

#include <sstream>
#include <silicon/api.hh>
#include <silicon/backends/ws_api.hh>


namespace sl
{

  const char* type_string(const void*) { return "void"; }
  const char* type_string(const std::string*) { return "string"; }
  const char* type_string(const int*) { return "int"; }
  const char* type_string(const float*) { return "float"; }
  const char* type_string(const double*) { return "double"; }
  template <typename T>
  std::string type_string(const std::vector<T>*) {
    std::stringstream res;
    res << "vector of " << type_string(static_cast<const T*>(nullptr));
    return std::move(res.str());
  }

  template <typename... T>
  std::string type_string(const sio<T...>* o)
  {
    std::stringstream res; res << "{";
    bool first = true;
    foreach(*o) | [&] (auto& m) {
      if (!first) res << ", ";
      first = false;
      res << m.symbol().name() << ": " << type_string(&m.value());
    };
    res << "}";
    return std::move(res.str());
  }

  template <typename... R, typename P>
  std::string procedure_description(const http_route<R...>& route, P f)
  {
    std::stringstream res;

    res << route.verb_as_string() << ": ";

    foreach(route.path) | [&] (auto e)
    {
      static_if<is_symbol<decltype(e)>::value>(
        [&] (auto e2) { res << std::string("/") + e2.name(); },
        [&] (auto e2) { res << std::string("/[") << e2.symbol().name() << ": "
                            << type_string(&e2.value()) << "]"; }, e);
    };
    
    res << "(";
    typedef std::remove_reference_t<decltype(f.function())> F;
    typedef callable_return_type_t<F> ret_type;
    first_sio_of_tuple_t<callable_arguments_tuple_t<F>> args;

    auto get_post_args = iod::cat(route.get_params, route.post_params);
    
    bool first = true;
    foreach(get_post_args) | [&] (auto& a) {
      if (!first) res << ", ";
      first = false;
      res << a.symbol().name() << ": " << type_string(&a.value());
    };
    res << ") -> " << type_string((ret_type*) 0);
    return res.str();
  }
  

  template <typename... R, typename P>
  std::string procedure_description(const ws_route<R...>& route, P f)
  {
    std::stringstream res;

    foreach(route.path) | [&] (auto e)
    {
      res << std::string("/") + e.name();
    };
    
    res << "(";
    typedef std::remove_reference_t<decltype(f.function())> F;
    typedef callable_return_type_t<F> ret_type;
    first_sio_of_tuple_t<callable_arguments_tuple_t<F>> args;

    auto get_post_args = route.params;
    
    bool first = true;
    foreach(get_post_args) | [&] (auto& a) {
      if (!first) res << ", ";
      first = false;
      res << a.symbol().name() << ": " << type_string(&a.value());
    };
    res << ") -> " << type_string((ret_type*) 0);
    return res.str();
  }
  
  template <typename... R, typename P>
  std::string procedure_description(const rmq::route<R...>& route, P f)
  {
    std::stringstream res;

    res << route.exchange_as_string() << ": " << route.path_as_string(false);

    res << "(";
    typedef std::remove_reference_t<decltype(f.function())> F;
    typedef callable_return_type_t<F> ret_type;
    first_sio_of_tuple_t<callable_arguments_tuple_t<F>> args;

    auto params = route.all_params();

    bool first = true;
    foreach(params) | [&] (auto& a) {
      if (!first) res << ", ";
      first = false;
      res << a.symbol().name() << ": " << type_string(&a.value());
    };
    res << ") -> " << type_string((ret_type*) 0);
    return res.str();
  }

  template <typename A>
  std::string api_description(A& api)
  {
    std::stringstream res;
    foreach(api) | [&] (auto& m)
    {
      static_if<is_tuple<decltype(m.content)>::value>(
        [&] (auto m) { // If sio, recursion.
          res << api_description(m.content);
        },
        [&] (auto m) { // Else, register the procedure.
          res << procedure_description(m.route, m.content) << std::endl;
        }, m);
      
    };
    return res.str();
  }
  
}
