union poly_fp64_bits {
  double f;
  unsigned long u;
};

extern double poly_cross_fp64_mix(double, double);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const union poly_fp64_bits result = {
    .f = poly_cross_fp64_mix(1.5, 2.25)
  };
  return result.u;
}
