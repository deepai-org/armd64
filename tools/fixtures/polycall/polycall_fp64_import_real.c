extern double poly_import_fp64_add(double, double);

__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  (void) a2;
  return poly_import_fp64_add(a0, a1);
}
