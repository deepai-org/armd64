static volatile double dseed = 3.5;
static volatile float fseed = -1.25f;
static volatile long double qseed = 4.75L;
static volatile long double zseed = 0.0L;

extern int __eqtf2(long double left, long double right);
extern int __getf2(long double left, long double right);
extern int __netf2(long double left, long double right);
extern int __unordtf2(long double left, long double right);

static unsigned long descriptor_compare_probe(long double x, long double y)
{
  long double nan = zseed / zseed;
  unsigned long failures = 0;

  failures += __eqtf2(x, x) != 0;
  failures += __getf2(x, y) <= 0;
  failures += __netf2(x, y) == 0;
  failures += __unordtf2(nan, x) == 0;

  return failures;
}

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
    (unsigned long) (f * -4.0f) + descriptor_compare_probe(x, y);
}
