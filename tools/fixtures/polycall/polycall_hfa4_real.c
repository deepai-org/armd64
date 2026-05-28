struct poly_hfa4_fp64 {
  double a;
  double b;
  double c;
  double d;
};

struct poly_hfa4_fp64 poly_entry(double a, double b, double c) {
  struct poly_hfa4_fp64 result;
  result.a = a + b;
  result.b = b + c;
  result.c = a + c + 1.0;
  result.d = a + b + c + 2.0;
  return result;
}
