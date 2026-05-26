extern unsigned long poly_needed_leaf(unsigned long, unsigned long);

static unsigned long poly_needed_bias = 99;
static unsigned long *volatile poly_needed_bias_ptr = &poly_needed_bias;

__attribute__((constructor))
static void poly_needed_ctor(void) {
  poly_needed_bias = poly_needed_leaf(40, 50);
}

__attribute__((visibility("default")))
unsigned long poly_needed_add(unsigned long a, unsigned long b) {
  return poly_needed_leaf(a, b) + *poly_needed_bias_ptr;
}
