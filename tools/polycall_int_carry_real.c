static volatile unsigned long alo = 0xfffffffffffffff0UL;
static volatile unsigned long ahi = 0x123456789abcdef0UL;
static volatile unsigned long blo = 0x30UL;
static volatile unsigned long bhi = 0x0102030405060708UL;
static volatile unsigned long slo = 0x20UL;
static volatile unsigned long shi = 0x8000000000000001UL;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned __int128 x =
    ((unsigned __int128) (ahi + a0) << 64) | (alo + a1);
  unsigned __int128 y =
    ((unsigned __int128) (bhi + a2) << 64) | (blo + a3);
  unsigned __int128 sum = x + y;
  unsigned __int128 sub = sum - ((((unsigned __int128) shi) << 64) | slo);

  return (unsigned long) sum ^ (unsigned long) (sum >> 64) ^
    (unsigned long) sub ^ (unsigned long) (sub >> 64);
}
