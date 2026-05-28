typedef unsigned int u32x4 __attribute__((vector_size(16)));

__attribute__((visibility("default")))
u32x4 poly_cross_needed_vec128_u32(u32x4 a, u32x4 b)
{
  const u32x4 bias = { 100, 200, 300, 400 };
  return a + b + bias;
}
