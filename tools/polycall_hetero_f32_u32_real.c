struct hetero_f32_u32 {
  float f;
  unsigned int i;
};

__attribute__((visibility("default")))
struct hetero_f32_u32 poly_entry(struct hetero_f32_u32 in,
    unsigned int scale)
{
  struct hetero_f32_u32 out;
  out.f = in.f + (float) scale;
  out.i = in.i + scale;
  return out;
}
