typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef s8 s8x16 __attribute__((vector_size(16)));
typedef s16 s16x8 __attribute__((vector_size(16)));
typedef s32 s32x4 __attribute__((vector_size(16)));
typedef u8 u8x16 __attribute__((vector_size(16)));
typedef u16 u16x8 __attribute__((vector_size(16)));
typedef u32 u32x4 __attribute__((vector_size(16)));

union vec128 {
  s8x16 sb;
  s16x8 sh;
  s32x4 sw;
  u8x16 ub;
  u16x8 uh;
  u32x4 uw;
  u8 b[16];
  u16 h[8];
  u32 w[4];
};

static const union vec128 seed_a = {
  .b = { 0x80, 0x81, 0xf0, 0xff, 0x00, 0x01, 0x10, 0x7f,
         0x11, 0x22, 0x33, 0x44, 0x55, 0xaa, 0xee, 0xff }
};

static const union vec128 seed_b = {
  .b = { 0x7f, 0x01, 0x10, 0x02, 0xff, 0x80, 0xf0, 0x00,
         0x88, 0x44, 0x22, 0x11, 0xaa, 0x55, 0x0f, 0xfe }
};

static const union vec128 seed_acc = {
  .b = { 0x01, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f,
         0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 }
};

static s8x16 hw_sabd_b(s8x16 a, s8x16 b)
{
  s8x16 result;
  __asm__ volatile("sabd %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static s16x8 hw_sabd_h(s16x8 a, s16x8 b)
{
  s16x8 result;
  __asm__ volatile("sabd %0.8h,%1.8h,%2.8h"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static s32x4 hw_sabd_s(s32x4 a, s32x4 b)
{
  s32x4 result;
  __asm__ volatile("sabd %0.4s,%1.4s,%2.4s"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u8x16 hw_uabd_b(u8x16 a, u8x16 b)
{
  u8x16 result;
  __asm__ volatile("uabd %0.16b,%1.16b,%2.16b"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u16x8 hw_uabd_h(u16x8 a, u16x8 b)
{
  u16x8 result;
  __asm__ volatile("uabd %0.8h,%1.8h,%2.8h"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u32x4 hw_uabd_s(u32x4 a, u32x4 b)
{
  u32x4 result;
  __asm__ volatile("uabd %0.4s,%1.4s,%2.4s"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static s8x16 hw_saba_b(s8x16 d, s8x16 a, s8x16 b)
{
  __asm__ volatile("saba %0.16b,%1.16b,%2.16b"
      : "+w"(d) : "w"(a), "w"(b));
  return d;
}

static u16x8 hw_uaba_h(u16x8 d, u16x8 a, u16x8 b)
{
  __asm__ volatile("uaba %0.8h,%1.8h,%2.8h"
      : "+w"(d) : "w"(a), "w"(b));
  return d;
}

static int same_vec(union vec128 actual, union vec128 expected)
{
  for (unsigned i = 0; i < 16; i++) {
    if (actual.b[i] != expected.b[i])
      return 0;
  }
  return 1;
}

static union vec128 expected_sabd_b(union vec128 a, union vec128 b)
{
  union vec128 result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++) {
    int diff = (int) a.sb[i] - (int) b.sb[i];
    result.b[i] = (u8) (diff < 0 ? -diff : diff);
  }
  return result;
}

static union vec128 expected_sabd_h(union vec128 a, union vec128 b)
{
  union vec128 result = { .b = { 0 } };
  for (unsigned i = 0; i < 8; i++) {
    int diff = (int) a.sh[i] - (int) b.sh[i];
    result.h[i] = (u16) (diff < 0 ? -diff : diff);
  }
  return result;
}

static union vec128 expected_sabd_s(union vec128 a, union vec128 b)
{
  union vec128 result = { .b = { 0 } };
  for (unsigned i = 0; i < 4; i++) {
    long long diff = (long long) a.sw[i] - (long long) b.sw[i];
    result.w[i] = (u32) (diff < 0 ? -diff : diff);
  }
  return result;
}

static union vec128 expected_uabd_b(union vec128 a, union vec128 b)
{
  union vec128 result = { .b = { 0 } };
  for (unsigned i = 0; i < 16; i++)
    result.b[i] = a.b[i] >= b.b[i] ? a.b[i] - b.b[i] : b.b[i] - a.b[i];
  return result;
}

static union vec128 expected_uabd_h(union vec128 a, union vec128 b)
{
  union vec128 result = { .b = { 0 } };
  for (unsigned i = 0; i < 8; i++)
    result.h[i] = a.h[i] >= b.h[i] ? a.h[i] - b.h[i] : b.h[i] - a.h[i];
  return result;
}

static union vec128 expected_uabd_s(union vec128 a, union vec128 b)
{
  union vec128 result = { .b = { 0 } };
  for (unsigned i = 0; i < 4; i++)
    result.w[i] = a.w[i] >= b.w[i] ? a.w[i] - b.w[i] : b.w[i] - a.w[i];
  return result;
}

static union vec128 expected_saba_b(union vec128 d, union vec128 a,
    union vec128 b)
{
  union vec128 diff = expected_sabd_b(a, b);
  for (unsigned i = 0; i < 16; i++)
    d.b[i] = (u8) (d.b[i] + diff.b[i]);
  return d;
}

static union vec128 expected_uaba_h(union vec128 d, union vec128 a,
    union vec128 b)
{
  union vec128 diff = expected_uabd_h(a, b);
  for (unsigned i = 0; i < 8; i++)
    d.h[i] = (u16) (d.h[i] + diff.h[i]);
  return d;
}

__attribute__((visibility("default")))
u64 poly_entry(void)
{
  union vec128 actual;
  u64 failures = 0;

  actual.sb = hw_sabd_b(seed_a.sb, seed_b.sb);
  if (!same_vec(actual, expected_sabd_b(seed_a, seed_b)))
    failures |= 1UL << 0;

  actual.sh = hw_sabd_h(seed_a.sh, seed_b.sh);
  if (!same_vec(actual, expected_sabd_h(seed_a, seed_b)))
    failures |= 1UL << 1;

  actual.sw = hw_sabd_s(seed_a.sw, seed_b.sw);
  if (!same_vec(actual, expected_sabd_s(seed_a, seed_b)))
    failures |= 1UL << 2;

  actual.ub = hw_uabd_b(seed_a.ub, seed_b.ub);
  if (!same_vec(actual, expected_uabd_b(seed_a, seed_b)))
    failures |= 1UL << 3;

  actual.uh = hw_uabd_h(seed_a.uh, seed_b.uh);
  if (!same_vec(actual, expected_uabd_h(seed_a, seed_b)))
    failures |= 1UL << 4;

  actual.uw = hw_uabd_s(seed_a.uw, seed_b.uw);
  if (!same_vec(actual, expected_uabd_s(seed_a, seed_b)))
    failures |= 1UL << 5;

  actual.sb = hw_saba_b(seed_acc.sb, seed_a.sb, seed_b.sb);
  if (!same_vec(actual, expected_saba_b(seed_acc, seed_a, seed_b)))
    failures |= 1UL << 6;

  actual.uh = hw_uaba_h(seed_acc.uh, seed_a.uh, seed_b.uh);
  if (!same_vec(actual, expected_uaba_h(seed_acc, seed_a, seed_b)))
    failures |= 1UL << 7;

  return failures == 0 ? 42 : (0xa6500000UL | failures);
}
