union poly_fp32_bits {
  float f;
  unsigned int u;
};

extern float poly_root_ifunc_fp32_mix(float, float, float);

__attribute__((visibility("default")))
unsigned long poly_cross_root_ifunc_fp32_call(void)
{
  const union poly_fp32_bits result = {
    .f = poly_root_ifunc_fp32_mix(1.5f, 2.25f, 3.0f)
  };
  return result.u;
}
