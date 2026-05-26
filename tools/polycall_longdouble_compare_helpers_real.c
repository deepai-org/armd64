static volatile double dseed = 3.5;
static volatile float fseed = -1.25f;
static volatile long double qseed = 4.75L;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  long double x = (long double) dseed + (long double) fseed + qseed;
  long double y = (long double) (a + b) / (long double) (c + 1);
  double d = (double) (x + y);
  float f = (float) (x - y);
  unsigned long score = 0;

  if (x < y)
    score += 3;
  if (x <= y)
    score += 5;
  if (x > y)
    score += 7;
  if (x >= y)
    score += 11;
  if (x == y)
    score += 13;
  if (x != y)
    score += 17;

  return score + (unsigned long) (d * 10.0) +
    (unsigned long) (f * -4.0f);
}
