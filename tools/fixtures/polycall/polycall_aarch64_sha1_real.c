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

static const union vec_words expected_sha1c = {
  .w = { 0x4612ed1fU, 0xe410f76aU, 0xd4487b54U, 0xd16ecc24U }
};

static const union vec_words expected_sha1p = {
  .w = { 0xf86e5bc2U, 0x58d90a7eU, 0x6e2255efU, 0x92006ed8U }
};

static const union vec_words expected_sha1m = {
  .w = { 0xb9189111U, 0x2fb20548U, 0xcd237934U, 0xd16ecc24U }
};

static const union vec_words expected_sha1su0 = {
  .w = { 0xeca86420U, 0xdb97531fU, 0x2f6f294bU, 0x2d2e6b28U }
};

static const union vec_words expected_sha1su1 = {
  .w = { 0x94f2583eU, 0x1c7ad0b6U, 0x7a1cb6d0U, 0xc54cd45dU }
};

static u32x4 hw_sha1c(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sha1c %q0,%s1,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sha1p(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sha1p %q0,%s1,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sha1m(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sha1m %q0,%s1,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32 hw_sha1h(u32x4 n)
{
  union vec_words result;
  __asm__ volatile("sha1h %s0,%s1" : "=w"(result.v) : "w"(n));
  return result.w[0];
}

static u32x4 hw_sha1su0(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sha1su0 %0.4s,%1.4s,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sha1su1(u32x4 d, u32x4 n)
{
  __asm__ volatile("sha1su1 %0.4s,%1.4s"
      : "+w"(d) : "w"(n));
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

  actual.v = hw_sha1c(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha1c))
    failures |= 1UL << 0;

  actual.v = hw_sha1p(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha1p))
    failures |= 1UL << 1;

  actual.v = hw_sha1m(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha1m))
    failures |= 1UL << 2;

  if (hw_sha1h(seed_d.v) != 0xc048d159U)
    failures |= 1UL << 3;

  actual.v = hw_sha1su0(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sha1su0))
    failures |= 1UL << 4;

  actual.v = hw_sha1su1(seed_d.v, seed_n.v);
  if (!same_vec(actual, expected_sha1su1))
    failures |= 1UL << 5;

  return failures == 0 ? 42 : (0xa1510000UL | failures);
}
