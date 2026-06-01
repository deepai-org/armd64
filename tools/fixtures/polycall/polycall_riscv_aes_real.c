static unsigned long aes64es(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("aes64es %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long aes64esm(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("aes64esm %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long aes64ds(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("aes64ds %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long aes64dsm(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("aes64dsm %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

static unsigned long aes64im(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("aes64im %0,%1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long aes64ks1i_1(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("aes64ks1i %0,%1,1" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long aes64ks1i_10(unsigned long a)
{
  unsigned long result;
  __asm__ volatile("aes64ks1i %0,%1,10" : "=r"(result) : "r"(a));
  return result;
}

static unsigned long aes64ks2(unsigned long a, unsigned long b)
{
  unsigned long result;
  __asm__ volatile("aes64ks2 %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const unsigned long a = 0x0011223344556677UL;
  const unsigned long b = 0x8899aabbccddeeffUL;
  const unsigned long c = 0x0f1e2d3c4b5a6978UL;
  unsigned long failures = 0;

  if (aes64es(a, b) != 0x1bee28c3c4c193f5UL)
    failures |= 1UL << 0;
  if (aes64esm(a, b) != 0xae01a110c5a8545aUL)
    failures |= 1UL << 1;
  if (aes64ds(a, b) != 0x27f9d36652c96202UL)
    failures |= 1UL << 2;
  if (aes64dsm(a, b) != 0x460a644350878da1UL)
    failures |= 1UL << 3;
  if (aes64im(c) != 0xe1b4c396a5f087d2UL)
    failures |= 1UL << 4;
  if (aes64ks1i_1(c) != 0xeb7672daeb7672daUL)
    failures |= 1UL << 5;
  if (aes64ks1i_10(c) != 0x7672d8eb7672d8ebUL)
    failures |= 1UL << 6;
  if (aes64ks2(c, b) != 0x4b5a6978c3c3c3c3UL)
    failures |= 1UL << 7;

  return failures == 0 ? 42 : (0xbae60000UL | failures);
}
