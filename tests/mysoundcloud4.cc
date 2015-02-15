#include <thread>
#include <iostream>
#include <iod/sio_utils.hh>
#include <silicon/sqlite.hh>
#include <silicon/sqlite_orm.hh>
#include <silicon/sqlite_session.hh>
#include <silicon/api.hh>
#include <silicon/mimosa.hh>
#include <silicon/client.hh>
#include <silicon/crud.hh>
#include <silicon/hash.hh>
#include <silicon/javascript_client.hh>
#include "symbols.hh"

using namespace s;

using namespace iod;

// Models
typedef decltype(iod::D(_id(_primary_key) = int(),
                        _email = std::string(),
                        _password = blob()
                   )) User;

typedef decltype(iod::D(_id(_primary_key) = int(),
                        _user_id(_computed) = int(),
                        _title = std::string(),
                        _filename = std::string()
                   )) Song;


// ==================================================
// Session
struct session_data
{
  session_data() { user_id = -1; }
  bool is_authenticated() const { return user_id != -1; }

  auto sio_info() { return D(_user_id = int()); }
  
  int user_id;
};

typedef sqlite_session<session_data> session;

// ==================================================
// Authentication.
struct authenticator
{
  bool login(const std::string& email, const std::string& password)
  {
    User u;
    if (con("SELECT * from msc_users where email == ? and password == ?", email, blob(password)) >> u)
    {
      sess.user_id = u.id;
      sess.save();
      return true;
    }
    return false;
  };

  void logout()
  {
    sess.user_id = -1;
    sess.destroy();
  }
  
  static auto instantiate(sqlite_session<session_data>& sess,
                          sqlite_connection& con)
  {
    return authenticator{sess, con};
  }

  sqlite_session<session_data> sess;
  sqlite_connection con;
};

// ==================================================
// Current user.
struct current_user : public User
{
  static auto instantiate(session& sess, sqlite_orm<User>& orm)
  {
    if (!sess.is_authenticated())
      throw error::unauthorized("Access to this procedure is reserved to logged users.");

    current_user u;
    if (!orm.find_by_id(sess.user_id, u))
      throw error::internal_server_error("Session user_id not in user table.");

    return u;
  }
};

auto mysoundcloud_server(std::string sqlite_db_path, std::string song_root_)
{

  std::string song_root = song_root_ + std::string("/");

  auto song_path = [&] (const Song& s)
  {
    std::stringstream ss;
    ss << song_root << s.id;
    return ss.str();
  };
  
  // =========================================================
  // Build the server with its attached middlewares.
  return iod::make_api(

    _test = [] () { return D(_message = "hello world."); },
    
      // =========================================================
      // User signup, signout, login, logout.
      _login(_email, _password) = [] (auto params, authenticator& auth) {
        if (!auth.login(params.email, hash_sha3_512(params.password)))
          throw error::bad_request("Invalid login or password");
      },

      _logout = [] (session& sess) { sess.destroy(); },


      _signup(_email, _password) = [] (auto params, sqlite_orm<User>& users, sqlite_connection& con) {
        if (con("SELECT * from msc_users where email == ?", params.email).exists())
          throw error::bad_request("This user already exists");
        User u;
        u.email = params.email;
        u.password = hash_sha3_512(params.password);
        int id = 0;
        if (!(id = users.insert(u)))
          throw error::bad_request("Cannot create user account");
        return D(_id = id);
      },
 

      _signout(_password) = [] (auto params, sqlite_orm<User>& users,
                                current_user& user, session& sess, sqlite_connection& db) {
        if (user.password != hash_sha3_512(params.password)) throw error::bad_request("Invalid password");
        
        db("DELETE from msc_songs where user_id = ?", user.id).exec();
        users.destroy(user);
        sess.destroy();
      },

      // ========================================================
      // CRUD on songs.
      _song = crud<sqlite_orm_middleware<Song>>(
        _write_access = [] (current_user& user, Song& song) { return song.user_id == user.id; },
        _before_create = [] (current_user& user, Song& song) { return song.user_id = user.id; },
        _on_destroy_success = [=] (Song& song) { ::remove(song_path(song).c_str());}),

      // =========================================================
      // Upload procedure to attach a given file to the song of the given id.
      _upload =
      [&] (auto params, sqlite_orm<Song>& orm, current_user& user, mimosa::http::RequestReader* req) {

        // // Parse
        // int id = 0;
        // char id_str[9];
        // req->read(&id_str, 9);
        // for (int i = 0; i < 9; i++) id += id * 10 + id_str[i] - '0';

        Song song;
        if (!orm.find_by_id(params.id, song))
          throw error::bad_request("This song's metadata does not exist. Save them before uploading the file.");
    
        if (song.user_id != user.id)
          throw error::unauthorized("This song belongs to another user.");

        std::ofstream f(song_path(song), std::ios::binary);
        if (!f)
          throw error::internal_server_error("Cannot open file ", song_path(song), " for writting.");
        stringview file_content = req.get_tail_string();
        f.write(file_content.data(), file_content.size());
        f.close();      
      },

      // // =========================================================
      // // Access to the song of the given id.
      // _stream(_id = int()) = [&] (auto params, sqlite_orm<Song>& orm)
      // {
      //   Song song;
      //   if (!orm.find_by_id(params.id, song))
      //     throw error::not_found("song with id ", params.id, " does not exist.");

      //   if (!std::ifstream(song_path(song)))
      //     throw error::not_found("This song has not been uploaded yet.");
    
      //   return file(song_path(song));
      // }
      
    )
    // .bind_middlewares(sqlite_middleware(sqlite_db_path),
    //                   sqlite_session_middleware<session_data>("msc_sessions"),
    //                   sqlite_orm_middleware<Song>("msc_songs"),
    //                   sqlite_orm_middleware<User>("msc_users"));

}

// void test(int port)
// {
//   auto c = silicon_client(mysoundcloud_server("","").get_api(), "0.0.0.0", port);

//   std::string email = "test_test.com"; 
//   std::string email2 = "test2_test.com"; 
//   std::string password = "password";
//   std::string song_title = "song_test";
//   std::string song_filename = "song_test.mp3";

//   // Cleanup.
//   c.login(email, password);
//   c.signout(password);

//   assert(c.login(email, password).status != 200);
//   assert(c.signup(email, password).status == 200);
//   assert(c.signup(email2, password).status == 200);

//   // Signup twice with the same email should fail.
//   assert(c.signup(email, password).status != 200);

//   assert(c.login(email, password).status == 200);

//   auto r1 = c.song.create(song_title, song_filename);
//   assert(r1.status == 200);

//   int song_id = r1.response.id;
//   auto r2 = c.song.get_by_id(r1.response.id);
//   auto s = r2.response;
//   assert(r2.status == 200 and
//          s.id == song_id and
//          s.title == song_title and
//          s.filename == song_filename);

//   assert(c.signout(password).status == 200);
// }

int main(int argc, char* argv[])
{
  // using namespace iod;

  // if (argc != 4)
  // {
  //   std::cout << "Usage: " << argv[0] << " sqlite_database_path server_port songs_path" << std::endl;
  //   return 1;
  // }

  // // Instantiate the server.
  // auto server = mysoundcloud_server(argv[1], argv[3]);

  // // Serve the javascript client code at a specific route.
  // // std::string javascript_client_source_code = generate_javascript_client(server, _module = "msc");
  // // server.route("/bindings.js") = [&] (response& resp)
  // // {
  // //   resp.set_header("Content-Type", "text/javascript");
  // //   resp.write(javascript_client_source_code.data(), javascript_client_source_code.size());
  // // };

  // int port = atoi(argv[2]);
  // std::cout << "----------------------------------------------------" << std::endl;
  // std::cout << "Welcome to MySoundCloud, a tiny soundcloud-like API." << std::endl;
  // std::cout << "  Note: the server is running on port " << port << std::endl;
  // std::cout << "----------------------------------------------------" << std::endl << std::endl;

  // std::cout << "  Call one of the following procedures via the javacript bindings at /bindings.js " << std::endl;
  // std::cout << "----------------------------------------------------" << std::endl << std::endl;

  // //print_server_api(server);
  // mimosa_json_serve(api, port);
  
  // // std::thread t([&] () { server.serve(port); });

  // // test(port);

  // // t.join();
}
