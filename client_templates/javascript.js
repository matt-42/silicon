

function silicon_api_base()
{
  this.server_url = "/";
  this.set_server = function(url) { this.server_url = url; }
  this.call_procedure = function(proc_url, params, return_type)
  {
    // Return a new promise.
    var _this = this;
    return new Promise(function(resolve, reject) {
      // Do the usual XHR stuff
      var req = new XMLHttpRequest();
      req.open('POST', _this.server_url + "/" + proc_url);

      req.onload = function() {
        // This is called even on 404 etc
        // so check the status
        if (req.status == 200) {
          // Resolve the promise with the response text
          switch (return_type) {
          case "object": resolve(JSON.parse(req.response)); break;
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

var {{root_scope}} = new silicon_api_base();

{{scope}}
  if ( ! {{scope_path}} ) {{scope_path}} = {};
  {{procedure}}
    // {{procedure_description}}
{{procedure_path}} = function(params) { return {{root_scope}}.call_procedure("{{procedure_url}}", params, '{{return_type}}'); };
  {{end_procedure}}

  {{child_scopes}}
{{end_scope}}
