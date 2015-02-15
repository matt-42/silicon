#include <silicon/api.hh>
#include <silicon/remote_api.hh>
#include <silicon/backends/websocketpp.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

struct user { std::string nickname; };

struct chat_room
{
  typedef std::unique_lock<std::mutex> lock;

  chat_room()
    : users_mutex(new std::mutex),
      users(new std::map<wspp_connection, user>)
  {}

  void remove(wspp_connection c) { lock l(*users_mutex); users->erase(c); }
  void add(wspp_connection c) { lock l(*users_mutex); (*users)[c]; }
  
  user& find_user(wspp_connection c)
  {
    auto it = users->find(c);
    if (it != users->end()) return it->second;
    else throw error::bad_request("Cannot find this user.");
  }

  wspp_connection find_connection(std::string n)
  {
    auto u = users->end();
    for (auto it = users->begin(); it != users->end(); it++)
      if (it->second.nickname == n) { u = it; break; }

    if (u == users->end()) throw error::bad_request("The user ", n, " does not exists.");
    return u->first;
  }

  bool nickname_exists(std::string n)
  {
    for (auto& it : *users)
      if (it.second.nickname == n) return true;
    return false;
  }

  template <typename F>
  void foreach(F f) {
    for(auto& u : *users)
      di_call(f, wspp_connection(u.first), u.second);
  }

private:
  std::shared_ptr<std::mutex> users_mutex;
  std::shared_ptr<std::map<wspp_connection, user>> users;
};

int main(int argc, char* argv[])
{
  using namespace sl;

  // The remote client api accessible from this server.
  auto client_api = make_remote_api(   _broadcast(_from, _text), _pm(_from, _text)  );
  
  // The type of a client to call the remote api.
  typedef ws_client<decltype(client_api)> client;

  // The server api accessible by the client.
  auto server_api = make_api(

    // Set nickname.
    _nick(_nick) = [] (auto p, wspp_connection hdl, chat_room& room) {
      while(room.nickname_exists(p.nick)) p.nick += "_";
      room.find_user(hdl).nickname = p.nick;
      return D(_nick = p.nick);
    },

    // Broadcast a message to all clients.
    _broadcast(_message) = [] (auto p, wspp_connection hdl, client& rclient, chat_room& room) {
      auto from = room.find_user(hdl);
      room.foreach([&] (wspp_connection h) { rclient(h).broadcast(from.nickname, p.message); });      
    },

    // Private message.
    _pm(_to, _message) = [] (auto p, wspp_connection hdl, client& rclient, chat_room& room) {

      user from = room.find_user(hdl);
      rclient(room.find_connection(p.to)).pm(from.nickname, p.message);
    }
    
    ).bind_middlewares(chat_room());

    
  auto on_open_handler = [] (wspp_connection& hdl, chat_room& r) { r.add(hdl); };
  auto on_close_handler = [] (wspp_connection hdl, chat_room& r) { r.remove(hdl); };

  wspp_json_serve(server_api, client_api, atoi(argv[1]),
                  _on_open = on_open_handler,
                  _on_close = on_close_handler);

}
