#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace iod
{

  template <typename RQ, typename RS>
  struct websocketpp_json_service_utils
  {
    typedef std::string request_type;
    typedef decltype(D(_Status = int(), _Body = std::string())) response_type;

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
      std::string str = json_encode(res);
      r.status = 200;
      r.body = json_encode(res);
    }

  };

  template <typename S, typename RQ, typename RS>
  struct websocketpp_handler {

    typedef RQ request_type;
    typedef RS response_type;
    
    websocketpp_handler(S service) : service_(service) {}

    void operator() (request_type& request,
                     response_type& response)
    {

      try
      {
        service_(std::string(request.destination), request, response);
      }
      catch(const error::error& e)
      {
        auto s = (typename response_type::status_type)e.status();
        response = response_type::stock_reply(s, e.what());
      }
      catch(const std::runtime_error& e)
      {
        std::cout << e.what() << std::endl;
      }
    }
      
    void log(std::string const &info) {
      std::cerr << "ERROR: " << info << '\n';
    }

  private:
    S service_;
    
  };

  template <typename A>
  void websocketpp_json_serve(const A1& server_api, const A2& client_api, int port)
  {
    using websocketpp::connection_hdl;

    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;

    server server;
    con_list connections;

    std::mutex messages_mutex;
    std::queue<messages> messages;
    
    auto on_open = [&] (connection_hdl hdl)
    {
      std::unique_lock lock(connections_mutex);
      connections.insert(hdl);
    };

    auto on_close = [&] (connection_hdl hdl)
    {
      std::unique_lock lock(connections_mutex);
      connections.erase(hdl);
    };

    auto on_message = [&] (connection_hdl hdl, server::message_ptr msg)
    {
      std::unique_lock lock(messages_mutex);
      messages.push(message{hdl, msg});
    };

    auto process_messages = [&] ()
    {
      while (true)
      {
        std::unique_lock<std::mutex> lock(messages_mutex);
        while (messages.empty()) messages_condition.wait(lock);

        message m = messages.front();
        connection_hdl client = m.hdl();
        std::string str = m.to_string();
        int i = 0;
        while (str[i] != '|' and i < str.size()) i++;

        std::string location = str.substr(0, i);
        std::string request = str.substr(i, str.size() - i);

        try
        {
          auto response = D(_Status = int(),
                            _Id = request_id,
                            _Body = std::string());
          _this->service_(request.location(), request, response);
          if (response.body.size() != 0)
            server.send(client, json_encode(response));
        }
        catch(const error::error& e)
        {
          std::string str = json_encode(D(_Status = e.status(), _Body = e.what));
          server.send(client, str);
        }
        catch(const std::runtime_error& e)
        {
          std::string str = json_encode(D(_Status = 500, _Body = e.what()));
          server.send(client, str);
        }
        
      }
    }
    
    server.init_asio();

    // Register handler callbacks
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;
    
    server.set_open_handler(bind(on_open, _1));
    server.set_close_handler(bind(on_close, _1));
    server.set_message_handler(bind(on_message, _1, _2));

    server.listen(port);
    server.start_accept();

    try
    {
      server.run();
    }
    catch (const std::exception & e)
    {
      std::cout << e.what() << std::endl;
    }
    catch (websocketpp::lib::error_code e)
    {
      std::cout << e.message() << std::endl;
    }
    catch (...) {
      std::cout << "Error: Unknown exception" << std::endl;
    }

  }

}
