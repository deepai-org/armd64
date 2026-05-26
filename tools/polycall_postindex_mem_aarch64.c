static unsigned long values[6] = { 3, 5, 7, 11, 0, 0 };

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b,
    unsigned long c, unsigned long d)
{
  unsigned long *p = values;
  unsigned long sum, out0, out1, tmp;

  asm volatile(
    "mov %[sum], xzr\n\t"
    "ldr %[tmp], [%[p]], #8\n\t"
    "add %[sum], %[sum], %[tmp]\n\t"
    "ldr %[tmp], [%[p]], #8\n\t"
    "add %[sum], %[sum], %[tmp]\n\t"
    "ldr %[tmp], [%[p]], #8\n\t"
    "add %[sum], %[sum], %[tmp]\n\t"
    "ldr %[tmp], [%[p]], #8\n\t"
    "add %[sum], %[sum], %[tmp]\n\t"
    "add %[out0], %[a], %[sum]\n\t"
    "str %[out0], [%[p]], #8\n\t"
    "add %[out1], %[b], %[c]\n\t"
    "add %[out1], %[out1], %[d]\n\t"
    "str %[out1], [%[p]], #8\n\t"
    : [p] "+r"(p), [sum] "=&r"(sum), [out0] "=&r"(out0),
      [out1] "=&r"(out1), [tmp] "=&r"(tmp)
    : [a] "r"(a), [b] "r"(b), [c] "r"(c), [d] "r"(d)
    : "memory", "cc");

  return sum + values[4] + values[5] + (unsigned long) (p - values);
}
