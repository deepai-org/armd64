extern double sqrt(double);

static int near_fp64(double value, double expected)
{
  double delta = value - expected;
  if (delta < 0.0)
    delta = -delta;
  return delta < 0.000000001;
}

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  volatile double big = 144.0;
  volatile double frac = 0.25;
  double (*volatile fn)(double) = sqrt;

  if (!near_fp64(fn(big), 12.0))
    return 1;
  if (!near_fp64(fn(frac), 0.5))
    return 2;
  return 1412;
}
