__attribute__((visibility("default")))
unsigned long poly_cross_needed_bias = 11;

struct poly_compact_u32_f32 {
  unsigned int i;
  float f;
};

__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_needed_fini_result compact_u32_f32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

static unsigned long poly_cross_needed_lifecycle;
__thread unsigned long poly_cross_needed_tls_counter = 31;

extern unsigned long poly_cross_root_callback(unsigned long);
extern unsigned long poly_cross_root_callback9(unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long);
extern unsigned long poly_cross_root_bias;

__attribute__((constructor))
static void poly_cross_needed_ctor(void) {
  poly_cross_needed_lifecycle = 23;
}

__attribute__((destructor))
static void poly_cross_needed_dtor(void) {
  poly_cross_needed_lifecycle += 29 + poly_cross_needed_tls_counter;
}

static unsigned long poly_cross_needed_add_impl(unsigned long a,
    unsigned long b) {
  poly_cross_needed_tls_counter += a + b;
  return a + b + 300 + poly_cross_root_callback(a) + poly_cross_root_bias +
    poly_cross_root_callback9(a, b, 3, 4, 5, 6, 7, 8, 9) +
    poly_cross_needed_lifecycle + poly_cross_needed_tls_counter;
}

static void *poly_cross_needed_add_resolver(void) {
  return poly_cross_needed_add_impl;
}

unsigned long poly_cross_needed_add(unsigned long, unsigned long)
  __attribute__((ifunc("poly_cross_needed_add_resolver")));

__attribute__((visibility("default")))
double poly_cross_needed_fp64(double a, double b) {
  return a + b + 6.0;
}

static struct poly_compact_u32_f32 poly_needed_fini_result_impl(void) {
  struct poly_compact_u32_f32 result;
  result.i = (unsigned int) poly_cross_needed_lifecycle;
  result.f = 13.5f;
  return result;
}

static void *poly_needed_fini_result_resolver(void) {
  return poly_needed_fini_result_impl;
}

__attribute__((visibility("default")))
struct poly_compact_u32_f32 poly_needed_fini_result(void)
  __attribute__((ifunc("poly_needed_fini_result_resolver")));
