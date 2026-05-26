static volatile long double qseed = 12345.75L;
static volatile int signed_seed = -12345;
static volatile unsigned int unsigned_seed = 12345u;
static volatile long signed_long_seed = -1234567890123L;
static volatile unsigned long unsigned_long_seed = 1234567890123UL;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  long double x = qseed + (long double) signed_seed +
    (long double) unsigned_seed + (long double) signed_long_seed +
    (long double) unsigned_long_seed + (long double) a;
  int signed32 = (int) x;
  unsigned int unsigned32 = (unsigned int) (x + (long double) b);
  long signed64 = (long) (x - (long double) c);
  unsigned long unsigned64 = (unsigned long) (x + 7.0L);

  return (unsigned long) signed32 + unsigned32 +
    (unsigned long) signed64 + unsigned64;
}
