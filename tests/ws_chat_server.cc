#include <silicon/api.hh>
#include <silicon/remote_api.hh>
#include <silicon/websocketpp.hh>

using websocketpp::connection_hdl;

struct session
{

  static session* instantiate(connection_hdl c)
  {
    auto it = sessions.find(c);
    if (it != sessions.end()) return &(it->second);
    else
    {
      std::unique_lock<std::mutex> l(sessions_mutex);
      return &(sessions[c]);
    }
  }

  static session* find(std::string n)
  {
    for (auto& it : sessions) if (it.second.nickname == n) return &it.second;
    return 0;
  }

  std::string nickname;

private:
  static std::mutex sessions_mutex;
  static std::map<connection_hdl, session> sessions;
};

std::mutex session::sessions_mutex;
std::map<connection_hdl, session> session::sessions;

int main(int argc, char* argv[])
{
  using namespace sl;

  // The remote client api accessible from this server.
  auto client_api = make_remote_api(   @broadcast(@from, @text), @pm(@from, @text)  );

  // The type of a client to call the remote api.
  typedef ws_client<decltype(client_api)> client;
  // The list of client.
  typedef ws_clients<decltype(client_api)> clients;

  // The server api accessible by the client.
  auto server_api = make_api(

    // Set nickname.
    @nick(@nick) = [] (auto p, session* s, client& c) {
      while(session::find(p.nick)) p.nick += "_";
      s->nickname = p.nick;
      return D(@nick = s->nickname);
    },

    // Broadcast a message to all clients.
    @broadcast(@message) = [] (auto p, session* s, clients& cs) {
      cs | [&] (client& c) { c.broadcast(s->nickname, p.message); };
    },

    // Private message.
    @pm(@to, @message) = [] (auto p, session* s, clients& cs) {
      cs | [&] (client& c2, session* s2) {
        if (s2->nickname == p.to)
          c2.pm(s->nickname, p.message);
      };
    }
    
    );

  websocketpp_json_serve(server_api, client_api, atoi(argv[1]));

}
