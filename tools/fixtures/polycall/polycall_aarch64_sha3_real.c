typedef unsigned long u64;
typedef u64 u64x2 __attribute__((vector_size(16)));

union vec_dwords {
  u64x2 v;
  u64 q[2];
};

static const union vec_dwords seed_n = {
  .q = { 0x0123456789abcdefUL, 0xfedcba9876543210UL }
};

static const union vec_dwords seed_m = {
  .q = { 0x0f1e2d3c4b5a6978UL, 0x8796a5b4c3d2e1f0UL }
};

static const union vec_dwords seed_a = {
  .q = { 0x13579bdf2468ace0UL, 0xdeadbeef10203040UL }
};

static const union vec_dwords expected_eor3 = {
  .q = { 0x1d6af384e6990877UL, 0xa7e7a1c3a5a6e3a0UL }
};

static const union vec_dwords expected_bcax = {
  .q = { 0x0d2b6147c2b98cf7UL, 0xffcebb88b586f3a0UL }
};

static const union vec_dwords expected_rax1 = {
  .q = { 0x1f1f1f1f1f1f1f1fUL, 0xf1f1f1f1f1f1f1f1UL }
};

static const union vec_dwords expected_xar = {
  .q = { 0xd24b871eb42de178UL, 0x69f03ca50f965ac3UL }
};

static u64x2 hw_eor3(u64x2 n, u64x2 m, u64x2 a)
{
  u64x2 result;
  __asm__ volatile("eor3 %0.16b,%1.16b,%2.16b,%3.16b"
      : "=w"(result) : "w"(n), "w"(m), "w"(a));
  return result;
}

static u64x2 hw_bcax(u64x2 n, u64x2 m, u64x2 a)
{
  u64x2 result;
  __asm__ volatile("bcax %0.16b,%1.16b,%2.16b,%3.16b"
      : "=w"(result) : "w"(n), "w"(m), "w"(a));
  return result;
}

static u64x2 hw_rax1(u64x2 n, u64x2 m)
{
  u64x2 result;
  __asm__ volatile("rax1 %0.2d,%1.2d,%2.2d"
      : "=w"(result) : "w"(n), "w"(m));
  return result;
}

static u64x2 hw_xar(u64x2 n, u64x2 m)
{
  u64x2 result;
  __asm__ volatile("xar %0.2d,%1.2d,%2.2d,#17"
      : "=w"(result) : "w"(n), "w"(m));
  return result;
}

static int same_vec(union vec_dwords actual, union vec_dwords expected)
{
  for (unsigned i = 0; i < 2; i++) {
    if (actual.q[i] != expected.q[i])
      return 0;
  }
  return 1;
}

__attribute__((visibility("default")))
u64 poly_entry(void)
{
  union vec_dwords actual;
  u64 failures = 0;

  actual.v = hw_eor3(seed_n.v, seed_m.v, seed_a.v);
  if (!same_vec(actual, expected_eor3))
    failures |= 1UL << 0;

  actual.v = hw_bcax(seed_n.v, seed_m.v, seed_a.v);
  if (!same_vec(actual, expected_bcax))
    failures |= 1UL << 1;

  actual.v = hw_rax1(seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_rax1))
    failures |= 1UL << 2;

  actual.v = hw_xar(seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_xar))
    failures |= 1UL << 3;

  return failures == 0 ? 42 : (0xa3000000UL | failures);
}
