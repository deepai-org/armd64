__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_root_ifunc_fp32_mix fp32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

extern unsigned long poly_cross_root_ifunc_fp32_call(void);

static float poly_root_ifunc_fp32_impl(float left, float right, float scale)
{
  return (left + right) * scale;
}

static void *poly_root_ifunc_fp32_resolver(void)
{
  return poly_root_ifunc_fp32_impl;
}

__attribute__((visibility("default")))
float poly_root_ifunc_fp32_mix(float, float, float)
    __attribute__((ifunc("poly_root_ifunc_fp32_resolver")));

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  return poly_cross_root_ifunc_fp32_call();
}
