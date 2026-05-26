struct hetero_f64_u64 {
  double d;
  unsigned long i;
};

__attribute__((visibility("default")))
struct hetero_f64_u64 poly_entry(struct hetero_f64_u64 in,
    unsigned long scale)
{
  struct hetero_f64_u64 out;
  out.d = in.d + (double) scale;
  out.i = in.i + scale;
  return out;
}
