static volatile unsigned long seed = 31;
static volatile long double scale = 1.25L;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  long double x = (long double) (a + seed) * scale;
  long double y = (long double) (b + 3) / (long double) (c + 5);
  long double z = x + y - (long double) c;
  return (unsigned long) z;
}
