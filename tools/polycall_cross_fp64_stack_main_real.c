union poly_fp64_bits {
  double f;
  unsigned long u;
};

extern double poly_cross_fp64_stack_sum(double, double, double, double,
    double, double, double, double, double, double, double, double,
    double, double, double, double);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const union poly_fp64_bits result = {
    .f = poly_cross_fp64_stack_sum(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
      9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0)
  };
  return result.u;
}
