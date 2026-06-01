typedef unsigned long long u64x2 __attribute__((vector_size(16)));

union vec_dwords {
  u64x2 v;
  unsigned long long q[2];
};

static u64x2 hw_pmull(u64x2 a, u64x2 b)
{
  u64x2 result;
  __asm__ volatile("pmull %0.1q,%1.1d,%2.1d"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

static u64x2 hw_pmull2(u64x2 a, u64x2 b)
{
  u64x2 result;
  __asm__ volatile("pmull2 %0.1q,%1.2d,%2.2d"
      : "=w"(result) : "w"(a), "w"(b));
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const union vec_dwords a = {
    .q = { 0x0011223344556677ULL, 0x8899aabbccddeeffULL }
  };
  const union vec_dwords b = {
    .q = { 0x0f1e2d3c4b5a6978ULL, 0x1021324354657687ULL }
  };
  union vec_dwords result;
  unsigned long failures = 0;

  result.v = hw_pmull(a.v, b.v);
  if (result.q[0] != 0x1010ef1017e8e8e8ULL ||
      result.q[1] != 0x0000ff0007f8f8f8ULL)
    failures |= 1UL << 0;

  result.v = hw_pmull2(a.v, b.v);
  if (result.q[0] != 0x454c4548252c257dULL ||
      result.q[1] != 0x089819af4ada5bb8ULL)
    failures |= 1UL << 1;

  return failures == 0 ? 42 : (0xa6400000UL | failures);
}
