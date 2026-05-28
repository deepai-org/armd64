extern unsigned long poly_cross_leaf_add(unsigned long, unsigned long);
extern unsigned long poly_cross_leaf_add9(unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long);
extern double poly_cross_leaf_fp64(double, double);
extern unsigned long poly_cross_leaf_bias;

__attribute__((visibility("default")))
unsigned long poly_cross_mid_sum(unsigned long a, unsigned long b) {
  return poly_cross_leaf_add(a, b) +
    poly_cross_leaf_add9(a, b, 3, 4, 5, 6, 7, 8, 9) + 50 +
    (unsigned long) poly_cross_leaf_fp64(4.5, 5.5) +
    poly_cross_leaf_bias;
}
