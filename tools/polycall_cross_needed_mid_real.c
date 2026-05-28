extern unsigned long poly_cross_leaf_add(unsigned long, unsigned long);
extern double poly_cross_leaf_fp64(double, double);

__attribute__((visibility("default")))
unsigned long poly_cross_mid_sum(unsigned long a, unsigned long b) {
  return poly_cross_leaf_add(a, b) + 50 +
    (unsigned long) poly_cross_leaf_fp64(4.5, 5.5);
}
