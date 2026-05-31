__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_cross_fp32_mix fp32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

__attribute__((visibility("default")))
float poly_cross_fp32_mix(float left, float right, float scale)
{
  return (left + right) * scale;
}
