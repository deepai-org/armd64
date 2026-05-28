typedef unsigned int u32x4 __attribute__((vector_size(16)));

extern u32x4 poly_cross_needed_vec128_u32(u32x4, u32x4);

union poly_vec128_u32_bits {
  u32x4 v;
  unsigned int u[4];
};

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  union poly_vec128_u32_bits a = { .u = { 1, 2, 3, 4 } };
  union poly_vec128_u32_bits b = { .u = { 10, 20, 30, 40 } };
  union poly_vec128_u32_bits result;
  result.v = poly_cross_needed_vec128_u32(a.v, b.v);
  return ((unsigned long) (result.u[3] & 0xffffU) << 48) |
    ((unsigned long) (result.u[2] & 0xffffU) << 32) |
    ((unsigned long) (result.u[1] & 0xffffU) << 16) |
    (unsigned long) (result.u[0] & 0xffffU);
}
