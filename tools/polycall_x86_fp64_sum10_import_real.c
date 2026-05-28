extern double poly_import_x86_fp64_sum10(double, double, double, double,
    double, double, double, double, double, double);

double poly_entry(double a0, double a1, double a2)
{
  return poly_import_x86_fp64_sum10(a0, a1, a2, 4.0, 5.0, 6.0, 7.0, 8.0,
    9.0, 10.0);
}
