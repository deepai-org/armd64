static volatile unsigned long seed = 7;
static unsigned __int128 unsigned_value =
  (((unsigned __int128) 0x1020304050607080ULL) << 64) |
  0x90a0b0c0d0e0f001ULL;
static __int128 signed_value =
  -((((__int128) 0x102030405060708ULL) << 64) | 0x90a0b0c0d0e0f001ULL);
static double positive_value = 123456789.25;
static double negative_value = -98765.75;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  unsigned __int128 from_double =
    (unsigned __int128) (positive_value + (double) a);
  __int128 signed_from_double =
    (__int128) (negative_value - (double) b);
  double unsigned_to_double = (double) (unsigned_value + seed + c);
  double signed_to_double = (double) (signed_value - (__int128) (a + b));
  unsigned long mix =
    (unsigned long) unsigned_to_double ^ (unsigned long) (-signed_to_double);

  return (unsigned long) from_double +
    (unsigned long) (from_double >> 64) +
    (unsigned long) signed_from_double +
    (unsigned long) (signed_from_double >> 64) + mix + c;
}
