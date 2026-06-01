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

static const union vec_words seed_a = {
  .w = { 0xcafebabeU, 0x8badf00dU, 0x0badc0deU, 0xfeedfaceU }
};

static const union vec_words expected_sm3ss1 = {
  .w = { 0x00000000U, 0x00000000U, 0x00000000U, 0x969ba59eU }
};

static const union vec_words expected_sm3partw1 = {
  .w = { 0xd2d6f0f4U, 0xd8e80627U, 0xf4c49282U, 0xd55fa228U }
};

static const union vec_words expected_sm3partw2 = {
  .w = { 0xa5f087d2U, 0xf6a7d485U, 0x2f9568c3U, 0x19007c45U }
};

static const union vec_words expected_sm3tt1a = {
  .w = { 0x89abcdefU, 0xb97531fdU, 0x76543210U, 0xa7a31e43U }
};

static const union vec_words expected_sm3tt1b = {
  .w = { 0x89abcdefU, 0xb97531fdU, 0x76543210U, 0x5fa1a583U }
};

static const union vec_words expected_sm3tt2a = {
  .w = { 0x89abcdefU, 0xd4c7f6e5U, 0x76543210U, 0x9cfdcd21U }
};

static const union vec_words expected_sm3tt2b = {
  .w = { 0x89abcdefU, 0xd4c7f6e5U, 0x76543210U, 0xc5a0181eU }
};

static u32x4 hw_sm3ss1(u32x4 n, u32x4 m, u32x4 a)
{
  u32x4 result;
  __asm__ volatile("sm3ss1 %0.4s,%1.4s,%2.4s,%3.4s"
      : "=w"(result) : "w"(n), "w"(m), "w"(a));
  return result;
}

static u32x4 hw_sm3partw1(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sm3partw1 %0.4s,%1.4s,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sm3partw2(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sm3partw2 %0.4s,%1.4s,%2.4s"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sm3tt1a(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sm3tt1a %0.4s,%1.4s,%2.s[1]"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sm3tt1b(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sm3tt1b %0.4s,%1.4s,%2.s[2]"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sm3tt2a(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sm3tt2a %0.4s,%1.4s,%2.s[3]"
      : "+w"(d) : "w"(n), "w"(m));
  return d;
}

static u32x4 hw_sm3tt2b(u32x4 d, u32x4 n, u32x4 m)
{
  __asm__ volatile("sm3tt2b %0.4s,%1.4s,%2.s[0]"
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

  actual.v = hw_sm3ss1(seed_n.v, seed_m.v, seed_a.v);
  if (!same_vec(actual, expected_sm3ss1))
    failures |= 1UL << 0;

  actual.v = hw_sm3partw1(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sm3partw1))
    failures |= 1UL << 1;

  actual.v = hw_sm3partw2(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sm3partw2))
    failures |= 1UL << 2;

  actual.v = hw_sm3tt1a(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sm3tt1a))
    failures |= 1UL << 3;

  actual.v = hw_sm3tt1b(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sm3tt1b))
    failures |= 1UL << 4;

  actual.v = hw_sm3tt2a(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sm3tt2a))
    failures |= 1UL << 5;

  actual.v = hw_sm3tt2b(seed_d.v, seed_n.v, seed_m.v);
  if (!same_vec(actual, expected_sm3tt2b))
    failures |= 1UL << 6;

  return failures == 0 ? 42 : (0xa3000000UL | failures);
}
