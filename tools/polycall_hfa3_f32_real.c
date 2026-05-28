struct poly_hfa3_fp32 {
  float a;
  float b;
  float c;
};

struct poly_hfa3_fp32 poly_entry(float a, float b, float c) {
  struct poly_hfa3_fp32 result;
  result.a = a + b;
  result.b = b + c;
  result.c = a + c + 1.0f;
  return result;
}
