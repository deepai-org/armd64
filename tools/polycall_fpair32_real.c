struct pair_fp32 {
  float lo;
  float hi;
};

__attribute__((visibility("default")))
struct pair_fp32 poly_entry(float a, float b, float c)
{
  struct pair_fp32 result;
  result.lo = a + b;
  result.hi = b * c;
  return result;
}
