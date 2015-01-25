function silicon_json_websocket(url)
{
  var ws = new WebSocket(url);
  var on_open_resolve;
  var on_open_promise = new Promise(function (resolve, reject)
                                    { on_open_resolve = resolve; });

  ws.onopen = function() {
    on_open_resolve();
  };
  
  this.on_open = function(f) { on_open_promise.then(f); }
  
  var parse_request = function (s) {
    // Request: req_id:/route:body
    var t = s.split(':')
    return {
      id: t[0],
      route: t[1].substr(1, t[1].length - 1).split('/'),
      params: JSON.parse(t.slice(2).join(':'))
    };
  }

  var request_id = 1;
  var promises = {};

  // Client -> Server communication
  var send_request = function(route, params)
  {
    var p = new Promise(function (resolve, reject)
                {
                  promises[request_id] = { resolve: resolve, reject: reject };
                });
    ws.send(request_id + ":/" + route + ":"+ JSON.stringify(params));
    request_id++;
    return p;
  }

  this.api = {
    broadcast: function (p) { console.log("Broadcast" + p.from + " " + p.text); },
    pm: function (p) { console.log("Pm" + p.from + " " + p.text); },
  }
  var api = this.api;
  // Server -> Client communication
  ws.onmessage = function(event) {
    var req = parse_request(event.data);
    if (req.id.length == 0) // Server -> client call.
    {
      var node = api;
      for (var i = 0; i < req.route.length && node != undefined; i++)
        node = node[req.route[i]];
      if (node) node(req.params);
      else console.error("Procedure " + req.route.join('.') + " is not implemented.")
    }
    else // Server response of a client call.
    {
      var id = parseInt(req.id);
      if (req.route == "200")
        promises[id].resolve(req.params);
      else
        promises[id].reject(req.params);
      promises[id] = null;
    }
  };

  var {{root_scope}} = this;
  {{scope}}
    if ( ! {{scope_path}} ) {{scope_path}} = {};
    {{procedure}}
      // {{procedure_description}}
      {{procedure_path}} = function(params) { return send_request("{{procedure_url}}", params); };
    {{end_procedure}}
    {{child_scopes}}
  {{end_scope}}

}
