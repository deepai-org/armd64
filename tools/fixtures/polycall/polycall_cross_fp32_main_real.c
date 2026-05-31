union poly_fp32_bits {
  float f;
  unsigned int u;
};

extern float poly_cross_fp32_mix(float, float, float);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const union poly_fp32_bits result = {
    .f = poly_cross_fp32_mix(1.5f, 2.25f, 3.0f)
  };
  return result.u;
}
