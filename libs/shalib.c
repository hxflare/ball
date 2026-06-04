#include "../bsys.h"
#include "../btools.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};
static const uint32_t H_INIT[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372,
                                   0xa54ff53a, 0x510e527f, 0x9b05688c,
                                   0x1f83d9ab, 0x5be0cd19};
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))
void sha256_process_chunk(const uint8_t chunk[64], uint32_t state[8]) {
  uint32_t W[64];
  uint32_t a, b, c, d, e, f, g, h;
  for (int i = 0; i < 16; i++) {
    W[i] = ((uint32_t)chunk[i * 4] << 24) | ((uint32_t)chunk[i * 4 + 1] << 16) |
           ((uint32_t)chunk[i * 4 + 2] << 8) | ((uint32_t)chunk[i * 4 + 3]);
  }
  for (int i = 16; i < 64; i++) {
    W[i] = SIG1(W[i - 2]) + W[i - 7] + SIG0(W[i - 15]) + W[i - 16];
  }
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  f = state[5];
  g = state[6];
  h = state[7];
  for (int i = 0; i < 64; i++) {
    uint32_t t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
    uint32_t t2 = EP0(a) + MAJ(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  state[5] += f;
  state[6] += g;
  state[7] += h;
}
void sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
  uint32_t state[8];
  uint8_t chunk[64];
  for (int i = 0; i < 8; i++)
    state[i] = H_INIT[i];

  size_t full_chunks = len / 64;
  for (size_t i = 0; i < full_chunks; i++)
    sha256_process_chunk(data + i * 64, state);
  size_t remaining = len % 64;
  memcpy(chunk, data + full_chunks * 64, remaining);
  chunk[remaining++] = 0x80;
  if (remaining > 56) {
    memset(chunk + remaining, 0, 64 - remaining);
    sha256_process_chunk(chunk, state);
    memset(chunk, 0, 56);
  } else {
    memset(chunk + remaining, 0, 56 - remaining);
  }
  uint64_t bit_len = (uint64_t)len * 8;
  chunk[56] = (uint8_t)(bit_len >> 56);
  chunk[57] = (uint8_t)(bit_len >> 48);
  chunk[58] = (uint8_t)(bit_len >> 40);
  chunk[59] = (uint8_t)(bit_len >> 32);
  chunk[60] = (uint8_t)(bit_len >> 24);
  chunk[61] = (uint8_t)(bit_len >> 16);
  chunk[62] = (uint8_t)(bit_len >> 8);
  chunk[63] = (uint8_t)(bit_len);
  sha256_process_chunk(chunk, state);
  for (int i = 0; i < 8; i++) {
    out[i * 4] = (uint8_t)(state[i] >> 24);
    out[i * 4 + 1] = (uint8_t)(state[i] >> 16);
    out[i * 4 + 2] = (uint8_t)(state[i] >> 8);
    out[i * 4 + 3] = (uint8_t)(state[i]);
  }
}
cstring sha256_cstr(cstring *input) {
  char hex[] = "0123456789abcdef";
  uint8_t hash[32];
  cstring out = CSTRING_INIT;
  sha256((const uint8_t *)input->str, (size_t)input->len, hash);
  for (int i = 0; i < 32; i++) {
    cchstr_append(&out, hex[hash[i] >> 4]);
    cchstr_append(&out, hex[hash[i] & 0xf]);
  }
  return out;
}
char *sha256_str(const char *input) {
    static const char hex[] = "0123456789abcdef";
    uint8_t hash[32];
    sha256((const uint8_t *)input, strlen(input), hash);
    char *out = malloc(65);
    if (!out) return NULL;
    for (int i = 0; i < 32; i++) {
        out[i * 2]     = hex[hash[i] >> 4];
        out[i * 2 + 1] = hex[hash[i] & 0xf];
    }
    out[64] = '\0';
    return out;
}

cstring sha256_str_cstr(const char *input) {
  cstring tmp = {(char *)input, (int)strlen(input)};
  return sha256_cstr(&tmp);
}
