static volatile unsigned long seed = 0x13579bdf2468ace0UL;

static unsigned long zbs_bset(unsigned long value, unsigned long bit)
{
  unsigned long result;
  __asm__ volatile("bset %0,%1,%2" : "=r"(result) : "r"(value), "r"(bit));
  return result;
}

static unsigned long zbs_bclr(unsigned long value, unsigned long bit)
{
  unsigned long result;
  __asm__ volatile("bclr %0,%1,%2" : "=r"(result) : "r"(value), "r"(bit));
  return result;
}

static unsigned long zbs_binv(unsigned long value, unsigned long bit)
{
  unsigned long result;
  __asm__ volatile("binv %0,%1,%2" : "=r"(result) : "r"(value), "r"(bit));
  return result;
}

static unsigned long zbs_bext(unsigned long value, unsigned long bit)
{
  unsigned long result;
  __asm__ volatile("bext %0,%1,%2" : "=r"(result) : "r"(value), "r"(bit));
  return result;
}

static unsigned long zbs_bseti(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("bseti %0,%1,5" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long zbs_bclri(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("bclri %0,%1,9" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long zbs_binvi(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("binvi %0,%1,17" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long zbs_bexti(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("bexti %0,%1,23" : "=r"(result) : "r"(value));
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long value = seed ^ (a0 << 7) ^ (a1 << 13) ^ a2;
  unsigned long bit_index = (a3 + 37) & 63;
  unsigned long bit = 1UL << bit_index;
  unsigned long failures = 0;

  if (zbs_bset(value, bit_index) != (value | bit))
    failures |= 1UL << 0;
  if (zbs_bclr(value, bit_index) != (value & ~bit))
    failures |= 1UL << 1;
  if (zbs_binv(value, bit_index) != (value ^ bit))
    failures |= 1UL << 2;
  if (zbs_bext(value, bit_index) != ((value >> bit_index) & 1UL))
    failures |= 1UL << 3;
  if (zbs_bseti(value) != (value | (1UL << 5)))
    failures |= 1UL << 4;
  if (zbs_bclri(value) != (value & ~(1UL << 9)))
    failures |= 1UL << 5;
  if (zbs_binvi(value) != (value ^ (1UL << 17)))
    failures |= 1UL << 6;
  if (zbs_bexti(value) != ((value >> 23) & 1UL))
    failures |= 1UL << 7;

  return failures == 0 ? 42 : (0xbad00000UL | failures);
}
