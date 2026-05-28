struct pair_fp64 {
  double lo;
  double hi;
};

__attribute__((visibility("default")))
struct pair_fp64 poly_entry(double a0, double a1, double a2)
{
  struct pair_fp64 result;
  result.lo = (a0 + a1) * a2;
  result.hi = (a2 - a0) * a1;
  return result;
}
