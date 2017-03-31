#include <set>
#include <map>
#include <string>
#include <vector>

#include <microhttpd.h>
#include <silicon/error.hh>
#include <iod/json.hh>
#include <iod/stringview.hh>

namespace sl
{
  using namespace iod;
  using error::format_error;

  // Decode a plain value.
  template <typename O>
  iod::stringview urldecode2(std::set<void*>& found, iod::stringview str, O& obj)
  {
    if (str.size() == 0)
      throw std::runtime_error(format_error("Urldecode error: expected key end", str[0]));

    if (str[0] != '=')
      throw std::runtime_error(format_error("Urldecode error: expected =, got ", str[0]));

    int start = 1;
    int end = 1;
    
    while (str.size() != end and str[end] != '&') end++;

    if (start != end)
    {
      std::string content = str.substr(start, end - start).to_std_string();
      // Url escape.
      content.resize(MHD_http_unescape(&content[0]));
      obj = boost::lexical_cast<O>(content);
      found.insert(&obj);
    }
    if (end == str.size())
      return str.substr(end, 0);
    else
      return str.substr(end + 1, str.size() - end - 1);
  }

  template <typename... O>
  iod::stringview urldecode2(std::set<void*>& found, iod::stringview str, sio<O...>& obj, bool root = false);
  
  // Decode an array element.
  template <typename O>
  iod::stringview urldecode2(std::set<void*>& found, iod::stringview str, std::vector<O>& obj)
  {
    if (str.size() == 0)
      throw std::runtime_error(format_error("Urldecode error: expected key end", str[0]));

    if (str[0] != '[')
      throw std::runtime_error(format_error("Urldecode error: expected [, got ", str[0]));

    // Get the index substring.
    int index_start = 1;
    int index_end = 1;
    
    while (str.size() != index_end and str[index_end] != ']') index_end++;

    if (str.size() == 0)
      throw std::runtime_error(format_error("Urldecode error: expected key end", str[0]));
    
    auto next_str = str.substr(index_end + 1, str.size() - index_end - 1);

    if (index_end == index_start) // [] syntax, push back a value.
    {
      O x;
      auto ret = urldecode2(found, next_str, x);
      obj.push_back(x);
      return ret;
    }
    else // [idx] set index idx.
    {
      int idx = std::strtol(str.data() + index_start, nullptr, 10);
      if (idx >= 0 and idx <= 9999)
      {
        if (obj.size() <= idx)
          obj.resize(idx + 1);
        return urldecode2(found, next_str, obj[idx]);
      }
      else
        throw std::runtime_error(format_error("Urldecode error: out of bound array subscript."));
    }
    
  }

  // Decode an object member.
  template <typename... O>
  iod::stringview urldecode2(std::set<void*>& found, iod::stringview str, sio<O...>& obj, bool root)
  {
    if (str.size() == 0)
      throw error::bad_request("Urldecode error: expected key end", str[0]);
      
    int key_start = 0;
    int key_end = key_start + 1;

    int next = 0;

    if (not root)
    {
      if (str[0] != '[')
        throw std::runtime_error(format_error("Urldecode error: expected [, got ", str[0]));
        
      key_start = 1;
      while (key_end != str.size() and str[key_end] != ']' and str[key_end] != '=') key_end++;

      if (key_end == str.size())
        throw std::runtime_error("Urldecode error: unexpected end.");
      next = key_end + 1; // skip the ]
    }
    else
    {
      while (key_end != str.size() and str[key_end] != '[' and str[key_end] != '=') key_end++;
      next = key_end;
    }

    auto key = str.substr(key_start, key_end - key_start).to_std_string();
    auto next_str = str.substr(next, str.size() - next);

    iod::stringview ret;
    foreach(obj) | [&] (auto& m)
    {
      if (m.symbol().name() == key)
      {
        ret = urldecode2(found, next_str, m.value());
      }
    };
    return ret;
  }

  template <typename O>
  std::string urldecode_check_missing_fields(const std::set<void*>& found, O& obj)
  {
    if (found.find(&obj) == found.end())
      return " ";
    else
      return std::string();
  }

  template <typename O>
  std::string urldecode_check_missing_fields(const std::set<void*>& found, std::vector<O>& obj)
  {
    // Fixme : implement missing fields checking in std::vector<sio<O...>>
    // for (int i = 0; i < obj.size(); i++)
    // {
    //   std::string missing = urldecode_check_missing_fields(found, obj[i]);
    //   if (missing.size())
    //   {
    //     std::stringstream ss;
    //     ss << '[' << i << "]" << missing;
    //     return ss.str();
    //   }
    // }
    return std::string();
  }
  
  template <typename... O>
  std::string urldecode_check_missing_fields(const std::set<void*>& found, sio<O...>& obj, bool root = false)
  {
    std::string missing;
    foreach(obj) | [&] (auto& m)
    {
      if(!m.attributes().has(_optional) and missing.size() == 0)
      {
        missing = urldecode_check_missing_fields(found, m.value());
        if (missing.size())
          missing = (root ? "" : ".") + std::string(m.symbol().name()) + missing;
      }
    };
    return missing;
  }

  template <typename S, typename... O>
  std::string urldecode_check_missing_fields_on_subset(const std::set<void*>& found, sio<O...>& obj, bool root = false)
  {
    std::string missing;
    foreach(S()) | [&] (auto m_)
    {
      auto& m = obj[m_.symbol()];
      if(!m_.attributes().has(_optional) and missing.size() == 0)
      {
        missing = urldecode_check_missing_fields(found, m);
        if (missing.size())
          missing = (root ? "" : ".") + std::string(m_.symbol().name()) + missing;
      }
    };
    return missing;
  }
  
  // Frontend.
  template <typename O>
  void urldecode(iod::stringview str, O& obj)
  {
    std::set<void*> found;

    // Parse the urlencoded string
    while (str.size() > 0)
      str = urldecode2(found, str, obj, true);

    // Check for missing fields.
    std::string missing = urldecode_check_missing_fields(found, obj, true);
    if (missing.size())
      throw std::runtime_error(format_error("Missing argument ", missing));
  }

}
