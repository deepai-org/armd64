extern double poly_import_x86_mixed_u64_fp64_stack(unsigned long long, double,
    unsigned long long, double, unsigned long long, double,
    unsigned long long, double, unsigned long long, double,
    unsigned long long, double, unsigned long long, double,
    unsigned long long, double, unsigned long long, double);

double poly_entry(unsigned long long a0, double f0, unsigned long long a1,
    double f1, unsigned long long a2, double f2, unsigned long long a3,
    double f3, unsigned long long a4, double f4, unsigned long long a5,
    double f5, unsigned long long a6, double f6, unsigned long long a7,
    double f7, unsigned long long a8, double f8)
{
  return poly_import_x86_mixed_u64_fp64_stack(a0, f0, a1, f1, a2, f2, a3,
    f3, a4, f4, a5, f5, a6, f6, a7, f7, a8, f8);
}
