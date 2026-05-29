extern float floorf(float);
extern double floor(double);
extern float ceilf(float);
extern double ceil(double);

static unsigned long fp32_bits(float value)
{
  union {
    float f;
    unsigned int u;
  } bits = { value };
  return bits.u;
}

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
  volatile float posf = 2.75f;
  volatile float negf = -3.25f;
  volatile double pos = 2.25;
  volatile double neg = -3.25;
  float (*volatile fn_floorf)(float) = floorf;
  float (*volatile fn_ceilf)(float) = ceilf;
  double (*volatile fn_floor)(double) = floor;
  double (*volatile fn_ceil)(double) = ceil;

  if (fp32_bits(fn_floorf(posf)) != 0x40000000UL)
    return 1;
  if (fp32_bits(fn_ceilf(negf)) != 0xc0400000UL)
    return 2;
  if (fp64_bits(fn_floor(neg)) != 0xc010000000000000UL)
    return 3;
  if (fp64_bits(fn_ceil(pos)) != 0x4008000000000000UL)
    return 4;
  return 1496;
}
