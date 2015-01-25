#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <silicon/symbols.hh>
#include <silicon/error.hh>
#include <silicon/ws_service.hh>
#include <silicon/websocketpp_remote_client.hh>

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

    template <typename T>
    auto serialize(response_type& r, const T& res) const
    {
      r = json_encode(res);
    }

  };

  template <typename C>
  struct ws_clients
  {
    typedef websocketpp::connection_hdl connection_hdl;

    ws_clients(const C& client_api, wspp_server* s) : api(client_api), server(s) {}

    auto find(connection_hdl c)
    {
      auto it = clients_.find(c);
      if (it == clients_.end())
        throw std::runtime_error("ws_clients::find The connection handler does not exists.");

      return make_ws_remote_client(api, *server, *it);
    }
    
    void add(const connection_hdl& c)
    {
      if (clients_.find(c) != clients_.end())
        throw std::runtime_error("ws_clients::add The connection handler already exists.");

      clients_.insert(c);
    }

    void remove(const connection_hdl& c)
    {
      auto it = clients_.find(c);
      if (it == clients_.end())
        throw std::runtime_error("ws_clients::erase The connection handler does not exists.");

      clients_.erase(it);
    }

    template <typename F>
    void operator|(F f)
    {
      for(auto hdl : clients_)
      {
        auto c = make_ws_remote_client(api, *server, hdl);
        di_call(f, hdl, c);
      }
    }

    C api;
    wspp_server* server;
    std::set<connection_hdl> clients_;
  };

  // template <typename D>
  // struct ws_session_middlewaree
  // {

  //   static D* instantiate(connection_hdl c)
  //   {
  //     auto it = sessions.find(c);
  //     if (it != sessions.end()) return &(it->second);
  //     else
  //     {
  //       std::unique_lock<std::mutex> l(sessions_mutex);
  //       return &(sessions[c]);
  //     }
  //   }

  //   static D* find(std::string n)
  //   {
  //     for (auto& it : sessions) if (it.second.nickname == n) return &it.second;
  //     return 0;
  //   }

  //   std::string nickname;

  // private:
  //   static std::mutex sessions_mutex;
  //   static std::map<connection_hdl, D> sessions;
  // };
  
  template <typename A1, typename A2>
  void websocketpp_json_serve(const A1& server_api, const A2& remote_client_api, int port)
  {
    using websocketpp::connection_hdl;
    typedef wspp_server::message_ptr message_ptr;
    typedef decltype(make_ws_remote_client(remote_client_api)) client_type;
    typedef ws_clients<A2> clients_type;
    wspp_server server;
    clients_type clients(remote_client_api, &server);

    auto service = ws_service<websocketpp_json_service_utils, A1, client_type, clients_type, connection_hdl>(server_api);
    


    std::mutex connections_mutex;
    std::mutex messages_mutex;
    typedef std::unique_lock<std::mutex> lock_type;
    struct message { connection_hdl hdl; message_ptr msg; };
    std::queue<message> messages;

    std::condition_variable messages_condition;
    
    auto on_open = [&] (connection_hdl hdl)
    {
      lock_type lock(connections_mutex);
      clients.add(hdl);
    };

    auto on_close = [&] (connection_hdl hdl)
    {
      lock_type lock(connections_mutex);
      clients.remove(hdl);
    };

    auto on_message = [&] (connection_hdl hdl, message_ptr msg)
    {
      lock_type lock(messages_mutex);
      messages.push(message{hdl, msg});
      lock.unlock();
      messages_condition.notify_one();
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

        connection_hdl client = m.hdl;
        std::string str = m.msg->get_payload();

        int i = 0;
        while (std::isdigit(str[i]) and i < str.size()) i++;
        std::string request_id = str.substr(0, i);
        int j = i;
        while (str[j] != '{' and j < str.size()) j++;
        
        std::string location = str.substr(i, j - i);
        std::string request = str.substr(j, str.size() - j);


        auto send_response = [&] (int status, std::string body) {
          std::stringstream ss;
          ss << request_id << ":" << status << ":" << body;
          server.send(m.hdl, ss.str(), websocketpp::frame::opcode::text);
        };
        try
        {
          auto c = clients.find(m.hdl);
          std::string response;
          service(location, request, response, c, clients, m.hdl);
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
