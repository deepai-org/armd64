struct hetero_u32_f64 {
  unsigned int i;
  double d;
};

__attribute__((visibility("default")))
struct hetero_u32_f64 poly_entry(struct hetero_u32_f64 in,
    unsigned int scale)
{
  struct hetero_u32_f64 out;
  out.i = in.i + scale;
  out.d = in.d + (double) scale;
  return out;
}
