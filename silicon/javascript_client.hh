
#include <silicon/server.hh>

iod_define_symbol(module, _Module);

namespace iod
{

  template <typename... T>
  std::string generate_javascript_client(const server& s, T... _options)
  {
//     std::string prolog = R"json(

//   var server_url = "http://localhost:8888";
//   this.call_procedure = function(id, params)
//   {
//     // Return a new promise.
//     var _this = this;
//     return new Promise(function(resolve, reject) {
//       // Do the usual XHR stuff
//       var req = new XMLHttpRequest();
//       req.open('POST', _this.server_url);

//       req.onload = function() {
//         // This is called even on 404 etc
//         // so check the status
//         if (req.status == 200) {
//           // Resolve the promise with the response text
//           resolve(req.response);
//         }
//         else {
//           // Otherwise reject with the status text
//           // which will hopefully be a meaningful error
//           reject(Error(req.statusText));
//         }
//       };

//       // Handle network errors
//       req.onerror = function() {
//         reject(Error("Network Error"));
//       };

//       // Make the request
//       body = JSON.stringify({ handler_id: id}) + JSON.stringify(params);
//       req.send(body);
//     });
//   }


// )json";

    auto opts = D(_options...);

    std::ostringstream calls;

    auto handlers = s.get_handlers();
    for (int i = 0; i < handlers.size(); i++)
    {
      auto h = handlers[i];
      //visit(h)
      calls << "  this." << h->name_ << " = function(params) { return this.call_procedure(" << i << ", params); }" << std::endl;
    }

    std::string module = opts.get(_Module, "sl");

    std::ostringstream src;

    src << "function " << module << "() {" << std::endl;
    //src << prolog;
    src << calls.str();
    src << "}";

    return src.str();
  }
}
