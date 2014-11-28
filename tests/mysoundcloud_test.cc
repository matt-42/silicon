

using namespace boost::network;
using namespace boost::network::http;

int main()
{
  auto server = mysoundcloud_server();

  std::thread t([&] () { server.serve(9999); });

  auto call = [&] (std::string f, auto params) {

    std::stringstream ss;
    ss << "http://127.0.0.1:9999/__" << server.procedure_index(f);
    client::request request_(ss.str());
    request_ << header("Connection", "close");
    client client_;
    client::response response_ = client_.get(request_);

    auto headers = headers(response_);
    std::string body = body(response_);

    return D(_Body = body(response_),
             _Status = status(response_));
  };

  // Unit check.
  check("login", D(:username = "matt")).status(200).json_body_has_keys(:id);

  auto actions = D(
    :login = D(:username = "matt");
    :logout,
    
    );
    
  // Scenario check:
  // If logged, then ...
  actions.logout();
  actions.update_song().status(401);
  
  server.call_procedure("logout")();
}
