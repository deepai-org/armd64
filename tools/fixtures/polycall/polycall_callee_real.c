static volatile unsigned long helper_bias = 17;

__attribute__((noinline))
static unsigned long poly_helper(unsigned long value)
{
  helper_bias += 3;
  return value * 5 + helper_bias;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
#if defined(__aarch64__)
  register unsigned long keep0 asm("x19") = a0 + 10;
  register unsigned long keep1 asm("x20") = a1 + 20;
  register unsigned long keep2 asm("x21") = a2 + 30;
  register unsigned long keep3 asm("x22") = a3 + 40;
#elif defined(__riscv)
  register unsigned long keep0 asm("s1") = a0 + 10;
  register unsigned long keep1 asm("s2") = a1 + 20;
  register unsigned long keep2 asm("s3") = a2 + 30;
  register unsigned long keep3 asm("s4") = a3 + 40;
#else
  unsigned long keep0 = a0 + 10;
  unsigned long keep1 = a1 + 20;
  unsigned long keep2 = a2 + 30;
  unsigned long keep3 = a3 + 40;
#endif

  asm volatile("" : "+r"(keep0), "+r"(keep1), "+r"(keep2), "+r"(keep3));
  unsigned long helper = poly_helper(a4 + a8);
  asm volatile("" : "+r"(keep0), "+r"(keep1), "+r"(keep2), "+r"(keep3));

  (void) a5;
  (void) a6;
  (void) a7;
  return keep0 + keep1 * 2 + keep2 * 3 + keep3 * 4 + helper;
}
