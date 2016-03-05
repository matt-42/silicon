

function silicon_api_base()
{
  this.server_url = "";
  this.set_server = function(url)
  {
    if (url.slice(-1) != "/")
      this.server_url = url;
    else
      this.server_url = url.slice(0, -1);      
  }

  this.call_procedure = function(method, proc_url, params, params_schema, return_type)
  {
    // Return a new promise.
    var _this = this;
    return new Promise(function(resolve, reject) {
      // Do the usual XHR stuff
      var req = new XMLHttpRequest();

      var url = _this.server_url;

      var error = "";

      // Url parameters
      for (var k in proc_url)
      {
        if (!proc_url.hasOwnProperty(k)) continue;
        if (proc_url[k].is_param)
        {
          if (!params.hasOwnProperty(k))
            error += "Missing required URL parameter " + k + "\n";
          else
            url += "/" + encodeURIComponent(String(params[k]))
        }
        else
          url += "/" + k;
      }
      
      // Get parameters
      var first = true;
      for (var k in params_schema.get)
      {
        if (!params_schema.get.hasOwnProperty(k)) continue;
        if (first) url += "?"; else url += "&";
        first = false;
        if (!params.hasOwnProperty(k) && !params_schema.get[k].optional)
          error += "Missing required POST parameter " + k + "\n";
        else
        {
          if (typeof(params[k]) == "object")
            url += k + "=" + encodeURIComponent(JSON.stringify(params[k]));
          else
            url += k + "=" + encodeURIComponent(params[k]);
        }
      }

      // Post parameters
      var post_string = ""
      first = true;
      for (var k in params_schema.post)
      {
        if (!params_schema.post.hasOwnProperty(k)) continue;
        if (!first) url += "&";
        first = false;
        if (!params.hasOwnProperty(k) && !params_schema.post[k].optional)
          error += "Missing required POST parameter " + k + "\n";
        else
        {
          if (typeof(params[k]) == "object")
            post_string += k + "=" + encodeURIComponent(JSON.stringify(params[k]));
          else
            post_string += k + "=" + encodeURIComponent(params[k]);
        }
      }

      if (error != "")
      {
        reject(Error(error));
        return;
      }

      // If no error, issue the request.
      req.open(method, url, true);

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

      // Send the post params
      req.send(post_string);
    });
  }

}

var {{root_scope}} = new silicon_api_base();

{{scope}}
  if ( ! {{scope_path}} ) {{scope_path}} = {};
  {{procedure}}
    // {{procedure_description}}
{{procedure_path}} = function(params) { return {{root_scope}}.call_procedure('{{http_method}}', {{procedure_url}}, params, {{params_schema}}, '{{return_type}}'); };
  {{end_procedure}}

  {{child_scopes}}
{{end_scope}}
