
#include <silicon/server.hh>

namespace iod
{
  std::string javascript_api_base();

  template <typename S, typename... T>
  std::string generate_javascript_client(const S& s, T... _options)
  {
    auto opts = D(_options...);
    std::string module = opts.get(_Module, "sl");

    auto desc = server_api_description(s);


    std::ostringstream calls;

    auto handlers = s.get_handlers();
    for (unsigned i = 0; i < desc.size(); i++)
    {
      std::string ret = desc[i].return_type;
      if (ret[0] == '{') ret = "json";

      calls << "// ";
      print_procedure_desc(calls, desc[i]);
      calls << std::endl;
      calls << module << "." << desc[i].name << " = function(params) { return this.call_procedure(" << i << ", params, '" << ret << "'); }" << std::endl;
      calls << std::endl;
    }


    std::ostringstream src;

    src << javascript_api_base();
    src << "var " << module << " = new silicon_api_base();" << std::endl;
    src << calls.str();

    return src.str();
  }
  
  std::string javascript_api_base()
  {
    return R"javascript(

function silicon_api_base()
{
  this.server_url = "/";
  this.set_server = function(url) { this.server_url = url; }
  this.call_procedure = function(id, params, return_type)
  {
    // Return a new promise.
    var _this = this;
    return new Promise(function(resolve, reject) {
      // Do the usual XHR stuff
      var req = new XMLHttpRequest();
      req.open('POST', _this.server_url + "__" + id);

      req.onload = function() {
        // This is called even on 404 etc
        // so check the status
        if (req.status == 200) {
          // Resolve the promise with the response text
          switch (return_type) {
          case "int":    resolve(parseInt(req.response)); break;
          case "float":  resolve(parseFloat(req.response)); break;
          case "json":   resolve(JSON.parse(req.response)); break;
          default: resolve(req.response); break;
          }
        }
        else {
          // Otherwise reject with the request object.
          reject(req);
        }
      };

      // Handle network errors
      req.onerror = function() {
        reject(Error("Network Error"));
      };

      // Make the request
      if (typeof params == 'object')
        req.send(JSON.stringify(params));
      else req.send(params);
    });
  }

}

)javascript";
    
  }
}
