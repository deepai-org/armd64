__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_cross_fp64_stack_sum fp64_stack\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

__attribute__((visibility("default")))
double poly_cross_fp64_stack_sum(double a0, double a1, double a2, double a3,
    double a4, double a5, double a6, double a7, double a8, double a9,
    double a10, double a11, double a12, double a13, double a14, double a15)
{
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 +
    a8 + a9 + a10 + a11 + a12 + a13 + a14 + a15 + 0.5;
}
