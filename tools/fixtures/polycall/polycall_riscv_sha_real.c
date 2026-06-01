static unsigned long sha256sig0(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("sha256sig0 %0,%1" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long sha256sig1(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("sha256sig1 %0,%1" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long sha256sum0(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("sha256sum0 %0,%1" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long sha256sum1(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("sha256sum1 %0,%1" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long sha512sig0(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("sha512sig0 %0,%1" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long sha512sig1(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("sha512sig1 %0,%1" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long sha512sum0(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("sha512sum0 %0,%1" : "=r"(result) : "r"(value));
  return result;
}

static unsigned long sha512sum1(unsigned long value)
{
  unsigned long result;
  __asm__ volatile("sha512sum1 %0,%1" : "=r"(result) : "r"(value));
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const unsigned long x = 0x0123456789abcdefUL;
  const unsigned long y = 0xfedcba9876543210UL;
  unsigned long failures = 0;

  if (sha256sig0(x) != 0x000000003d5dcc4cUL)
    failures |= 1UL << 0;
  if (sha256sig1(x) != 0xffffffff9f685f13UL)
    failures |= 1UL << 1;
  if (sha256sum0(x) != 0x0000000022210003UL)
    failures |= 1UL << 2;
  if (sha256sum1(x) != 0xffffffffd6316d8aUL)
    failures |= 1UL << 3;
  if (sha512sig0(y) != 0x6e6d388393b0e55eUL)
    failures |= 1UL << 4;
  if (sha512sig1(y) != 0x735cb9f2442bce85UL)
    failures |= 1UL << 5;
  if (sha512sum0(y) != 0x483a85eff3813e54UL)
    failures |= 1UL << 6;
  if (sha512sum1(y) != 0x88fceedcccb8aa98UL)
    failures |= 1UL << 7;

  return failures == 0 ? 42 : (0x5a120000UL | failures);
}
