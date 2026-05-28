struct poly_compact_u32_f32 {
  unsigned int i;
  float f;
};

union poly_float_bits {
  float f;
  unsigned int u;
};

extern struct poly_compact_u32_f32 poly_cross_ifunc_compact_u32_f32(
    struct poly_compact_u32_f32, unsigned int);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const struct poly_compact_u32_f32 arg = { 7, 2.5f };
  const struct poly_compact_u32_f32 result =
    poly_cross_ifunc_compact_u32_f32(arg, 5);
  const union poly_float_bits bits = { .f = result.f };

  return ((unsigned long) bits.u << 32) | result.i;
}
