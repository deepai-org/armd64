struct poly_compact_u32_f32 {
  unsigned int i;
  float f;
};

struct poly_compact_f32_u32 {
  float f;
  unsigned int i;
};

union poly_float_bits {
  float f;
  unsigned int u;
};

extern struct poly_compact_u32_f32 poly_root_compact_u32_f32(
    struct poly_compact_u32_f32, unsigned int);
extern struct poly_compact_f32_u32 poly_root_compact_f32_u32(
    struct poly_compact_f32_u32, unsigned int);

__attribute__((visibility("default")))
unsigned long poly_cross_root_compact_call(void)
{
  const struct poly_compact_u32_f32 a = { 11, 3.5f };
  const struct poly_compact_f32_u32 b = { 2.25f, 9 };
  const struct poly_compact_u32_f32 ra =
    poly_root_compact_u32_f32(a, 6);
  const struct poly_compact_f32_u32 rb =
    poly_root_compact_f32_u32(b, 4);
  const union poly_float_bits fa = { .f = ra.f };
  const union poly_float_bits fb = { .f = rb.f };

  return ((unsigned long) ((fb.u >> 16) & 0xffffU) << 48) |
    ((unsigned long) (rb.i & 0xffffU) << 32) |
    ((unsigned long) ((fa.u >> 16) & 0xffffU) << 16) |
    (unsigned long) (ra.i & 0xffffU);
}
