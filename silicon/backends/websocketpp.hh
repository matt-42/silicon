#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
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
  
  struct websocketpp_json_service_utils
  {
    typedef std::string request_type;
    typedef std::string response_type;

    template <typename T>
    auto deserialize(const request_type& r, T& res) const
    {
      try
      {
        json_decode(res, r);
      }
      catch (const std::runtime_error& e)
      {
        throw error::bad_request("Error when decoding procedure arguments: ", e.what());
      }

    }

    inline auto serialize(response_type& r, const std::string& res) const
    {
      r = res;
    }
    
    template <typename T>
    auto serialize(response_type& r, const T& res) const
    {
      r = json_encode(res);
    }

  };

  template <typename A1, typename A2, typename... OPTS>
  void wspp_json_serve(const A1& server_api, const A2& remote_client_api, int port, OPTS&&... opts)
  {
    using websocketpp::connection_hdl;
    typedef wspp_server::message_ptr message_ptr;
    auto rclient = make_ws_remote_client(remote_client_api);
    typedef decltype(rclient) client_type;

    auto options = D(opts...);
    auto on_close_handler = options.get(_on_close, [] () {});
    auto on_open_handler = options.get(_on_open, [] () {});
    auto http_api = options.get(_http_api, make_api());

    wspp_server server;

    auto ws_service = service<websocketpp_json_service_utils, A1, client_type, wspp_connection>(server_api);
    auto http_service = service<websocketpp_json_service_utils, decltype(http_api)>(http_api);

    std::mutex connections_mutex;
    std::mutex messages_mutex;
    typedef std::unique_lock<std::mutex> lock_type;
    struct message { connection_hdl hdl; message_ptr msg; };
    std::queue<message> messages;

    std::condition_variable messages_condition;
    
    auto on_close = [&] (connection_hdl hdl)
    {
      wspp_connection c{hdl, &server};
      di_middlewares_call(on_close_handler, ws_service.api().middlewares(), c);
    };

    auto on_open = [&] (connection_hdl hdl)
    {
      wspp_connection c{hdl, &server};
      di_middlewares_call(on_open_handler, ws_service.api().middlewares(), c);
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
        std::string request, response;
        http_service(con->get_resource(), request, response);
        con->set_status(websocketpp::http::status_code::ok);
        con->set_body(response);
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
        std::string request = str.substr(j + 1, str.size() - j - 1);

        auto send_response = [&] (int status, std::string body) {
          std::stringstream ss;
          ss << request_id << ":" << status << ":" << body;
          server.send(m.hdl, ss.str(), websocketpp::frame::opcode::text);
        };
        try
        {
          std::string response;
          ws_service(location, request, response, rclient, connection);
          if (response.size() != 0)
            send_response(200, response);
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

}
