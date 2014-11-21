#include <iostream>
#include <iod/sio_utils.hh>
#include <silicon/mimosa_backend.hh>
#include <silicon/sqlite.hh>
#include <silicon/sqlite_orm.hh>
#include <silicon/sqlite_session.hh>
#include <silicon/server.hh>
#include <silicon/crud.hh>

#include "symbols.hh"

using namespace s;
using namespace iod;

// Model
typedef decltype(iod::D(_Id(_Primary_key) = int(),
                        _Email = std::string(),
                        _Password = std::string()
                   )) User;

typedef decltype(iod::D(_Id(_Primary_key) = int(),
                        _User_id = int(),
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
    if (con("SELECT * from user where email == ? and password == ?", email, password) >> u)
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

  sqlite_session<session_data>& sess;
  sqlite_connection& con;
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

  if (!argc)
  {
    std::cout << "Usage: " << argv[0] << " sqlite_database_path server_port" << std::endl;
    return 1;
  }

  // silicon.middlewares(sqlite_middleware(argv[1]),
  //                     sqlite_session_storage("user_sessions"),
  //                     sqlite_orm_middleware<Song>("songs"),
  //                     sqlite_orm_middleware<User>("users"))
  //   .api(

  //     _Song = crud<sqlite_orm_middleware<Song>>(),
      
  //     _User = crud<sqlite_orm_middleware<User>>(),
      
  //     _Upload(_Id = int(), _File = binary_blob()) = [] (auto params, sqlite_orm& orm, current_user& user)
  //     {
  //       Song song;
  //       if (!orm.find_by_id(params.id, song))
  //         throw error::bad_request("This song's metadata does not exist. Save them before uploading the file.");
    
  //       if (song.user_id != user.id)
  //         throw error::unauthorized("This song belongs to another user.");

  //       std::ofstream f(song_path(song), std::ios::binary);
  //       f.write(params.file.data, params.file.len);
  //       f.close();      
  //     }

  //     _Stream(_Id = int()) = [] (auto params, sqlite_orm& orm, response& resp)
  //     {
  //       Song song;
  //       if (!orm.find_by_id(params.id, song))
  //         throw error::not_found("This song does not exist.");

  //       resp.set_header("Content-Type", mime_type(song.filename));

  //       std::ifstream f(song_path(song), std::ios::binary);
  //       char buf[1024];
  //       int s;
  //       while (s = f.read(buf, sizeof(buf)))
  //         resp.write(buf, s);
  //       f.close();      
  //     }
            
  //     )

  //   .serve(atoi(argv[2]));
  

  // Was:

  auto song_path = [&] (const Song& s)
  {
    std::ostringstream ss;
    ss << argv[2] << s.id << "_" << s.title;
    return ss.str();
  };

  auto mime_type = [&] (const std::string& s)
  {
    return "audio/mp3";
  };

  // Build the server with its stateful middlewares.
  auto server = silicon(sqlite_middleware(argv[1]),
                        sqlite_session_middleware<session_data>("user_sessions"),
                        sqlite_orm_middleware<Song>("songs"),
                        sqlite_orm_middleware<User>("users"));
  
  // Setup CRUD procedures of objects Song and User.
  setup_crud(server,
             get_middleware<sqlite_orm_middleware<Song>>(server),
             _Prefix = "song",
             _Validate = [] (current_user& user, Song& song) { return song.user_id == user.id; });

  setup_crud(server,
             get_middleware<sqlite_orm_middleware<User>>(server),
             _Prefix = "user",
             _Validate = [] (current_user& cuser, User& user) { return cuser.id == user.id; });

  // Upload procedure to attach a given file to the song of the given id.
  server["upload"](_Id = int()) =
    [&] (auto params, sqlite_orm<Song>& orm, current_user& user, request& req) {

    Song song;
    if (!orm.find_by_id(params.id, song))
      throw error::bad_request("This song's metadata does not exist. Save them before uploading the file.");
    
    if (song.user_id != user.id)
      throw error::unauthorized("This song belongs to another user.");

    std::ofstream f(song_path(song), std::ios::binary);
    f.write(params.file.data(), params.file.size());
    f.close();      
  };

  // Access to the song of the given id.
  server["stream"](_Id = int()) = [&] (auto params,
                                       sqlite_orm<Song>& orm,
                                       response& resp)
  {
    Song song;
    if (!orm.find_by_id(params.id, song))
      throw error::not_found("This song does not exist.");

    resp.set_header("Content-Type", mime_type(song.filename));

    std::ifstream f(song_path(song), std::ios::binary);
    char buf[1024];
    do
    {
      f.read(buf, sizeof(buf));
      int s = f.gcount();
      resp.write(buf, s);
    } while (!f.eof());

    f.close();      
  };

  server.serve();
}
