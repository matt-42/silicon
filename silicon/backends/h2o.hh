#pragma once

#undef H2O_USE_LIBUV
#define H2O_USE_EPOLL 1


#include <sys/sysinfo.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <thread>

#include <silicon/symbols.hh>
#include <silicon/error.hh>
#include <silicon/service.hh>
#include <silicon/middlewares/tracking_cookie.hh>
#include <iod/json.hh>

#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>

extern "C"
{
#include <h2o.h>
#include <h2o/http1.h>
#include <h2o/socket/evloop.h>
//#include <h2o/http2.h>
}

namespace sl
{
    
  struct h2o_json_service_utils
  {
    template <typename T>
    auto deserialize(h2o_req_t* req, T& res) const
    {
      try
      {
        std::string body(req->entity.base, req->entity.len);
        json_decode(res, body);
      }
      catch (const std::runtime_error& e)
      {
        throw error::bad_request("Error when decoding procedure arguments: ", e.what());
      }

    }

    void send(h2o_req_t* r, const std::string& m) const
    {
      static h2o_generator_t generator = { NULL, NULL };
      r->res.status = 200;
      r->res.reason = "OK";
      h2o_iovec_t body = h2o_strdup(&r->pool, m.c_str(), m.size());
      h2o_start_response(r, &generator);
      h2o_send(r, &body, 1, 1);
    }

    void serialize2(h2o_req_t* r, const std::string& res) const
    {
      send(r, res);
    }

    void serialize2(h2o_req_t* r, const char* res) const
    {
      send(r, res);
    }
    
    template <typename T>
    auto serialize2(h2o_req_t* r, const T& res) const
    {
      std::string str = json_encode(res);
      send(r, str);
    }

    template <typename T>
    auto serialize(h2o_req_t* r, const T& res) const
    {
      serialize2(r, res);
    }
    
    
  };

  template <typename S>
  static int h2o_on_request(h2o_handler_t* self, h2o_req_t* req)
  {
    h2o_generator_t* generator = new h2o_generator_t{ NULL, NULL };  
    
    S& service = **((S**)(self + 1));
    // std::cout << "on req " << req << std::endl;

    // req->res.status = 200;
    // req->res.reason = "OK";
    // h2o_iovec_t body = h2o_strdup(&req->pool, "hello world\n", SIZE_MAX);

    // h2o_start_response(req, generator);
    // h2o_send(req, &body, 1, 1);
    // return 0;
    
    try
    {
      std::string path(req->path.base, req->path.len);
      service(path, req);
    }
    catch(const error::error& e)
    {
      req->res.status = e.status();
      std::string m = e.what();
      h2o_iovec_t body = h2o_strdup(&req->pool, m.c_str(), m.size());
      h2o_start_response(req, generator);
      h2o_send(req, &body, 1, 1);
    }
    catch(const std::runtime_error& e)
    {
      std::cout << e.what() << std::endl;
      req->res.status = 500;
      const char* m = "Internal server error.";
      h2o_iovec_t body = h2o_strdup(&req->pool, m, strlen(m));
      h2o_start_response(req, generator);
      h2o_send(req, &body, 1, 1);
    }
    
    return 0;
  }


  h2o_globalconf_t config;
  h2o_context_t ctx;
  SSL_CTX *ssl_ctx = NULL;

  void on_accept(h2o_socket_t *listener, int status)
  {
    h2o_socket_t *sock;

    if (status == -1) {
      return;
    }

    if ((sock = h2o_evloop_socket_accept(listener)) == NULL) {
      return;
    }
    if (ssl_ctx != NULL)
      h2o_accept_ssl(&ctx, sock, ssl_ctx);
    else
      h2o_http1_accept(&ctx, sock);
  }

  int create_listener(void)
  {
    struct sockaddr_in addr;
    int fd, reuseaddr_flag = 1;
    h2o_socket_t *sock;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7f000001);
    addr.sin_port = htons(7890);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1
        || setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_flag, sizeof(reuseaddr_flag)) != 0
        || bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0
        || listen(fd, SOMAXCONN) != 0) {
      return -1;
    }

    //sock = h2o_evloop_socket_create(ctx.loop, fd, (sockaddr*)&addr, sizeof(addr), H2O_SOCKET_FLAG_IS_ACCEPT);
    sock = h2o_evloop_socket_create(ctx.loop, fd, 0,0, H2O_SOCKET_FLAG_IS_ACCEPT);
    //sock = h2o_evloop_socket_create(ctx.loop, fd, 0,0, 0);
    h2o_socket_read_start(sock, on_accept);

    return 0;
  }

  
  int create_socket_fd()
  {
    struct sockaddr_in addr;
    int fd, reuseaddr_flag = 1;
    h2o_socket_t *sock;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7f000001);
    addr.sin_port = htons(7890);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1
        || setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_flag, sizeof(reuseaddr_flag)) != 0
        || bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0
        || listen(fd, SOMAXCONN) != 0) {
      return -1;
    }

    return fd;
  }


  struct listener_config_t {
    int fd;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    H2O_VECTOR(struct listener_ssl_config_t) ssl;
  };

  struct listener_ctx_t {
    h2o_context_t *ctx;
    SSL_CTX *ssl_ctx;
    h2o_socket_t *sock;
  };
  
  static void *run_loop()
  {
    const int num_listeners = 1;
    h2o_evloop_t *loop;
    h2o_context_t ctx;
    listener_ctx_t listeners[num_listeners];
    size_t i;

    /* setup loop and context */
    loop = h2o_evloop_create();
    h2o_context_init(&ctx, loop, &config);

      // struct sockaddr_in addr;
      // int fd, reuseaddr_flag = 1;
      // h2o_socket_t *sock;

      // memset(&addr, 0, sizeof(addr));
      // addr.sin_family = AF_INET;
      // addr.sin_addr.s_addr = htonl(0x7f000001);
      // addr.sin_port = htons(7890);
      
      // if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1
      //     || setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_flag, sizeof(reuseaddr_flag)) != 0
      //     || bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0
      //     || listen(fd, SOMAXCONN) != 0) {
      //   // if (fd != -1)
      //   //     close(fd);
      //   std::cout << "Error listening: " << strerror(errno) << std::endl;
      //   //return NULL;
      // }
      
    int fd = create_socket_fd();
    h2o_socket_t* sock = h2o_evloop_socket_create(
        ctx.loop, fd, 0, 0,
        //(struct sockaddr*)&listener_config->addr, listener_config->addrlen,
        H2O_SOCKET_FLAG_IS_ACCEPT);

    h2o_socket_read_start(sock, on_accept);
    
    /* the main loop */
    while (1) {
      h2o_evloop_run(loop);
    }

    return NULL;
  }
  
  template <typename A, typename... O>
  void h2o_json_serve(const A& api, int port, O&&... opts)
  {
    // auto api2 = api.bind_factories(h2o_session_cookie());
    auto s = service<h2o_json_service_utils, A, h2o_req_t*>(api);
    typedef decltype(s) S;
    
    h2o_hostconf_t *hostconf;

    signal(SIGPIPE, SIG_IGN);
    
    h2o_config_init(&config);
    hostconf = h2o_config_register_host(&config, "default");
    h2o_pathconf_t *pathconf = h2o_config_register_path(hostconf, "/");
    h2o_handler_t *handler = h2o_create_handler(pathconf, sizeof(*handler) + sizeof(void*));
    handler->on_req = &h2o_on_request<S>;

    *((void**)(handler + 1)) = &s;

    h2o_context_init(&ctx, h2o_evloop_create(), &config);

    /* disabled by default: uncomment the block below to use HTTPS instead of HTTP */
    /*
    if (setup_ssl("server.crt", "server.key") != 0)
        goto Error;
    */

    /* disabled by default: uncomment the line below to enable access logging */
    /* h2o_access_log_register(&config.default_host, "/dev/stdout", NULL); */

    //int socket_fd = create_socket_fd();

    const int num_threads = 4;
    std::thread* threads[num_threads];
    for (int i = 0; i < num_threads; i++)
      threads[i] = new std::thread(run_loop);
    for (int i = 0; i < num_threads; i++)
      threads[i]->join();
    // run_loop(nullptr);
    
    // if (create_listener() != 0) {
    //     fprintf(stderr, "failed to listen to 127.0.0.1:7890:%s\n", strerror(errno));
    //     return;
    // }

    // while (h2o_evloop_run(ctx.loop) == 0)
    //   ;
    
    // int flags = H2O_USE_SELECT_INTERNALLY;
    // auto options = D(opts...);
    // if (options.has(_one_thread_per_connection))
    //   flags = H2O_USE_THREAD_PER_CONNECTION;
    // else if (options.has(_select))
    //   flags = H2O_USE_SELECT_INTERNALLY;
    // else if (options.has(_linux_epoll))
    //   flags = H2O_USE_EPOLL_INTERNALLY_LINUX_ONLY;

    // int thread_pool_size = options.get(_nthreads, get_nprocs());

    // auto api2 = api.bind_factories(h2o_session_cookie());
    // auto s = service<h2o_json_service_utils, decltype(api2)>(api2);
    // typedef decltype(s) S;
      
    // struct H2O_Daemon * d;

    // if (flags != H2O_USE_THREAD_PER_CONNECTION)
    //   d = H2O_start_daemon(
    //     flags,
    //     port,
    //     NULL,
    //     NULL,
    //     &h2o_handler<S>,
    //     &s,
    //     H2O_OPTION_THREAD_POOL_SIZE, get_nprocs(),
    //     H2O_OPTION_END);
    // else
    //   d = H2O_start_daemon(
    //     flags,
    //     port,
    //     NULL,
    //     NULL,
    //     &h2o_handler<S>,
    //     &s,
    //     H2O_OPTION_END);
      
    
    // if (d == NULL)
    //   return;

    // while (getc (stdin) != 'q');
    // H2O_stop_daemon(d);
  }
  

}
