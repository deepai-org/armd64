static unsigned __int128 unsigned_value =
  (((unsigned __int128) 0x123456789abcdef0ULL) << 64) |
  0xfedcba9876543210ULL;
static __int128 signed_value =
  -((((__int128) 0x123456789abcdefULL) << 64) | 0x1234567890ULL);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  unsigned __int128 unsigned_divisor =
    (((unsigned __int128) (a + 1)) << 64) | (b + 17);
  __int128 signed_divisor = -(((__int128) (a + 3)) << 64) | (b + 19);
  unsigned __int128 unsigned_quotient = unsigned_value / unsigned_divisor;
  unsigned __int128 unsigned_remainder = unsigned_value % unsigned_divisor;
  __int128 signed_quotient = signed_value / signed_divisor;
  __int128 signed_remainder = signed_value % signed_divisor;

  return (unsigned long) unsigned_quotient +
    (unsigned long) (unsigned_quotient >> 64) +
    (unsigned long) unsigned_remainder +
    (unsigned long) (unsigned_remainder >> 64) +
    (unsigned long) signed_quotient +
    (unsigned long) (signed_quotient >> 64) +
    (unsigned long) signed_remainder +
    (unsigned long) (signed_remainder >> 64) + c;
}
