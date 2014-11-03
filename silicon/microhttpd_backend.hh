#pragma once

#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sstream>

#include <iod/json.hh>

namespace iod
{


  struct request
  {
    request(std::string str)  {
      body_stream.str(str);
      body_stream.seekg(0);
      body_stream.clear();
    }
    
    auto get_body_string() const { return body_stream.str(); }
    auto& get_body_stream() { return body_stream; }

    std::istringstream body_stream;
    std::string body_string;
  };

  struct response
  {
    response() {}

    auto get_body_string() const { return body_stream_.str(); }
    auto& get_body_stream() { return body_stream_; }

    template <typename T>
    auto operator<<(const T& t)
    {
      body_stream_ << t;
    }

    template <typename... T>
    auto operator<<(const iod::sio<T...>& t)
    {
      json_encode(t, body_stream_);
    }

    std::ostringstream body_stream_;
  };


  static int mhd_handler(void * cls,
                  struct MHD_Connection * connection,
                  const char * url,
                  const char * method,
                  const char * version,
                  const char * upload_data,
                  size_t * upload_data_size,
                  void ** ptr) {
    static int dummy;
    struct MHD_Response * response;
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

    auto& fun = * (std::function<void(iod::request&,iod::response&)>*)cls;

    iod::request rq(*pp);
    iod::response resp;
    fun(rq, resp);
    const std::string& str = resp.get_body_string();
    response = MHD_create_response_from_data(str.size(),
                                             (void*) str.c_str(),
                                             MHD_NO,
                                             MHD_YES);
    ret = MHD_queue_response(connection,
                             MHD_HTTP_OK,
                             response);
    MHD_destroy_response(response);
    delete pp;
    return ret;
  }

  struct mhd_backend
  {
    mhd_backend(int argc, char* argv[]) { }
    mhd_backend() {  }
    
    template <typename F>
    void serve(F f);
  };

  typedef mhd_backend backend;

  
  template <typename F>
  void mhd_backend::serve(F f)
  {
    std::function<void(request&,response&)> fun = f;

    struct MHD_Daemon * d;
    d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
                         1235,
                         NULL,
                         NULL,
                         &mhd_handler,
                         &fun,
                         MHD_OPTION_END);
    if (d == NULL)
      return;
    (void) getc (stdin);
    MHD_stop_daemon(d);
    return;
  }
  
}
