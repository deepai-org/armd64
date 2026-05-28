typedef unsigned int u32x4 __attribute__((vector_size(16)));

__attribute__((visibility("default")))
u32x4 poly_entry(u32x4 a, u32x4 b)
{
  return a + b;
}
