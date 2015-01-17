#pragma once

#include <sstream>
#include <string>

namespace sl
{

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
    inline void format_error_(E&) {}

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

}
