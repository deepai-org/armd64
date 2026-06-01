static unsigned long zbc_clmul(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("clmul %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbc_clmulh(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("clmulh %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbc_clmulr(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("clmulr %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned __int128 carryless_product64(unsigned long a, unsigned long b)
{
  unsigned __int128 result = 0;
  for (unsigned bit = 0; bit < 64; bit++) {
    if ((b & (1UL << bit)) != 0)
      result ^= ((unsigned __int128) a) << bit;
  }
  return result;
}

static unsigned long expected_clmul(unsigned long a, unsigned long b)
{
  return (unsigned long) carryless_product64(a, b);
}

static unsigned long expected_clmulh(unsigned long a, unsigned long b)
{
  return (unsigned long) (carryless_product64(a, b) >> 64);
}

static unsigned long expected_clmulr(unsigned long a, unsigned long b)
{
  return (unsigned long) (carryless_product64(a, b) >> 63);
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long a = 0xfedcba9876543210UL ^ a0 ^ (a2 << 17);
  unsigned long b = 0x0123456789abcdefUL ^ (a1 << 1) ^ (a3 >> 3);
  unsigned long c = 0x8000000000000001UL ^ (a0 << 7) ^ a3;
  unsigned long d = 0x0001000100010001UL ^ (a1 >> 5) ^ (a2 << 9);
  unsigned long failures = 0;

  if (zbc_clmul(a, b) != expected_clmul(a, b))
    failures |= 1UL << 0;
  if (zbc_clmulh(a, b) != expected_clmulh(a, b))
    failures |= 1UL << 1;
  if (zbc_clmulr(a, b) != expected_clmulr(a, b))
    failures |= 1UL << 2;

  if (zbc_clmul(c, d) != expected_clmul(c, d))
    failures |= 1UL << 3;
  if (zbc_clmulh(c, d) != expected_clmulh(c, d))
    failures |= 1UL << 4;
  if (zbc_clmulr(c, d) != expected_clmulr(c, d))
    failures |= 1UL << 5;

  return failures == 0 ? 42 : (0xbad70000UL | failures);
}
