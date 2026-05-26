__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  return __builtin_sqrt(a0) + __builtin_sqrt(a1) + a2;
}
