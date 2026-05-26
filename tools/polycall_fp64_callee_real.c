extern double poly_import_fp64_add(double, double);

__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  double sum01 = a0 + a1;
  double sum12 = a1 + a2;
  double sum02 = a0 + a2;
  double imported = poly_import_fp64_add(sum01, sum12);

  return imported + sum01 + sum12 + sum02;
}
