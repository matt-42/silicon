#pragma once

#include <sstream>
#include <silicon/api.hh>


namespace sl
{

  const char* type_string(const void*) { return "void"; }
  const char* type_string(const std::string*) { return "string"; }
  const char* type_string(const int*) { return "int"; }
  const char* type_string(const float*) { return "float"; }
  const char* type_string(const double*) { return "double"; }

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

  template <typename P>
  std::string procedure_description(P& m, std::string scope = "")
  {
    std::stringstream res;

    res << scope << m.symbol().name() << '(';
    typedef std::remove_reference_t<decltype(m.value().function())> F;
    typedef callable_return_type_t<F> ret_type;
    first_sio_of_tuple_t<callable_arguments_tuple_t<F>> args;

    //void* x = m.value();
    bool first = true;
    foreach(args) | [&] (auto& a) {
      if (!first) res << ", ";
      first = false;
      res << a.symbol().name() << ": " << type_string(&a.value());
    };
    res << ") -> " << type_string((ret_type*) 0);
    return res.str();
  }
  
  template <typename A>
  std::string api_description2(A& api, std::string scope = "")
  {
    std::stringstream res;
    foreach(api) | [&] (auto& m)
    {
      static_if<is_sio<decltype(m.value())>::value>(
        [&] (auto m) { // If sio, recursion.
          res << api_description2(m.value(), scope + m.symbol().name() + ".");
        },
        [&] (auto m) { // Else, register the procedure.
          res << procedure_description(m, scope) << std::endl;
        }, m);
      
    };
    return res.str();
  }

  template <typename A, typename M>
  std::string api_description(api<A, M>& api)
  {
    return api_description2(api.procedures());
  }
  
}
