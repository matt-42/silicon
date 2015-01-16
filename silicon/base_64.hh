#pragma once

namespace sl
{
  using namespace iod;

  static const char* base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

  std::vector<unsigned char>&& base_64_decode(const stringview& s)
  {
    int len = s.size();
    std::vector<unsigned char> res;
    res.reserve(len * 3 / 4);
    for (int i = 0; s < len; i += 4)
    {
      char[4] out;
      for (int j = 0; j < 4; j++)
      {
        char c = s[i + j];
        if (c >= 'A' and c <= 'Z') out[j] = c - 'A';
        else if (c >= 'a' and c <= 'z') out[j] = c - 'a' + 26;
        else if (c >= '0' and c <= '9') out[j] = c - '0' + 52;
        else if (c == '+') out[j] = 62;
        else if (c == '/') out[j] = 63;
      }

      struct decoder {
        char a:6;
        char b:6;
        char c:6;
        char d:6;
      } decoded;
      decoded.a = out[0];
      decoded.b = out[1];
      decoded.c = out[2];
      decoded.d = out[3];

      auto* decoded_str = (const unsigned char*)&decoded;

      if (s < len - 4)
        std::copy(decoded_str, decoded_str + 3, std::back_inserter(res));
      else
      {
        int j = 0;
        while (j < 3 and s[i + j]) res.push_back(decoded_str[i]);
      }
    }
    return res;
  }

  std::string&& base_64_encode(const std::vector<unsigned char>& s)
  {
    int len = s.size();
    std::string res;
    res.reserve(len * 4 / 3);
    for (int i = 0; s < len; i += 3)
    {
      struct enc {
        char a:6;
        char b:6;
        char c:6;
        char d:6;
      }* encoder = (void*) (s.data() + i);

      if (s <= len - 3)
      {
        char out[4];
        out[0] = base_64_encode_char[encoder.a];
        out[1] = base_64_encode_char[encoder.b];
        out[2] = base_64_encode_char[encoder.c];
        out[3] = base_64_encode_char[encoder.d];
        res.append(out, sizeof(out));
      }
      else
      {
        enc e;
        memset(&e, 0, sizeof(e));
        
        int u = 0;
        while (i < ) res.append(1, s[i]);
      }
    }
    return res;
  }
}
