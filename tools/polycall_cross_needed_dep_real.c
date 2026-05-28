__attribute__((visibility("default")))
unsigned long poly_cross_needed_add(unsigned long a, unsigned long b) {
  return a + b + 300;
}

__attribute__((visibility("default")))
double poly_cross_needed_fp64(double a, double b) {
  return a + b + 6.0;
}
