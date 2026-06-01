typedef unsigned long u64;
typedef u64 u64x2 __attribute__((vector_size(16)));

union vec_dwords {
  u64x2 v;
  u64 q[2];
};

static const union vec_dwords seed_d = {
  .q = { 0x0123456789abcdefUL, 0xfedcba9876543210UL }
};

static const union vec_dwords seed_n = {
  .q = { 0x0f1e2d3c4b5a6978UL, 0x8796a5b4c3d2e1f0UL }
};

static const union vec_dwords seed_m = {
  .q = { 0x13579bdf2468ace0UL, 0xdeadbeef10203040UL }
};

static const union vec_dwords expected_sha512h = {
  .q = { 0x14fed81b43f97fe3UL, 0xc89644daee8fe657UL }
};

static const union vec_dwords expected_sha512h2 = {
  .q = { 0xcfaebb77d42c6834UL, 0x4fd4aef868474ea2UL }
};

static const union vec_dwords expected_sha512su0 = {
  .q = { 0x6f907deb1d5cb34dUL, 0x7e7aef81d7c50c17UL }
};

static const union vec_dwords expected_sha512su1 = {
  .q = { 0x4a5cf1fc1c6e25ddUL, 0x401fe1499fa33ec9UL }
};

static u64x2 hw_sha512h(u64x2 d, u64x2 n, u64x2 m)
{
  __asm__ volatile("sha512h %q0,%q1,%2.2d"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u64x2 hw_sha512h2(u64x2 d, u64x2 n, u64x2 m)
{
  __asm__ volatile("sha512h2 %q0,%q1,%2.2d"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u64x2 hw_sha512su0(u64x2 d, u64x2 n)
{
  __asm__ volatile("sha512su0 %0.2d,%1.2d"
      : "+w"(d) : "w"(n));
  return d;
}

static u64x2 hw_sha512su1(u64x2 d, u64x2 n, u64x2 m)
{
  __asm__ volatile("sha512su1 %0.2d,%1.2d,%2.2d"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
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

  actual.v = hw_sha512h(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha512h))
    failures |= 1UL << 0;

  actual.v = hw_sha512h2(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha512h2))
    failures |= 1UL << 1;

  actual.v = hw_sha512su0(seed_d.v, seed_n.v);
  if (!same_vec(actual, expected_sha512su0))
    failures |= 1UL << 2;

  actual.v = hw_sha512su1(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha512su1))
    failures |= 1UL << 3;

  return failures == 0 ? 42 : (0xa5120000UL | failures);
}
