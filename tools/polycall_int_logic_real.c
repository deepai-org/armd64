static volatile unsigned long seed_a = 0x13579bdf2468ace0UL;
static volatile unsigned long seed_b = 0xfedcba9876543210UL;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long shift = (a3 + 9) & 63;
  unsigned long a = seed_a ^ a0;
  unsigned long b = seed_b + a1;
  unsigned long c = a & ~(b << (shift & 7));
  unsigned long d = a | ~(b >> ((shift + 11) & 15));
  unsigned long e = a ^ ~(a2 + (b << 1));
  unsigned long f = ~((a2 ^ b) >> ((shift + 3) & 31));

  if ((c & (d >> ((shift + 5) & 31))) != 0)
    e += 0x1020304050607080UL;
  else
    e -= 0x0102030405060708UL;

  return c ^ d ^ e ^ f;
}
