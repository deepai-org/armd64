__attribute__((visibility("default")))
unsigned long poly_cross_needed_bias = 11;

static unsigned long poly_cross_needed_lifecycle;

extern unsigned long poly_cross_root_callback(unsigned long);
extern unsigned long poly_cross_root_bias;

__attribute__((constructor))
static void poly_cross_needed_ctor(void) {
  poly_cross_needed_lifecycle = 23;
}

__attribute__((destructor))
static void poly_cross_needed_dtor(void) {
  poly_cross_needed_lifecycle += 29;
}

__attribute__((visibility("default")))
unsigned long poly_cross_needed_add(unsigned long a, unsigned long b) {
  return a + b + 300 + poly_cross_root_callback(a) + poly_cross_root_bias +
    poly_cross_needed_lifecycle;
}

__attribute__((visibility("default")))
double poly_cross_needed_fp64(double a, double b) {
  return a + b + 6.0;
}

__attribute__((visibility("default")))
unsigned long poly_needed_fini_result(void) {
  return poly_cross_needed_lifecycle;
}
