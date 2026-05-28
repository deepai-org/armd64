__attribute__((visibility("default")))
unsigned long poly_cross_leaf_add(unsigned long a, unsigned long b) {
  return a + b + 700;
}

__attribute__((visibility("default")))
double poly_cross_leaf_fp64(double a, double b) {
  return a + b + 7.0;
}
