struct pair_fp64 {
  double lo;
  double hi;
};

__attribute__((visibility("default")))
double poly_entry(struct pair_fp64 pair, double scale)
{
  return (pair.lo + pair.hi) * scale;
}
