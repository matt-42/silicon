#include <silicon/mimosa_backend.hh>
#include <silicon/server.hh>

int main(int argc, char* argv[])
{
  using namespace iod;

  auto server = silicon();

  char pwd[1024];
  std::string root = getcwd(pwd, sizeof(pwd));

  server.route("/file") = [&] () { return file(root + "/test.txt"); };

  server.serve(8888);
}
