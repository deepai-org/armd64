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

static const union vec_words expected_sm4e = {
  .w = { 0x7d05e4abU, 0x851257f1U, 0xd8339f3bU, 0xcd9cbd0aU }
};

static const union vec_words expected_sm4ekey = {
  .w = { 0x52aa469aU, 0x412bd8e7U, 0x0fd867b0U, 0x805c06fbU }
};

static u32x4 hw_sm4e(u32x4 d, u32x4 n)
{
  __asm__ volatile("sm4e %0.4s,%1.4s" : "+w"(d) : "w"(n));
  return d;
}

static u32x4 hw_sm4ekey(u32x4 n, u32x4 m)
{
  u32x4 result;
  __asm__ volatile("sm4ekey %0.4s,%1.4s,%2.4s"
      : "=w"(result) : "w"(n), "w"(m));
  return result;
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

  actual.v = hw_sm4e(seed_d.v, seed_n.v);
  if (!same_vec(actual, expected_sm4e))
    failures |= 1UL << 0;

  actual.v = hw_sm4ekey(seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sm4ekey))
    failures |= 1UL << 1;

  return failures == 0 ? 42 : (0xa4000000UL | failures);
}
