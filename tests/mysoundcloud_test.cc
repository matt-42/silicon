  // auto call = [&] (std::string f, auto params) {

  //   std::stringstream ss;
  //   ss << "http://127.0.0.1:9999/__" << server.procedure_index(f);
  //   client::request request_(ss.str());
  //   request_ << header("Connection", "close");
  //   client client_;
  //   client::response response_ = client_.get(request_);

  //   auto headers = headers(response_);
  //   std::string body = body(response_);

  //   return D(_Body = body(response_),
  //            _Status = status(response_));
  // };


using namespace boost::network;
using namespace boost::network::http;

silicon_make_client(S& server)
{
  return foreach(server.api()) | [] (auto& f)
  {
    return f.symbol() = create_client_call(get_procedure_args(f),
                                           get_procedure_return_type(f));
  };
}

int main()
{
  auto server = mysoundcloud_server();
  std::thread t([&] () { server.serve(9999); });

  auto client = silicon_make_client_from_server(mysoundcloud_server(), @async);
  client.set_server("127.0.0.1:9999");

  client.signup(@email = "user@google.com", @password = "password");
  client.login(@email = "user@google.com", @password = "password");

  auto res = client.song.create(@title = "my_new_song", @filename = "my_new_song.mp3");
  std::cout << res.id << std::endl;

  client.logout();

  assert(response.status == 200);

  client
  expect(res).status(200);
  expect(res)
  assert
  // Scenario check:
  // If logged, then ...
  client.logout();
  client.song.update_song().status(401);
  
  server.call_procedure("logout")();
}
