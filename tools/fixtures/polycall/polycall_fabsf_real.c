extern float fabsf(float);

static unsigned int fp32_bits(float value)
{
  union {
    float f;
    unsigned int u;
  } bits = { value };
  return bits.u;
}

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  volatile float neg = -13.5f;
  volatile float pos = 0.25f;

  if (fp32_bits(fabsf(neg)) != 0x41580000U)
    return 1;
  if (fp32_bits(fabsf(pos)) != 0x3e800000U)
    return 2;
  return 1396;
}
