static volatile unsigned long seed_a = 0x9e3779b97f4a7c15UL;
static volatile unsigned long seed_b = 0xfedcba9876543210UL;

static unsigned long rol64(unsigned long value, unsigned int shift)
{
  shift &= 63;
  return shift == 0 ? value : ((value << shift) | (value >> (64 - shift)));
}

static unsigned long ror64(unsigned long value, unsigned int shift)
{
  shift &= 63;
  return shift == 0 ? value : ((value >> shift) | (value << (64 - shift)));
}

static unsigned long count_leading_zeroes(unsigned long value)
{
  unsigned long count = 0;
  for (int bit = 63; bit >= 0; bit--) {
    if ((value >> bit) & 1UL)
      break;
    count++;
  }
  return count;
}

static unsigned long count_trailing_zeroes(unsigned long value)
{
  unsigned long count = 0;
  while (((value >> count) & 1UL) == 0)
    count++;
  return count;
}

static unsigned long count_ones(unsigned long value)
{
  unsigned long count = 0;
  while (value != 0) {
    count += value & 1UL;
    value >>= 1;
  }
  return count;
}

static unsigned long orc_b_expected(unsigned long value)
{
  unsigned long result = 0;
  for (unsigned i = 0; i < 8; i++) {
    unsigned long byte = (value >> (i * 8)) & 0xffUL;
    if (byte != 0)
      result |= 0xffUL << (i * 8);
  }
  return result;
}

static unsigned long rev8_expected(unsigned long value)
{
  unsigned long result = 0;
  for (unsigned i = 0; i < 8; i++)
    result |= ((value >> (i * 8)) & 0xffUL) << ((7 - i) * 8);
  return result;
}

static unsigned long zbb_andn(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("andn %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbb_orn(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("orn %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbb_xnor(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("xnor %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbb_min(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("min %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbb_maxu(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("maxu %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbb_rol(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("rol %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbb_ror(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("ror %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zbb_clz(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("clz %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long zbb_ctz(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("ctz %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long zbb_cpop(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("cpop %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long zbb_sext_b(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("sext.b %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long zbb_sext_h(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("sext.h %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long zbb_orc_b(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("orc.b %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long zbb_rev8(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("rev8 %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long zba_sh1add(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("sh1add %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zba_sh2add(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("sh2add %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zba_sh3add(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("sh3add %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zba_add_uw(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("add.uw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zba_sh2add_uw(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("sh2add.uw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zicond_eqz(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("czero.eqz %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long zicond_nez(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("czero.nez %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long a = seed_a ^ (a0 << 11) ^ a2;
  unsigned long b = seed_b + (a1 << 17) + a3;
  unsigned long nonzero = (a3 | 1UL) & 63UL;
  unsigned long failures = 0;

  if (zbb_andn(a, b) != (a & ~b))
    failures |= 1UL << 0;
  if (zbb_orn(a, b) != (a | ~b))
    failures |= 1UL << 1;
  if (zbb_xnor(a, b) != ~(a ^ b))
    failures |= 1UL << 2;
  if (zbb_min(a, b) != ((long) a < (long) b ? a : b))
    failures |= 1UL << 3;
  if (zbb_maxu(a, b) != (a > b ? a : b))
    failures |= 1UL << 4;
  if (zbb_rol(a, nonzero) != rol64(a, nonzero))
    failures |= 1UL << 5;
  if (zbb_ror(a, nonzero) != ror64(a, nonzero))
    failures |= 1UL << 6;
  if (zbb_clz(a) != count_leading_zeroes(a))
    failures |= 1UL << 7;
  if (zbb_ctz(a) != count_trailing_zeroes(a))
    failures |= 1UL << 8;
  if (zbb_cpop(a) != count_ones(a))
    failures |= 1UL << 9;
  if (zbb_sext_b(a) != (unsigned long) (long) (signed char) a)
    failures |= 1UL << 10;
  if (zbb_sext_h(a) != (unsigned long) (long) (short) a)
    failures |= 1UL << 11;
  if (zbb_orc_b(a) != orc_b_expected(a))
    failures |= 1UL << 12;
  if (zbb_rev8(a) != rev8_expected(a))
    failures |= 1UL << 13;
  if (zba_sh1add(a, b) != ((a << 1) + b))
    failures |= 1UL << 14;
  if (zba_sh2add(a, b) != ((a << 2) + b))
    failures |= 1UL << 15;
  if (zba_sh3add(a, b) != ((a << 3) + b))
    failures |= 1UL << 16;
  if (zba_add_uw(a, b) != ((unsigned int) a + b))
    failures |= 1UL << 17;
  if (zba_sh2add_uw(a, b) != (((unsigned long) (unsigned int) a << 2) + b))
    failures |= 1UL << 18;
  if (zicond_eqz(a, 0) != 0)
    failures |= 1UL << 19;
  if (zicond_eqz(a, b) != a)
    failures |= 1UL << 20;
  if (zicond_nez(a, 0) != a)
    failures |= 1UL << 21;
  if (zicond_nez(a, b) != 0)
    failures |= 1UL << 22;

  return failures == 0 ? 42 : (0xbad1000000000000UL | failures);
}
