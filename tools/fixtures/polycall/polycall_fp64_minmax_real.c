__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  return __builtin_fmax(a0, a1) + __builtin_fmin(a1, a2);
}
