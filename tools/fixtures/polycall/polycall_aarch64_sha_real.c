typedef unsigned int u32;
typedef unsigned long u64;
typedef u32 u32x4 __attribute__((vector_size(16)));

union vec_words {
  u32x4 v;
  u32 w[4];
};

static const union vec_words seed_d = {
  .w = { 0x01234567U, 0x89abcdefU, 0xfedcba98U, 0x76543210U }
};

static const union vec_words seed_n = {
  .w = { 0x0f1e2d3cU, 0x4b5a6978U, 0x8796a5b4U, 0xc3d2e1f0U }
};

static const union vec_words seed_m = {
  .w = { 0x13579bdfU, 0x2468ace0U, 0xdeadbeefU, 0x10203040U }
};

static const union vec_words expected_sha256h = {
  .w = { 0x10ca70b9U, 0x71bf358eU, 0x6da55c11U, 0x2b813f33U }
};

static const union vec_words expected_sha256h2 = {
  .w = { 0xab2c0960U, 0xe5ac5712U, 0x02aaf8d8U, 0xeec4cadbU }
};

static const union vec_words expected_sha256su0 = {
  .w = { 0x3e8111b3U, 0x8a2bdf80U, 0x217eee4bU, 0x69072c4aU }
};

static const union vec_words expected_sha256su1 = {
  .w = { 0xb51b6ecbU, 0x2f6e75bbU, 0x9d41877dU, 0x7e0cf7b6U }
};

static u32x4 hw_sha256h(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sha256h %q0,%q1,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sha256h2(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sha256h2 %q0,%q1,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sha256su0(u32x4 d, u32x4 n)
{
  __asm__ volatile("sha256su0 %0.4s,%1.4s"
      : "+w"(d) : "w"(n));
  return d;
}

static u32x4 hw_sha256su1(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sha256su1 %0.4s,%1.4s,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static int same_vec(union vec_words actual, union vec_words expected)
{
  for (unsigned i = 0; i < 4; i++) {
    if (actual.w[i] != expected.w[i])
      return 0;
  }
  return 1;
}

__attribute__((visibility("default")))
u64 poly_entry(void)
{
  union vec_words actual;
  u64 failures = 0;

  actual.v = hw_sha256h(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha256h))
    failures |= 1UL << 0;

  actual.v = hw_sha256h2(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha256h2))
    failures |= 1UL << 1;

  actual.v = hw_sha256su0(seed_d.v, seed_n.v);
  if (!same_vec(actual, expected_sha256su0))
    failures |= 1UL << 2;

  actual.v = hw_sha256su1(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha256su1))
    failures |= 1UL << 3;

  return failures == 0 ? 42 : (0xa2560000UL | failures);
}
