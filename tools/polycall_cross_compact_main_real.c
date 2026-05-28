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

extern struct poly_compact_u32_f32 poly_cross_compact_u32_f32(
    struct poly_compact_u32_f32, unsigned int);
extern struct poly_compact_f32_u32 poly_cross_compact_f32_u32(
    struct poly_compact_f32_u32, unsigned int);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const struct poly_compact_u32_f32 a = { 7, 2.5f };
  const struct poly_compact_f32_u32 b = { 1.25f, 6 };
  const struct poly_compact_u32_f32 ra =
    poly_cross_compact_u32_f32(a, 5);
  const struct poly_compact_f32_u32 rb =
    poly_cross_compact_f32_u32(b, 8);
  const union poly_float_bits fa = { .f = ra.f };
  const union poly_float_bits fb = { .f = rb.f };

  return ((unsigned long) ((fb.u >> 16) & 0xffffU) << 48) |
    ((unsigned long) (rb.i & 0xffffU) << 32) |
    ((unsigned long) ((fa.u >> 16) & 0xffffU) << 16) |
    (unsigned long) (ra.i & 0xffffU);
}
