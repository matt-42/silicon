
#include <silicon/server.hh>

iod_define_symbol(module, _Module);

namespace iod
{
  std::string javascript_call_procedure();

  template <typename S, typename... T>
  std::string generate_javascript_client(const S& s, T... _options)
  {
    auto desc = server_api_description(s);


    auto opts = D(_options...);

    std::ostringstream calls;

    auto handlers = s.get_handlers();
    for (int i = 0; i < desc.size(); i++)
    {
      //visit(h)
      calls << "  // ";
      print_procedure_desc(calls, desc[i]);
      calls << std::endl;
      calls << "  this." << desc[i].name << " = function(params) { return this.call_procedure(" << i << ", params); }" << std::endl;
      calls << std::endl;
    }

    std::string module = opts.get(_Module, "sl");

    std::ostringstream src;

    src << "function " << module << "() {" << std::endl;
    src << javascript_call_procedure();
    src << calls.str();
    src << "}" << std::endl;

    return src.str();
  }
  
  std::string javascript_call_procedure()
  {
    return R"javascript(
  var server_url = "/";
  this.set_server = function(url) { server_url = url; }
  this.call_procedure = function(id, params)
  {
    // Return a new promise.
    var _this = this;
    return new Promise(function(resolve, reject) {
      // Do the usual XHR stuff
      var req = new XMLHttpRequest();
      req.open('POST', _this.server_url);

      req.onload = function() {
        // This is called even on 404 etc
        // so check the status
        if (req.status == 200) {
          // Resolve the promise with the response text
          resolve(req.response);
        }
        else {
          // Otherwise reject with the status text
          // which will hopefully be a meaningful error
          reject(Error(req.statusText));
        }
      };

      // Handle network errors
      req.onerror = function() {
        reject(Error("Network Error"));
      };

      // Make the request
      body = JSON.stringify({ handler_id: id}) + JSON.stringify(params);
      req.send(body);
    });
  }


)javascript";
    
  }
}
