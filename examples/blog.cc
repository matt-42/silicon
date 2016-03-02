#include <silicon/backends/mhd.hh>
#include <silicon/api_description.hh>

#include "blog_api.hh"

using namespace sl;
using namespace s;

int main()
{
  std::cout << api_description(blog_api) << std::endl;
  mhd_json_serve(blog_api, blog_middlewares, 9999);
}
