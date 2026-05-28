static unsigned long poly_state;

#if defined(__riscv)
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
  "1: .ascii \"poly_fini_result compact_u32_f32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");
#endif

__attribute__((constructor))
static void poly_ctor(void)
{
  poly_state = 100;
}

__attribute__((destructor))
static void poly_fini(void)
{
  poly_state += 1000;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  poly_state += a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
  return poly_state;
}

#if defined(__riscv)
static struct poly_compact_u32_f32 poly_fini_result_impl(void)
{
  struct poly_compact_u32_f32 result;
  result.i = (unsigned int) poly_state;
  result.f = 13.5f;
  return result;
}

static void *poly_fini_result_resolver(void)
{
  return poly_fini_result_impl;
}

__attribute__((visibility("default")))
struct poly_compact_u32_f32 poly_fini_result(void)
  __attribute__((ifunc("poly_fini_result_resolver")));
#else
static unsigned long poly_fini_result_impl(void)
{
  return poly_state;
}

static void *poly_fini_result_resolver(void)
{
  return poly_fini_result_impl;
}

__attribute__((visibility("default")))
unsigned long poly_fini_result(void)
  __attribute__((ifunc("poly_fini_result_resolver")));
#endif
