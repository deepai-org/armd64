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
  "1: .ascii \"poly_root_ifunc_compact_u32_f32 compact_u32_f32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

extern unsigned long poly_cross_root_ifunc_compact_call(void);

static struct poly_compact_u32_f32 poly_root_ifunc_compact_impl(
    struct poly_compact_u32_f32 in, unsigned int scale)
{
  struct poly_compact_u32_f32 out;
  out.i = in.i + scale + 400;
  out.f = in.f + (float) scale + 100.0f;
  return out;
}

static void *poly_root_ifunc_compact_resolver(void)
{
  return poly_root_ifunc_compact_impl;
}

__attribute__((visibility("default")))
struct poly_compact_u32_f32 poly_root_ifunc_compact_u32_f32(
    struct poly_compact_u32_f32, unsigned int)
    __attribute__((ifunc("poly_root_ifunc_compact_resolver")));

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  return poly_cross_root_ifunc_compact_call();
}
