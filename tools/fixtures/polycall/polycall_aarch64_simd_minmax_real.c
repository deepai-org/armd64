typedef signed char s8x16 __attribute__((vector_size(16)));
typedef signed short s16x8 __attribute__((vector_size(16)));
typedef signed int s32x4 __attribute__((vector_size(16)));
typedef unsigned char u8x16 __attribute__((vector_size(16)));
typedef unsigned short u16x8 __attribute__((vector_size(16)));
typedef unsigned int u32x4 __attribute__((vector_size(16)));

union sbytes {
  s8x16 v;
  signed char b[16];
};

union shwords {
  s16x8 v;
  signed short h[8];
};

union swords {
  s32x4 v;
  signed int w[4];
};

union ubytes {
  u8x16 v;
  unsigned char b[16];
};

union uhwords {
  u16x8 v;
  unsigned short h[8];
};

union uwords {
  u32x4 v;
  unsigned int w[4];
};

static union sbytes seed_sb0 = {
  .b = { -120, -7, 0, 1, 9, 23, 42, 100,
         127, -128, -64, 11, -3, 55, 88, -99 }
};

static union sbytes seed_sb1 = {
  .b = { -121, -6, 1, -1, 10, 22, 43, 99,
         126, -127, -65, 12, -4, 54, 89, -100 }
};

static union shwords seed_sh0 = {
  .h = { -32768, -1024, -1, 0, 17, 4096, 16384, 32767 }
};

static union shwords seed_sh1 = {
  .h = { -32767, -2048, 1, -2, 16, 4097, 16383, 32766 }
};

static union swords seed_sw0 = {
  .w = { (signed int) 0x80000000U, -77, 0x1020304, 0x7fffffff }
};

static union swords seed_sw1 = {
  .w = { (signed int) 0x80000001U, -78, 0x1020305, 0x7ffffffe }
};

static union ubytes seed_ub0 = {
  .b = { 0, 1, 2, 3, 17, 31, 63, 64, 65, 100, 127, 128, 129, 200, 254, 255 }
};

static union ubytes seed_ub1 = {
  .b = { 255, 254, 3, 2, 16, 32, 62, 65, 64, 101, 126, 129, 128, 199, 1, 0 }
};

static union uhwords seed_uh0 = {
  .h = { 0, 1, 255, 256, 4095, 4096, 32768, 65535 }
};

static union uhwords seed_uh1 = {
  .h = { 65535, 2, 254, 257, 4094, 4097, 32767, 0 }
};

static union uwords seed_uw0 = {
  .w = { 0, 1, 0x80000000U, 0xffffffffU }
};

static union uwords seed_uw1 = {
  .w = { 0xffffffffU, 2, 0x7fffffffU, 0 }
};

static s8x16 simd_smax_b(s8x16 a, s8x16 b)
{
  s8x16 result;
  __asm__ volatile("smax %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static s16x8 simd_smin_h(s16x8 a, s16x8 b)
{
  s16x8 result;
  __asm__ volatile("smin %0.8h,%1.8h,%2.8h"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static s32x4 simd_smax_s(s32x4 a, s32x4 b)
{
  s32x4 result;
  __asm__ volatile("smax %0.4s,%1.4s,%2.4s"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 simd_umax_b(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("umax %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u16x8 simd_umin_h(u16x8 a, u16x8 b)
{
  u16x8 result;
  __asm__ volatile("umin %0.8h,%1.8h,%2.8h"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u32x4 simd_umax_s(u32x4 a, u32x4 b)
{
  u32x4 result;
  __asm__ volatile("umax %0.4s,%1.4s,%2.4s"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static int same_sbytes(union sbytes actual, union sbytes expected)
{
  for (unsigned i = 0; i < 16; i++) {
    if (actual.b[i] != expected.b[i])
      return 0;
  }
  return 1;
}

static int same_shwords(union shwords actual, union shwords expected)
{
  for (unsigned i = 0; i < 8; i++) {
    if (actual.h[i] != expected.h[i])
      return 0;
  }
  return 1;
}

static int same_swords(union swords actual, union swords expected)
{
  for (unsigned i = 0; i < 4; i++) {
    if (actual.w[i] != expected.w[i])
      return 0;
  }
  return 1;
}

static int same_ubytes(union ubytes actual, union ubytes expected)
{
  for (unsigned i = 0; i < 16; i++) {
    if (actual.b[i] != expected.b[i])
      return 0;
  }
  return 1;
}

static int same_uhwords(union uhwords actual, union uhwords expected)
{
  for (unsigned i = 0; i < 8; i++) {
    if (actual.h[i] != expected.h[i])
      return 0;
  }
  return 1;
}

static int same_uwords(union uwords actual, union uwords expected)
{
  for (unsigned i = 0; i < 4; i++) {
    if (actual.w[i] != expected.w[i])
      return 0;
  }
  return 1;
}

static union sbytes expected_smax_b(union sbytes a, union sbytes b)
{
  union sbytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++)
    result.b[i] = a.b[i] >= b.b[i] ? a.b[i] : b.b[i];
  return result;
}

static union shwords expected_smin_h(union shwords a, union shwords b)
{
  union shwords result = { .h = { 0 } };
  for (unsigned i = 0; i < 8; i++)
    result.h[i] = a.h[i] <= b.h[i] ? a.h[i] : b.h[i];
  return result;
}

static union swords expected_smax_s(union swords a, union swords b)
{
  union swords result = { .w = { 0 } };
  for (unsigned i = 0; i < 4; i++)
    result.w[i] = a.w[i] >= b.w[i] ? a.w[i] : b.w[i];
  return result;
}

static union ubytes expected_umax_b(union ubytes a, union ubytes b)
{
  union ubytes result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++)
    result.b[i] = a.b[i] >= b.b[i] ? a.b[i] : b.b[i];
  return result;
}

static union uhwords expected_umin_h(union uhwords a, union uhwords b)
{
  union uhwords result = { .h = { 0 } };
  for (unsigned i = 0; i < 8; i++)
    result.h[i] = a.h[i] <= b.h[i] ? a.h[i] : b.h[i];
  return result;
}

static union uwords expected_umax_s(union uwords a, union uwords b)
{
  union uwords result = { .w = { 0 } };
  for (unsigned i = 0; i < 4; i++)
    result.w[i] = a.w[i] >= b.w[i] ? a.w[i] : b.w[i];
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  union sbytes sb0 = seed_sb0;
  union sbytes sb1 = seed_sb1;
  union shwords sh0 = seed_sh0;
  union shwords sh1 = seed_sh1;
  union swords sw0 = seed_sw0;
  union swords sw1 = seed_sw1;
  union ubytes ub0 = seed_ub0;
  union ubytes ub1 = seed_ub1;
  union uhwords uh0 = seed_uh0;
  union uhwords uh1 = seed_uh1;
  union uwords uw0 = seed_uw0;
  union uwords uw1 = seed_uw1;
  unsigned long failures = 0;

  sb0.b[0] ^= (signed char) a0;
  ub1.b[15] ^= (unsigned char) a1;
  sh0.h[3] ^= (signed short) a2;
  uw0.w[2] ^= (unsigned int) a3;

  union sbytes actual_sb;
  actual_sb.v = simd_smax_b(sb0.v, sb1.v);
  if (!same_sbytes(actual_sb, expected_smax_b(sb0, sb1)))
    failures |= 1UL << 0;

  union shwords actual_sh;
  actual_sh.v = simd_smin_h(sh0.v, sh1.v);
  if (!same_shwords(actual_sh, expected_smin_h(sh0, sh1)))
    failures |= 1UL << 1;

  union swords actual_sw;
  actual_sw.v = simd_smax_s(sw0.v, sw1.v);
  if (!same_swords(actual_sw, expected_smax_s(sw0, sw1)))
    failures |= 1UL << 2;

  union ubytes actual_ub;
  actual_ub.v = simd_umax_b(ub0.v, ub1.v);
  if (!same_ubytes(actual_ub, expected_umax_b(ub0, ub1)))
    failures |= 1UL << 3;

  union uhwords actual_uh;
  actual_uh.v = simd_umin_h(uh0.v, uh1.v);
  if (!same_uhwords(actual_uh, expected_umin_h(uh0, uh1)))
    failures |= 1UL << 4;

  union uwords actual_uw;
  actual_uw.v = simd_umax_s(uw0.v, uw1.v);
  if (!same_uwords(actual_uw, expected_umax_s(uw0, uw1)))
    failures |= 1UL << 5;

  return failures == 0 ? 42 : (0xbad60000UL | failures);
}
