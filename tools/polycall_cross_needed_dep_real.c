__attribute__((visibility("default")))
unsigned long poly_cross_needed_bias = 11;

extern unsigned long poly_cross_root_callback(unsigned long);

__attribute__((visibility("default")))
unsigned long poly_cross_needed_add(unsigned long a, unsigned long b) {
  return a + b + 300 + poly_cross_root_callback(a);
}

__attribute__((visibility("default")))
double poly_cross_needed_fp64(double a, double b) {
  return a + b + 6.0;
}
