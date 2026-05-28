struct hetero_u64_f64 {
  unsigned long i;
  double d;
};

__attribute__((visibility("default")))
struct hetero_u64_f64 poly_entry(struct hetero_u64_f64 in,
    unsigned long scale)
{
  struct hetero_u64_f64 out;
  out.i = in.i + scale;
  out.d = in.d + (double) scale;
  return out;
}
