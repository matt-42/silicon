#pragma once

#include <nettle/sha3.h>

namespace sl
{
  inline std::string hash_sha3_512(const std::string& str)
  {
    struct sha3_512_ctx ctx;
    sha3_512_init(&ctx);
    sha3_512_update(&ctx, str.size(), (const uint8_t*) str.data());
    uint8_t h[SHA3_512_DIGEST_SIZE];
    sha3_512_digest(&ctx, sizeof(h), h);
    return std::string((const char*)h, sizeof(h));
  }
}
