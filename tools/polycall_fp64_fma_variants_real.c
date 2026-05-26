__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  return __builtin_fma(a0, a1, -a2) +
    __builtin_fma(-a0, a1, a2) +
    __builtin_fma(-a0, a1, -a2);
}
