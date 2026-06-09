// ============================================================
// ARJUNA — crypto.h  v3.0
// ============================================================
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "config.h"

namespace Crypto {
    void encrypt(const uint8_t* key,
                 const uint8_t* plain,  size_t len,
                 uint8_t*       cipher,
                 uint8_t*       nonce_out);

    void decrypt(const uint8_t* key,
                 const uint8_t* cipher, size_t len,
                 uint8_t*       plain,
                 const uint8_t* nonce_in);

    void mac4(const uint8_t* key,  size_t klen,
              const uint8_t* data, size_t dlen,
              uint8_t tag[4]);

    void toHex(const uint8_t* buf, size_t n,
               char* out, size_t out_sz);
}
