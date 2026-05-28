typedef unsigned int u32x4 __attribute__((vector_size(16)));

extern u32x4 poly_import_x86_vec128_u32(u32x4, u32x4);

u32x4 poly_entry(u32x4 a, u32x4 b)
{
  u32x4 result = poly_import_x86_vec128_u32(a, b);
  return result + b;
}
