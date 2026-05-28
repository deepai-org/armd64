struct pair_fp32 {
  float lo;
  float hi;
};

extern struct pair_fp32 poly_import_x86_fpair32(float, float, float);

struct pair_fp32 poly_entry(float a0, float a1, float a2)
{
  struct pair_fp32 result = poly_import_x86_fpair32(a0, a1, a2);
  result.lo += a2;
  result.hi += a0;
  return result;
}
