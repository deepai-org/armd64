extern double poly_import_x86_fp64_add(double, double);

double poly_entry(double a0, double a1, double a2)
{
  return poly_import_x86_fp64_add(a0, a1) + a2;
}
