struct hetero_f32_u64 {
  float f;
  unsigned long i;
};

__attribute__((visibility("default")))
struct hetero_f32_u64 poly_entry(struct hetero_f32_u64 in,
    unsigned long scale)
{
  struct hetero_f32_u64 out;
  out.f = in.f + (float) scale;
  out.i = in.i + scale;
  return out;
}
