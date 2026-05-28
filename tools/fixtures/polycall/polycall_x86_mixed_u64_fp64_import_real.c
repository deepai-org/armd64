extern double poly_import_x86_mixed_u64_fp64(unsigned long long, double,
    unsigned long long, double, unsigned long long, double);

double poly_entry(unsigned long long a0, double a1, unsigned long long a2,
    double a3, unsigned long long a4, double a5)
{
  return poly_import_x86_mixed_u64_fp64(a0, a1, a2, a3, a4, a5);
}
