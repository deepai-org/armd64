extern unsigned long poly_cross_needed_add(unsigned long, unsigned long);
extern double poly_cross_needed_fp64(double, double);
extern unsigned long poly_cross_needed_bias;

__attribute__((visibility("default")))
unsigned long poly_cross_root_bias = 17;

__attribute__((visibility("default")))
unsigned long poly_cross_root_callback(unsigned long value) {
  return value + 19;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1, unsigned long a2,
    unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6,
    unsigned long a7, unsigned long a8) {
  return poly_cross_needed_add(a0, a1) + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    (unsigned long) poly_cross_needed_fp64(2.5, 3.5) +
    poly_cross_needed_bias;
}
