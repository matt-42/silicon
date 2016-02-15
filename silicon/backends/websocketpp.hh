#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <silicon/api.hh>
#include <silicon/symbols.hh>
#include <silicon/error.hh>
#include <silicon/service.hh>
#include <silicon/backends/websocketpp_remote_client.hh>
#include <silicon/backends/wspp_connection.hh>

namespace sl
{
  using ::s::_status;
  using ::s::_body;
  using ::s::_id;

  typedef websocketpp::server<websocketpp::config::asio> wspp_server;

  struct wspp_response { std::string body; };
  struct wspp_request { std::string body; };

  struct websocketpp_json_service_utils
  {
    typedef wspp_request request_type;
    typedef wspp_response response_type;

    template <typename P, typename T>
    auto deserialize(request_type* r, P procedure, T& res) const
    {
      try
      {
        if (r->body.size() > 0)
          json_decode<typename P::route_type::parameters_type>(res, r->body);
        else
          json_decode<typename P::route_type::parameters_type>(res, "{}");
      }
      catch (const std::runtime_error& e)
      {
        throw error::bad_request("Error when decoding procedure arguments: ", e.what());
      }
      
    }

    inline auto serialize2(response_type* r, const std::string& res) const
    {
      r->body = res;
    }
    
    template <typename T>
    auto serialize2(response_type* r, const T& res) const
    {
      r->body = json_encode(res);
    }

    template <typename T>
    auto serialize(response_type* r, const T& res) const
    {
      serialize2(r, res);
    }
    
  };

  template <typename A1, typename M, typename... OPTS>
  void wspp_json_serve(const A1& server_api, M middleware_factories, int port, OPTS&&... opts)
  {
    using websocketpp::connection_hdl;
    typedef wspp_server::message_ptr message_ptr;

    auto options = D(opts...);
    auto on_close_handler = options.get(_on_close, [] () {});
    auto on_open_handler = options.get(_on_open, [] () {});
    auto on_http_api = options.get(_http_api, http_api());

    wspp_server server;

    auto ws_service = service<websocketpp_json_service_utils, A1, M,
                              wspp_request*, wspp_response*,
                              wspp_connection>(server_api, middleware_factories);

    auto http_service = service<websocketpp_json_service_utils,
                                decltype(on_http_api), M,
                                wspp_request*, wspp_response*>(on_http_api, middleware_factories);

    std::mutex connections_mutex;
    std::mutex messages_mutex;
    typedef std::unique_lock<std::mutex> lock_type;
    struct message { connection_hdl hdl; message_ptr msg; };
    std::queue<message> messages;

    std::condition_variable messages_condition;
    
    auto on_close = [&] (connection_hdl hdl)
    {
      wspp_connection c{hdl, &server};
      di_factories_call(on_close_handler, middleware_factories, c);
    };

    auto on_open = [&] (connection_hdl hdl)
    {
      wspp_connection c{hdl, &server};
      di_factories_call(on_open_handler, middleware_factories, c);
    };

    auto on_message = [&] (connection_hdl hdl, message_ptr msg)
    {
      lock_type lock(messages_mutex);
      messages.push(message{hdl, msg});
      lock.unlock();
      messages_condition.notify_one();
    };

    auto on_http = [&] (connection_hdl hdl)
    {
      wspp_server::connection_ptr con = server.get_con_from_hdl(hdl);
      try
      {
        wspp_request request;
        wspp_response response;
        // Only http GET requests.
        http_service(std::string("/GET") + con->get_resource(),
                     &request, &response);
        con->set_status(websocketpp::http::status_code::ok);
        con->set_body(response.body);
      }
      catch(const error::error& e)
      {
        auto s = (typename websocketpp::http::status_code::value)e.status();
        con->set_status(s);
        con->set_body(e.what());
      }
    };
    
    auto process_messages = [&] ()
    {
      while (true)
      {
        lock_type lock(messages_mutex);
        while (messages.empty()) messages_condition.wait(lock);
        
        message m = messages.front();
        messages.pop();
        lock.unlock();

        wspp_connection connection{m.hdl, &server};
        std::string str = m.msg->get_payload();

        int i = 0;
        while (str[i] != ':' and i < str.size()) i++;
        std::string request_id = str.substr(0, i);
        int j = i + 1;
        while (str[j] != ':' and j < str.size()) j++;
        
        std::string location = str.substr(i + 1, j - i - 1);
        wspp_request request;
        request.body = str.substr(j + 1, str.size() - j - 1);

        auto send_response = [&] (int status, std::string body) {
          std::stringstream ss;
          ss << request_id << ":" << status << ":" << body;
          server.send(m.hdl, ss.str(), websocketpp::frame::opcode::text);
        };
        try
        {
          wspp_response response;
          ws_service(location, &request, &response, connection);
          if (response.body.size() != 0)
            send_response(200, response.body);
        }
        catch(const error::error& e)
        {
          std::cerr << e.what() << std::endl;
          send_response(e.status(), e.what());
        }
        catch(const std::runtime_error& e)
        {
          std::cerr << e.what() << std::endl;
          send_response(500, "Internal server error");
        }
        
      }
    };


    server.clear_access_channels(websocketpp::log::alevel::all);
    server.init_asio();

    // Register handler callbacks
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;
    
    server.set_open_handler(bind<void>(on_open, _1));
    server.set_close_handler(bind<void>(on_close, _1));
    server.set_message_handler(bind<void>(on_message, _1, _2));
    server.set_http_handler(bind<void>(on_http, _1));

    server.listen(port);
    server.start_accept();

    boost::asio::signal_set signals(server.get_io_service(), SIGINT, SIGTERM);
    auto stop = [&] (const boost::system::error_code& error, int signal_number) {
      server.stop();
      exit(0);
    };
    signals.async_wait(stop);

    try
    {
      std::thread worker(process_messages);
      server.run();
      worker.join();
    }
    catch (const std::exception & e)
    {
      std::cerr << e.what() << std::endl;
    }
    catch (websocketpp::lib::error_code e)
    {
      std::cerr << e.message() << std::endl;
    }
    catch (...) {
      std::cout << "Error: Unknown exception" << std::endl;
    }

  }

  template <typename A, typename... O>
  auto wspp_json_serve(const A& api, int port, O&&... opts)
  {
    return wspp_json_serve(api, std::make_tuple(), port, opts...);
  }
  
}

