static volatile unsigned long ux = 0xfedcba9876543210UL;
static volatile unsigned long uy = 0x8000000000001234UL;
static volatile long sx = -0x123456789abcdeL;
static volatile long sy = 0x7000000000004321L;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long uleft = ux + a0;
  unsigned long uright = uy + a1;
  long sleft = sx - (long) a2;
  long sright = sy + (long) a3;
  unsigned long high_u =
    (unsigned long) (((unsigned __int128) uleft * uright) >> 64);
  unsigned long high_s =
    (unsigned long) (((__int128) sleft * sright) >> 64);

  return (high_u ^ high_s) + a0 + (a1 << 1);
}
