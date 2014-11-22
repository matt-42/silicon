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

  auto song_path = [&] (const Song& s)
  {
    std::ostringstream ss;
    ss << argv[2] << s.id;
    return ss.str();
  };

  auto mime_type = [&] (const std::string& s)
  {
    return "audio/mp3";
  };
  
  silicon.middlewares(sqlite_middleware(argv[1]),
                      sqlite_session_storage("user_sessions"),
                      sqlite_orm_middleware<Song>("songs"),
                      sqlite_orm_middleware<User>("users"))
    .api(
      

      // =========================================================
      // User signup, signout, login, logout.
      
      _Login(_Email, _Password) = [] (auto params, authenticator& auth)
      {
        if (!auth.login(params.email, hash_sha3_512(params.password)))
          throw error::bad_request("Invalid login or password");
      },
      
      _Logout = [] (session& sess) { sess.destroy(); },
      
      _Signup(_Email, _Password) = [] (auto params, sqlite_orm<User>& users)
      {
        params.password = hash_sha3_512(params.password);
        if (!users.insert(params))
          throw error::bad_request("Cannot create user account");
      },
      
      _Signout(_Password) = [] (auto params, sqlite_orm<User>& users,
                                current_user& user, session& sess)
      {
        if (user.password != hash_sha3_512(params.password)) throw error::bad_request("Invalid password");
        users.destroy(user);
        sess.destroy();
      },

      // =========================================================
      // Setup CRUD procedures of objects Song.
      _Song = crud<sqlite_orm_middleware<Song>>(
        _Write_access = [] (current_user& user, Song& song) { return song.user_id == user.id; }
        ),
      
      // =========================================================
      // Upload procedure to attach a given file to the song of the given id.
      _Upload(_Id = int()) =

      [&] (auto params, sqlite_orm<Song>& orm, current_user& user, request& req) {

        Song song;
        if (!orm.find_by_id(params.id, song))
          throw error::bad_request("This song's metadata does not exist. Save them before uploading the file.");
    
        if (song.user_id != user.id)
          throw error::unauthorized("This song belongs to another user.");

        std::ofstream f(song_path(song), std::ios::binary);
        stringview file_content = req.get_tail_string();
        f.write(file_content.data(), file_content.size());
        f.close();      
      },

      // =========================================================
      // Access to the song of the given id.
      _Stream(_Id = int()) =

      [] (auto params, sqlite_orm& orm, response& resp)
      {
        Song song;
        if (!orm.find_by_id(params.id, song))
          throw error::not_found("This song does not exist.");

        resp.set_header("Content-Type", mime_type(song.filename));

        std::ifstream f(song_path(song), std::ios::binary);
        char buf[1024];
        int s;
        while (s = f.read(buf, sizeof(buf)))
          resp.write(buf, s);
        f.close();      
      }
            
      )

    .serve(atoi(argv[2]));
}
