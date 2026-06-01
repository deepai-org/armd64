typedef unsigned char u8x16 __attribute__((vector_size(16)));

union vec_bytes {
  u8x16 v;
  unsigned char b[16];
};

static union vec_bytes seed_a = {
  .b = { 0x81, 0x02, 0x73, 0x44, 0xb5, 0x66, 0x37, 0xc8,
         0x19, 0xea, 0x5b, 0x2c, 0xdd, 0x0e, 0x9f, 0x70 }
};

static union vec_bytes seed_b = {
  .b = { 0x3c, 0xa5, 0x5a, 0xc3, 0x18, 0x81, 0x7e, 0xe7,
         0x24, 0x8d, 0x42, 0xab, 0x30, 0x99, 0x6e, 0xd7 }
};

static u8x16 simd_shl(u8x16 source)
{
  u8x16 result;
  __asm__ volatile("shl %0.16b,%1.16b,#1"
      : "=w"(result) : "w"(source));
  return result;
}

static u8x16 simd_ushr(u8x16 source)
{
  u8x16 result;
  __asm__ volatile("ushr %0.16b,%1.16b,#2"
      : "=w"(result) : "w"(source));
  return result;
}

static u8x16 simd_sshr(u8x16 source)
{
  u8x16 result;
  __asm__ volatile("sshr %0.16b,%1.16b,#3"
      : "=w"(result) : "w"(source));
  return result;
}

static u8x16 simd_sri(u8x16 dest, u8x16 source)
{
  __asm__ volatile("sri %0.16b,%1.16b,#2"
      : "+w"(dest) : "w"(source));
  return dest;
}

static u8x16 simd_sli(u8x16 dest, u8x16 source)
{
  __asm__ volatile("sli %0.16b,%1.16b,#3"
      : "+w"(dest) : "w"(source));
  return dest;
}

static int same_vec(union vec_bytes actual, union vec_bytes expected)
{
  for (unsigned i = 0; i < 16; i++) {
    if (actual.b[i] != expected.b[i])
      return 0;
  }
  return 1;
}

static union vec_bytes expected_shl(union vec_bytes source)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++)
    result.b[i] = (unsigned char) (source.b[i] << 1);
  return result;
}

static union vec_bytes expected_ushr(union vec_bytes source)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++)
    result.b[i] = (unsigned char) (source.b[i] >> 2);
  return result;
}

static union vec_bytes expected_sshr(union vec_bytes source)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++)
    result.b[i] = (unsigned char) (((signed char) source.b[i]) >> 3);
  return result;
}

static union vec_bytes expected_sri(union vec_bytes dest,
    union vec_bytes source)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++) {
    unsigned keep = dest.b[i] & 0xc0;
    unsigned insert = source.b[i] >> 2;
    result.b[i] = (unsigned char) (keep | insert);
  }
  return result;
}

static union vec_bytes expected_sli(union vec_bytes dest,
    union vec_bytes source)
{
  union vec_bytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++) {
    unsigned keep = dest.b[i] & 0x07;
    unsigned insert = (source.b[i] << 3) & 0xff;
    result.b[i] = (unsigned char) (keep | insert);
  }
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  union vec_bytes a = seed_a;
  union vec_bytes b = seed_b;
  unsigned long failures = 0;
  union vec_bytes actual;

  a.b[0] ^= (unsigned char) a0;
  a.b[5] ^= (unsigned char) a1;
  b.b[9] ^= (unsigned char) a2;
  b.b[15] ^= (unsigned char) a3;

  actual.v = simd_shl(a.v);
  if (!same_vec(actual, expected_shl(a)))
    failures |= 1UL << 0;
  actual.v = simd_ushr(a.v);
  if (!same_vec(actual, expected_ushr(a)))
    failures |= 1UL << 1;
  actual.v = simd_sshr(a.v);
  if (!same_vec(actual, expected_sshr(a)))
    failures |= 1UL << 2;
  actual.v = simd_sri(b.v, a.v);
  if (!same_vec(actual, expected_sri(b, a)))
    failures |= 1UL << 3;
  actual.v = simd_sli(b.v, a.v);
  if (!same_vec(actual, expected_sli(b, a)))
    failures |= 1UL << 4;

  return failures == 0 ? 42 : (0xbad40000UL | failures);
}
