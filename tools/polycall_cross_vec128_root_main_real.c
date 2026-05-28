typedef unsigned int u32x4 __attribute__((vector_size(16)));

union vec128_u32 {
  u32x4 v;
  unsigned int u[4];
};

extern u32x4 poly_cross_root_vec128_call(u32x4, u32x4);

__attribute__((visibility("default")))
u32x4 poly_root_vec128_u32(u32x4 a, u32x4 b)
{
  const u32x4 bias = { 100, 200, 300, 400 };
  return a + b + bias;
}

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  union vec128_u32 a = { .u = { 10, 20, 30, 40 } };
  union vec128_u32 b = { .u = { 1, 2, 3, 4 } };
  union vec128_u32 result;
  result.v = poly_cross_root_vec128_call(a.v, b.v);
  return ((unsigned long) (result.u[0] & 0xffff)) |
    ((unsigned long) (result.u[1] & 0xffff) << 16) |
    ((unsigned long) (result.u[2] & 0xffff) << 32) |
    ((unsigned long) (result.u[3] & 0xffff) << 48);
}
