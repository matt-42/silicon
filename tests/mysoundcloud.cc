#include <cstdio>
#include <iostream>
#include <iod/sio_utils.hh>
#include <silicon/mimosa_backend.hh>
#include <silicon/sqlite.hh>
#include <silicon/sqlite_orm.hh>
#include <silicon/sqlite_session.hh>
#include <silicon/server.hh>
#include <silicon/crud.hh>
#include <silicon/hash.hh>
#include <silicon/javascript_client.hh>

#include "symbols.hh"

using namespace s;
using namespace iod;

// Models
typedef decltype(iod::D(_Id(_Primary_key) = int(),
                        _Email = std::string(),
                        _Password = blob()
                   )) User;

typedef decltype(iod::D(_Id(_Primary_key) = int(),
                        _User_id(_Computed) = int(),
                        _Title = std::string(),
                        _Filename = std::string()
                   )) Song;


// ==================================================
// Session
struct session_data
{
  session_data() { user_id = -1; }
  bool is_authenticated() const { return user_id != -1; }

  auto sio_info() { return D(_User_id = int()); }
  
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

int main(int argc, char* argv[])
{
  using namespace iod;

  if (argc != 4)
  {
    std::cout << "Usage: " << argv[0] << " sqlite_database_path server_port songs_path" << std::endl;
    return 1;
  }


  std::string song_root = argv[3] + std::string("/");

  auto song_path = [&] (const Song& s)
  {
    std::stringstream ss;
    ss << song_root << s.id;
    return ss.str();
  };

  // =========================================================
  // Build the server with its attached middlewares.
  auto server = silicon(sqlite_middleware(argv[1]),
                        sqlite_session_middleware<session_data>("msc_sessions"),
                        sqlite_orm_middleware<Song>("msc_songs"),
                        sqlite_orm_middleware<User>("msc_users"));

  // =========================================================
  // User signup, signout, login, logout.
  server["login"](_Email, _Password) = [] (auto params, authenticator& auth) {
    if (!auth.login(params.email, hash_sha3_512(params.password)))
      throw error::bad_request("Invalid login or password");
  };

  server["logout"] = [] (session& sess) { sess.destroy(); };

  server["signup"](_Email, _Password) = [] (auto params, sqlite_orm<User>& users, sqlite_connection& con) {
    if (con("SELECT * from msc_users where email == ?", params.email).exists())
      throw error::bad_request("This user already exists");
    User u;
    u.email = params.email;
    u.password = hash_sha3_512(params.password);
    int id = 0;
    if (!(id = users.insert(u)))
      throw error::bad_request("Cannot create user account");
    return D(_Id = id);
  };

  server["signout"](_Password) = [] (auto params, sqlite_orm<User>& users,
                                     current_user& user, session& sess, sqlite_connection& db) {
    if (user.password != hash_sha3_512(params.password)) throw error::bad_request("Invalid password");

    db("DELETE from msc_songs where user_id = ?", user.id).exec();
    users.destroy(user);
    sess.destroy();
  };

  // =========================================================
  // Setup CRUD procedures of object Song.
  setup_crud(server,
             get_middleware<sqlite_orm_middleware<Song>>(server),
             _Prefix = "song",
             _Write_access = [] (current_user& user, Song& song) { return song.user_id == user.id; },
             _Before_create = [] (current_user& user, Song& song) { return song.user_id = user.id; },
             _On_destroy_success = [=] (Song& song) { ::remove(song_path(song).c_str());}
    );
  
  // =========================================================
  // Upload procedure to attach a given file to the song of the given id.
  server["upload"](_Id = int()) =
    [&] (auto params, sqlite_orm<Song>& orm, current_user& user, request& req) {

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
  };

  // =========================================================
  // Access to the song of the given id.
  server["stream"](_Id = int()) = [&] (auto params, sqlite_orm<Song>& orm)
  {
    Song song;
    if (!orm.find_by_id(params.id, song))
      throw error::not_found("song with id ", params.id, " does not exist.");

    if (!std::ifstream(song_path(song)))
      throw error::not_found("This song has not been uploaded yet.");
    
    return file(song_path(song));
  };

  std::string javascript_client_source_code = generate_javascript_client(server, _Module = "msc");
  server.route("/bindings.js") = [&] (response& resp)
  {
    resp.set_header("Content-Type", "text/javascript");
    resp.write(javascript_client_source_code.data(), javascript_client_source_code.size());
  };

  server.route("/") = [] () { return file("../mysoundcloud_test.html"); };
  server.route("/test") = [] () { return file("../test.js"); };

  int port = atoi(argv[2]);
  std::cout << "----------------------------------------------------" << std::endl;
  std::cout << "Welcome to MySoundCloud, a tiny soundcloud-like API." << std::endl;
  std::cout << "  Note: the server is running on port " << port << std::endl;
  std::cout << "----------------------------------------------------" << std::endl << std::endl;

  std::cout << "  Call one of the following procedures via the javacript bindings at /bindings.js " << std::endl;
  std::cout << "----------------------------------------------------" << std::endl << std::endl;

  print_server_api(server);
  
  server.serve(atoi(argv[2]));
}
