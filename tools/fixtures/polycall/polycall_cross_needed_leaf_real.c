__attribute__((visibility("default")))
unsigned long poly_cross_leaf_bias = 13;

__attribute__((visibility("default")))
unsigned long poly_cross_leaf_add(unsigned long a, unsigned long b) {
  return a + b + 700;
}

__attribute__((visibility("default")))
unsigned long poly_cross_leaf_add9(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5,
    unsigned long a6, unsigned long a7, unsigned long a8) {
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + 900;
}

__attribute__((visibility("default")))
double poly_cross_leaf_fp64(double a, double b) {
  return a + b + 7.0;
}
