extern float sqrtf(float);

static int near_fp32(float value, float expected)
{
  float delta = value - expected;
  if (delta < 0.0f)
    delta = -delta;
  return delta < 0.00001f;
}

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  volatile float big = 144.0f;
  volatile float frac = 0.25f;
  float (*volatile fn)(float) = sqrtf;

  if (!near_fp32(fn(big), 12.0f))
    return 1;
  if (!near_fp32(fn(frac), 0.5f))
    return 2;
  return 1420;
}
