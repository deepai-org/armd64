struct pair_fp64 {
  double lo;
  double hi;
};

extern struct pair_fp64 poly_import_x86_fpair64(double, double, double);

struct pair_fp64 poly_entry(double a0, double a1, double a2)
{
  struct pair_fp64 result = poly_import_x86_fpair64(a0, a1, a2);
  result.lo += a2;
  result.hi += a0;
  return result;
}
