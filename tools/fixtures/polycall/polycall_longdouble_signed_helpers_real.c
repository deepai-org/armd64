static volatile long seed = -17;
static volatile long double scale = -2.5L;

__attribute__((visibility("default")))
long poly_entry(long a, long b, int c)
{
  long double x = (long double) (a + seed) * scale;
  long double y = (long double) (b - 3) / (long double) (c - 5);
  long double z = x + y - (long double) c;
  return (long) z;
}
