typedef unsigned int u32x4 __attribute__((vector_size(16)));

extern u32x4 poly_root_vec128_u32(u32x4, u32x4);

__attribute__((visibility("default")))
u32x4 poly_cross_root_vec128_call(u32x4 a, u32x4 b)
{
  return poly_root_vec128_u32(a, b);
}
