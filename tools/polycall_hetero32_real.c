struct hetero_u64_f32 {
  unsigned long i;
  float f;
};

__attribute__((visibility("default")))
struct hetero_u64_f32 poly_entry(struct hetero_u64_f32 in,
    unsigned long scale)
{
  struct hetero_u64_f32 out;
  out.i = in.i + scale;
  out.f = in.f + (float) scale;
  return out;
}
