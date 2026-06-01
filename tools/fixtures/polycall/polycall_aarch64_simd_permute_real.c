typedef unsigned char u8x16 __attribute__((vector_size(16)));

union vec_bytes {
  u8x16 v;
  unsigned char b[16];
};

static union vec_bytes seed_a = {
  .b = { 0x01, 0x13, 0x25, 0x37, 0x49, 0x5b, 0x6d, 0x7f,
         0x81, 0x93, 0xa5, 0xb7, 0xc9, 0xdb, 0xed, 0xff }
};

static union vec_bytes seed_b = {
  .b = { 0xf0, 0xe2, 0xd4, 0xc6, 0xb8, 0xaa, 0x9c, 0x8e,
         0x70, 0x62, 0x54, 0x46, 0x38, 0x2a, 0x1c, 0x0e }
};

static union vec_bytes seed_index = {
  .b = { 15, 0, 1, 2, 16, 3, 4, 31, 5, 6, 7, 8, 9, 10, 11, 12 }
};

static u8x16 simd_ext(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("ext %0.16b,%1.16b,%2.16b,#5"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 simd_tbl(u8x16 table, u8x16 indexes)
{
  u8x16 result;
  __asm__ volatile("tbl %0.16b,{%1.16b},%2.16b"
      : "=w"(result) : "w"(table), "w"(indexes));
  return result;
}

static u8x16 simd_zip1(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("zip1 %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 simd_zip2(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("zip2 %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 simd_trn1(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("trn1 %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 simd_trn2(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("trn2 %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 simd_uzp1(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("uzp1 %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 simd_uzp2(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("uzp2 %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static int same_vec(union vec_bytes actual, union vec_bytes expected)
{
  for (unsigned i = 0; i < 16; i++) {
    if (actual.b[i] != expected.b[i])
      return 0;
  }
  return 1;
}

static union vec_bytes expected_ext(union vec_bytes a, union vec_bytes b)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++) {
    unsigned source = i + 5;
    result.b[i] = source < 16 ? a.b[source] : b.b[source - 16];
  }
  return result;
}

static union vec_bytes expected_tbl(union vec_bytes table,
    union vec_bytes indexes)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++) {
    unsigned index = indexes.b[i];
    result.b[i] = index < 16 ? table.b[index] : 0;
  }
  return result;
}

static union vec_bytes expected_zip(union vec_bytes a, union vec_bytes b,
    unsigned high)
{
  union vec_bytes result = { .b = { 0 } };
  unsigned base = high ? 8 : 0;
  for (unsigned i = 0; i < 8; i++) {
    result.b[i * 2] = a.b[base + i];
    result.b[i * 2 + 1] = b.b[base + i];
  }
  return result;
}

static union vec_bytes expected_trn(union vec_bytes a, union vec_bytes b,
    unsigned odd)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 8; i++) {
    result.b[i * 2] = a.b[i * 2 + odd];
    result.b[i * 2 + 1] = b.b[i * 2 + odd];
  }
  return result;
}

static union vec_bytes expected_uzp(union vec_bytes a, union vec_bytes b,
    unsigned odd)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 8; i++) {
    result.b[i] = a.b[i * 2 + odd];
    result.b[i + 8] = b.b[i * 2 + odd];
  }
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  union vec_bytes a = seed_a;
  union vec_bytes b = seed_b;
  union vec_bytes indexes = seed_index;
  unsigned long failures = 0;

  a.b[0] ^= (unsigned char) a0;
  a.b[7] ^= (unsigned char) a1;
  b.b[3] ^= (unsigned char) a2;
  indexes.b[14] ^= (unsigned char) (a3 & 3);

  union vec_bytes actual;
  actual.v = simd_ext(a.v, b.v);
  if (!same_vec(actual, expected_ext(a, b)))
    failures |= 1UL << 0;
  actual.v = simd_tbl(a.v, indexes.v);
  if (!same_vec(actual, expected_tbl(a, indexes)))
    failures |= 1UL << 1;
  actual.v = simd_zip1(a.v, b.v);
  if (!same_vec(actual, expected_zip(a, b, 0)))
    failures |= 1UL << 2;
  actual.v = simd_zip2(a.v, b.v);
  if (!same_vec(actual, expected_zip(a, b, 1)))
    failures |= 1UL << 3;
  actual.v = simd_trn1(a.v, b.v);
  if (!same_vec(actual, expected_trn(a, b, 0)))
    failures |= 1UL << 4;
  actual.v = simd_trn2(a.v, b.v);
  if (!same_vec(actual, expected_trn(a, b, 1)))
    failures |= 1UL << 5;
  actual.v = simd_uzp1(a.v, b.v);
  if (!same_vec(actual, expected_uzp(a, b, 0)))
    failures |= 1UL << 6;
  actual.v = simd_uzp2(a.v, b.v);
  if (!same_vec(actual, expected_uzp(a, b, 1)))
    failures |= 1UL << 7;

  return failures == 0 ? 42 : (0xbad30000UL | failures);
}
