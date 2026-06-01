typedef unsigned char u8x16 __attribute__((vector_size(16)));
typedef unsigned short u16x8 __attribute__((vector_size(16)));
typedef unsigned int u32x4 __attribute__((vector_size(16)));

union vec_bytes {
  u8x16 v;
  unsigned char b[16];
};

union vec_hwords {
  u16x8 v;
  unsigned short h[8];
};

union vec_words {
  u32x4 v;
  unsigned int w[4];
};

static union vec_bytes seed_b0 = {
  .b = { 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59 }
};

static union vec_bytes seed_b1 = {
  .b = { 61, 2, 67, 3, 71, 5, 73, 7, 79, 11, 83, 13, 89, 17, 97, 19 }
};

static union vec_bytes seed_bacc = {
  .b = { 101, 103, 107, 109, 113, 127, 131, 137,
         139, 149, 151, 157, 163, 167, 173, 179 }
};

static union vec_hwords seed_h0 = {
  .h = { 3, 257, 1021, 4093, 8191, 12347, 32749, 49157 }
};

static union vec_hwords seed_h1 = {
  .h = { 5, 11, 17, 23, 29, 31, 37, 41 }
};

static union vec_hwords seed_hacc = {
  .h = { 97, 193, 389, 769, 1543, 3079, 6151, 12289 }
};

static union vec_words seed_w0 = {
  .w = { 0x00010003U, 0x01020305U, 0x1234567bU, 0x80000011U }
};

static union vec_words seed_w1 = {
  .w = { 7U, 11U, 13U, 17U }
};

static union vec_words seed_wacc = {
  .w = { 19U, 23U, 29U, 31U }
};

static u8x16 simd_mul_b(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("mul %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u16x8 simd_mul_h(u16x8 a, u16x8 b)
{
  u16x8 result;
  __asm__ volatile("mul %0.8h,%1.8h,%2.8h"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u32x4 simd_mul_s(u32x4 a, u32x4 b)
{
  u32x4 result;
  __asm__ volatile("mul %0.4s,%1.4s,%2.4s"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 simd_mla_b(u8x16 acc, u8x16 a, u8x16 b)
{
  __asm__ volatile("mla %0.16b,%1.16b,%2.16b"
      : "+w"(acc) : "w"(a), "w"(b));
  return acc;
}

static u16x8 simd_mls_h(u16x8 acc, u16x8 a, u16x8 b)
{
  __asm__ volatile("mls %0.8h,%1.8h,%2.8h"
      : "+w"(acc) : "w"(a), "w"(b));
  return acc;
}

static u32x4 simd_mls_s(u32x4 acc, u32x4 a, u32x4 b)
{
  __asm__ volatile("mls %0.4s,%1.4s,%2.4s"
      : "+w"(acc) : "w"(a), "w"(b));
  return acc;
}

static int same_bytes(union vec_bytes actual, union vec_bytes expected)
{
  for (unsigned i = 0; i < 16; i++) {
    if (actual.b[i] != expected.b[i])
      return 0;
  }
  return 1;
}

static int same_hwords(union vec_hwords actual, union vec_hwords expected)
{
  for (unsigned i = 0; i < 8; i++) {
    if (actual.h[i] != expected.h[i])
      return 0;
  }
  return 1;
}

static int same_words(union vec_words actual, union vec_words expected)
{
  for (unsigned i = 0; i < 4; i++) {
    if (actual.w[i] != expected.w[i])
      return 0;
  }
  return 1;
}

static union vec_bytes expected_mul_b(union vec_bytes a, union vec_bytes b)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++)
    result.b[i] = (unsigned char) (a.b[i] * b.b[i]);
  return result;
}

static union vec_hwords expected_mul_h(union vec_hwords a,
    union vec_hwords b)
{
  union vec_hwords result = { .h = { 0 } };
  for (unsigned i = 0; i < 8; i++)
    result.h[i] = (unsigned short) (a.h[i] * b.h[i]);
  return result;
}

static union vec_words expected_mul_s(union vec_words a, union vec_words b)
{
  union vec_words result = { .w = { 0 } };
  for (unsigned i = 0; i < 4; i++)
    result.w[i] = a.w[i] * b.w[i];
  return result;
}

static union vec_bytes expected_mla_b(union vec_bytes acc,
    union vec_bytes a, union vec_bytes b)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++)
    result.b[i] = (unsigned char) (acc.b[i] + (a.b[i] * b.b[i]));
  return result;
}

static union vec_hwords expected_mls_h(union vec_hwords acc,
    union vec_hwords a, union vec_hwords b)
{
  union vec_hwords result = { .h = { 0 } };
  for (unsigned i = 0; i < 8; i++)
    result.h[i] = (unsigned short) (acc.h[i] - (a.h[i] * b.h[i]));
  return result;
}

static union vec_words expected_mls_s(union vec_words acc,
    union vec_words a, union vec_words b)
{
  union vec_words result = { .w = { 0 } };
  for (unsigned i = 0; i < 4; i++)
    result.w[i] = acc.w[i] - (a.w[i] * b.w[i]);
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  union vec_bytes b0 = seed_b0;
  union vec_bytes b1 = seed_b1;
  union vec_bytes bacc = seed_bacc;
  union vec_hwords h0 = seed_h0;
  union vec_hwords h1 = seed_h1;
  union vec_hwords hacc = seed_hacc;
  union vec_words w0 = seed_w0;
  union vec_words w1 = seed_w1;
  union vec_words wacc = seed_wacc;
  unsigned long failures = 0;

  b0.b[0] ^= (unsigned char) a0;
  b1.b[9] ^= (unsigned char) a1;
  h0.h[3] ^= (unsigned short) a2;
  w0.w[2] ^= (unsigned int) a3;

  union vec_bytes actual_b;
  actual_b.v = simd_mul_b(b0.v, b1.v);
  if (!same_bytes(actual_b, expected_mul_b(b0, b1)))
    failures |= 1UL << 0;
  actual_b.v = simd_mla_b(bacc.v, b0.v, b1.v);
  if (!same_bytes(actual_b, expected_mla_b(bacc, b0, b1)))
    failures |= 1UL << 1;

  union vec_hwords actual_h;
  actual_h.v = simd_mul_h(h0.v, h1.v);
  if (!same_hwords(actual_h, expected_mul_h(h0, h1)))
    failures |= 1UL << 2;
  actual_h.v = simd_mls_h(hacc.v, h0.v, h1.v);
  if (!same_hwords(actual_h, expected_mls_h(hacc, h0, h1)))
    failures |= 1UL << 3;

  union vec_words actual_w;
  actual_w.v = simd_mul_s(w0.v, w1.v);
  if (!same_words(actual_w, expected_mul_s(w0, w1)))
    failures |= 1UL << 4;
  actual_w.v = simd_mls_s(wacc.v, w0.v, w1.v);
  if (!same_words(actual_w, expected_mls_s(wacc, w0, w1)))
    failures |= 1UL << 5;

  return failures == 0 ? 42 : (0xbad50000UL | failures);
}
