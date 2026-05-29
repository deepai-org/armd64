extern double fabs(double);

static unsigned long fp64_bits(double value)
{
  union {
    double d;
    unsigned long u;
  } bits = { value };
  return bits.u;
}

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  volatile double neg = -42.25;
  volatile double pos = 0.125;

  if (fp64_bits(fabs(neg)) != 0x4045200000000000UL)
    return 1;
  if (fp64_bits(fabs(pos)) != 0x3fc0000000000000UL)
    return 2;
  return 1404;
}
