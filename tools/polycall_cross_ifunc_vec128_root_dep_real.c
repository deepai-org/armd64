typedef unsigned int u32x4 __attribute__((vector_size(16)));

__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_cross_root_ifunc_vec128_call vec128_u32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

extern u32x4 poly_root_ifunc_vec128_u32(u32x4, u32x4);

__attribute__((visibility("default")))
u32x4 poly_cross_root_ifunc_vec128_call(u32x4 a, u32x4 b)
{
  return poly_root_ifunc_vec128_u32(a, b);
}
