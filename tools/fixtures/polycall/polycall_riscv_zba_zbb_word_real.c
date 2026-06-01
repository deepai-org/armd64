static volatile unsigned long seed = 0x0123456789abcdefUL;

static unsigned int rotl32(unsigned int value, unsigned int shift)
{
  shift &= 31;
  return shift == 0 ? value : ((value << shift) | (value >> (32 - shift)));
}

static unsigned int rotr32(unsigned int value, unsigned int shift)
{
  shift &= 31;
  return shift == 0 ? value : ((value >> shift) | (value << (32 - shift)));
}

static unsigned long sign_extend32(unsigned int value)
{
  return (unsigned long) (long) (int) value;
}

static unsigned long clz32(unsigned int value)
{
  unsigned long count = 0;
  for (int bit = 31; bit >= 0; bit--) {
    if ((value >> bit) & 1U)
      break;
    count++;
  }
  return count;
}

static unsigned long ctz32(unsigned int value)
{
  unsigned long count = 0;
  while (((value >> count) & 1U) == 0)
    count++;
  return count;
}

static unsigned long cpop32(unsigned int value)
{
  unsigned long count = 0;
  while (value != 0) {
    count += value & 1U;
    value >>= 1;
  }
  return count;
}

static unsigned long rv_rolw(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("rolw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long rv_rorw(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("rorw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long rv_clzw(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("clzw %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long rv_ctzw(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("ctzw %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long rv_cpopw(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("cpopw %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long rv_zext_h(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("zext.h %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long rv_add_uw(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("add.uw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long rv_sh1add_uw(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("sh1add.uw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long rv_sh2add_uw(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("sh2add.uw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long rv_sh3add_uw(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("sh3add.uw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long rv_slli_uw(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("slli.uw %0,%1,3" : "=r"(result) : "r"(a));
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long value = seed ^ (a0 << 9) ^ (a1 << 17) ^ a2;
  unsigned long addend = (a3 << 23) ^ 0x23456789abcdef01UL;
  unsigned int word = ((unsigned int) value) | (1U << 5);
  unsigned int shift = (unsigned int) (a3 + 11) & 31U;
  unsigned long failures = 0;

  if (rv_rolw(word, shift) != sign_extend32(rotl32(word, shift)))
    failures |= 1UL << 0;
  if (rv_rorw(word, shift) != sign_extend32(rotr32(word, shift)))
    failures |= 1UL << 1;
  if (rv_clzw(word) != clz32(word))
    failures |= 1UL << 2;
  if (rv_ctzw(word) != ctz32(word))
    failures |= 1UL << 3;
  if (rv_cpopw(word) != cpop32(word))
    failures |= 1UL << 4;
  if (rv_zext_h(word) != (unsigned short) word)
    failures |= 1UL << 5;
  if (rv_add_uw(word, addend) != ((unsigned long) word + addend))
    failures |= 1UL << 6;
  if (rv_sh1add_uw(word, addend) != (((unsigned long) word << 1) + addend))
    failures |= 1UL << 7;
  if (rv_sh2add_uw(word, addend) != (((unsigned long) word << 2) + addend))
    failures |= 1UL << 8;
  if (rv_sh3add_uw(word, addend) != (((unsigned long) word << 3) + addend))
    failures |= 1UL << 9;
  if (rv_slli_uw(word) != ((unsigned long) word << 3))
    failures |= 1UL << 10;

  return failures == 0 ? 42 : (0xbad2000000000000UL | failures);
}
