static unsigned long poly_needed_bias = 100;
static unsigned long *volatile poly_needed_bias_ptr = &poly_needed_bias;

__attribute__((visibility("default")))
unsigned long poly_needed_add(unsigned long a, unsigned long b) {
  return a + b + *poly_needed_bias_ptr;
}
