#include <silicon/backends/mhd.hh>
#include <silicon/api.hh>

iod_define_symbol(test);

using namespace sl;
auto test_api = http_api(
  _test = [] (mhd_request* req)
  {
    const char* t = req->get_header("User-Agent");
    if (t)
      return t;
    else
      return "Header not found";
  });

int main()
{
  auto ctx = mhd_json_serve(test_api, 9999);
}
