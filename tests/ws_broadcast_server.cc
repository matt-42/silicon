#include <silicon/api.hh>
#include <silicon/remote_api.hh>
#include <silicon/backends/websocketpp.hh>
#include <silicon/clients/javascript_client.hh>
#include "symbols.hh"

using namespace s;

using namespace sl;

// The chat room manage the list of connections.
struct chat_room
{
  typedef std::unique_lock<std::mutex> lock;

  chat_room()
    : users_mutex(new std::mutex),
      users(new std::set<wspp_connection>)
  {}

  void remove(wspp_connection c) { lock l(*users_mutex); users->erase(c); }
  void add(wspp_connection c) { lock l(*users_mutex); users->insert(c); }

  template <typename F>
  void foreach(F f) {
    for(auto& u : *users) f(u);
  }

private:
  std::shared_ptr<std::mutex> users_mutex;
  std::shared_ptr<std::set<wspp_connection>> users;
};

// The html page
std::string index_html_source = R"html(
<html>
  <head>
    
    <title>Silicon Broadcast</title>
    <style>
      body { margin: 0; padding: 0; }
      #content { overflow:auto; height: 80%; overflow: auto; width: 100%; padding: 10px; }
      #form { padding: 10px; width: 100%; }
      #message_input { width: 95%; height: 30px; }
    </style>
    <script src="/js_client" type="text/javascript"></script>
  </head>
<body>
<div id="content"></div>
<form id="form"><input autocomplete=off autofocus id="message_input"/></form>
</body>

<script>
  var messages_div = document.getElementById('content');
  var input = document.getElementById('message_input');
  var ws = new silicon_json_websocket('ws://' + window.location.host);

  ws.api.message = function (m) { 
    var n = document.createElement('div');
    n.appendChild(document.createTextNode(m.text));
    messages_div.appendChild(n);
    messages_div.scrollTop = messages_div.scrollHeight;
  }

  document.getElementById('form').onsubmit = function (e) {
    if (input.value.length > 0) ws.broadcast({message: input.value });
    input.value = "";
    return false;
  };

</script>
)html";

int main(int argc, char* argv[])
{
  using namespace sl;

  // The remote client api accessible from this server.
  auto client_api = make_remote_api( _message(_text) );
  
  // The type of a client to call the remote api.
  typedef ws_client<decltype(client_api)> client;

  // The server websocket api accessible by the client.
  auto server_api = make_api(

    // Broadcast a message to all clients.
    _broadcast(_message) = [] (auto p, client& rclient, chat_room& room) {
      room.foreach([&] (wspp_connection h) { rclient(h).message(p.message); });      
    }
    
    ).bind_middlewares(chat_room());

  // on_open, on_close handlers.
  auto on_open_handler = [] (wspp_connection hdl, chat_room& r) { r.add(hdl); };
  auto on_close_handler = [] (wspp_connection hdl, chat_room& r) { r.remove(hdl); };
  
  // Generate javascript client.
  std::string js_client = generate_javascript_websocket_client(server_api);

  // Serve the js and html code via classic http routes.
  auto http_api = make_api(
    _js_client = [&] () { return js_client; },
    _home = [&] () { return index_html_source; });

  // Start the websocketpp server.
  wspp_json_serve(server_api, client_api, atoi(argv[1]),
                  _on_open = on_open_handler,
                  _on_close = on_close_handler,
                  _http_api = http_api);

}
