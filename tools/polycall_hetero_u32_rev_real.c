struct hetero_f64_u32 {
  double d;
  unsigned int i;
};

__attribute__((visibility("default")))
struct hetero_f64_u32 poly_entry(struct hetero_f64_u32 in,
    unsigned int scale)
{
  struct hetero_f64_u32 out;
  out.d = in.d + (double) scale;
  out.i = in.i + scale;
  return out;
}
