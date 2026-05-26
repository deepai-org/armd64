static unsigned long poly_needed_leaf_bias;

__attribute__((visibility("default")))
unsigned long poly_needed_leaf_bonus = 3;

__attribute__((constructor))
static void poly_needed_leaf_ctor(void) {
  poly_needed_leaf_bias = 10;
}

__attribute__((destructor))
static void poly_needed_leaf_dtor(void) {
  poly_needed_leaf_bias = 20;
}

__attribute__((visibility("default")))
unsigned long poly_needed_leaf(unsigned long a, unsigned long b) {
  return a + b + poly_needed_leaf_bias;
}
