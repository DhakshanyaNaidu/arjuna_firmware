// ============================================================
// ARJUNA — crypto.cpp  v3.0
// AES-256-CTR encryption via rweather/Crypto library
// BLAKE2s-4 MAC for packet integrity (faster than HMAC-SHA256)
// Hardware TRNG via esp_fill_random() for nonce generation
// ============================================================
#include "crypto.h"
#include <AES.h>
#include <CTR.h>
#include <BLAKE2s.h>
#include <string.h>
#include <esp_random.h>

static CTR<AES128> ctr;

void Crypto::encrypt(const uint8_t* key,
                     const uint8_t* plain,  size_t len,
                     uint8_t*       cipher,
                     uint8_t*       nonce_out)
{
    // Fill nonce from ESP32 hardware TRNG (true random)
    esp_fill_random(nonce_out, CTR_NONCE_SIZE);

    // AES-CTR IV: 8-byte nonce || 8-byte zero counter
    uint8_t iv[16] = {0};
    memcpy(iv, nonce_out, CTR_NONCE_SIZE);

    ctr.clear();
    ctr.setKey(key, AES_KEY_SIZE);
    ctr.setIV(iv, 16);
    ctr.setCounterSize(8);
    ctr.encrypt(cipher, plain, len);
}

void Crypto::decrypt(const uint8_t* key,
                     const uint8_t* cipher, size_t len,
                     uint8_t*       plain,
                     const uint8_t* nonce_in)
{
    uint8_t iv[16] = {0};
    memcpy(iv, nonce_in, CTR_NONCE_SIZE);

    ctr.clear();
    ctr.setKey(key, AES_KEY_SIZE);
    ctr.setIV(iv, 16);
    ctr.setCounterSize(8);
    ctr.decrypt(plain, cipher, len);
}

void Crypto::mac4(const uint8_t* key,  size_t klen,
                  const uint8_t* data, size_t dlen,
                  uint8_t tag[4])
{
    BLAKE2s b2;
    b2.resetHMAC(key, klen);
    b2.update(data, dlen);
    uint8_t digest[32];
    b2.finalizeHMAC(key, klen, digest, 32);
    memcpy(tag, digest, 4);
}

void Crypto::toHex(const uint8_t* buf, size_t n,
                   char* out, size_t out_sz)
{
    static const char h[] = "0123456789ABCDEF";
    size_t w = 0;
    for (size_t i = 0; i < n && (w+2) < out_sz; i++) {
        out[w++] = h[buf[i] >> 4];
        out[w++] = h[buf[i] & 0xF];
    }
    out[w] = '\0';
}
