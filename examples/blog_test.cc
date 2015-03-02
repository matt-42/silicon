#include <thread>
#include <silicon/clients/client.hh>
#include <silicon/backends/mhd.hh>

#include "blog_api.hh"

int main(int argc, char* argv[])
{
  
  std::thread t([&] () { mhd_json_serve(blog_api, 12345); });
  // Wait for the server to start.
  usleep(.01e6);

  auto db = blog_api.instantiate_factory<sqlite_connection>();
  db("delete from blog_users where login = ?")("john");
  
  auto c = json_client(blog_api, "127.0.0.1", 12345);
  
  // User creation should succeed
  auto r1 = c.user.create("john", "password");
  std::cout << json_encode(r1) << std::endl;
  assert(r1.status == 200);

  int user_id = r1.response.id;
  //int user_id = 1;
  // Duplicate user creation should fail.
  auto r2 = c.user.create("john", "xxx");
  assert(r2.status == 400);

  // Login with a bad password should fail.
  auto r3 = c.login("johnx", "xxx");
  assert(r3.status == 400);

  // Login with a bad username should fail.
  auto r4 = c.login("johnn", "password");
  assert(r4.status == 400);
  
  // Valid login should succeed.
  auto r5 = c.login("john", "password");
  std::cout << json_encode(r5) << std::endl;
  assert(r5.status == 200);

  // Create a post.
  auto title1 = "My Title";
  auto content1 = "My content.";
  auto r6 = c.post.create("My Title", "My content.");
  std::cout << json_encode(r6) << std::endl;
  assert(r6.status == 200);

  // The post should be persisted.
  auto r7 = c.post.get_by_id(r6.response.id);
  std::cout << json_encode(r7) << std::endl;
  assert(r7.status == 200);
  auto post = r7.response;

  assert(post.title == title1);
  assert(post.body == content1);
  
  // The post creation should set the right user_id.
  assert(post.user_id == user_id);

  // Post update should work.
  // auto r8 = c.post.update(post.id, )
  
  // Logout
  auto rx = c.logout();
  assert(rx.status == 200);

  exit(0);
}
