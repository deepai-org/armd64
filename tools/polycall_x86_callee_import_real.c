extern unsigned long long poly_import_x86_sum8(unsigned long long,
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long);

unsigned long long poly_entry(unsigned long long a0, unsigned long long a1,
    unsigned long long a2, unsigned long long a3, unsigned long long a4,
    unsigned long long a5, unsigned long long a6, unsigned long long a7,
    unsigned long long a8)
{
#if defined(__aarch64__)
  register unsigned long long keep0 asm("x19") = a0 + 10;
  register unsigned long long keep1 asm("x20") = a1 + 20;
  register unsigned long long keep2 asm("x21") = a2 + 30;
  register unsigned long long keep3 asm("x22") = a3 + 40;
  register unsigned long long keep4 asm("x23") = a4 + 50;
  register unsigned long long keep5 asm("x24") = a5 + 60;
  register unsigned long long keep6 asm("x25") = a6 + 70;
  register unsigned long long keep7 asm("x26") = a7 + 80;
  register unsigned long long keep8 asm("x27") = a8 + 90;
  register unsigned long long keep9 asm("x28") = a0 + a8 + 100;
#elif defined(__riscv)
  register unsigned long long keep0 asm("s1") = a0 + 10;
  register unsigned long long keep1 asm("s2") = a1 + 20;
  register unsigned long long keep2 asm("s3") = a2 + 30;
  register unsigned long long keep3 asm("s4") = a3 + 40;
  register unsigned long long keep4 asm("s5") = a4 + 50;
  register unsigned long long keep5 asm("s6") = a5 + 60;
  register unsigned long long keep6 asm("s7") = a6 + 70;
  register unsigned long long keep7 asm("s8") = a7 + 80;
  register unsigned long long keep8 asm("s9") = a8 + 90;
  register unsigned long long keep9 asm("s10") = a0 + a8 + 100;
#else
  unsigned long long keep0 = a0 + 10;
  unsigned long long keep1 = a1 + 20;
  unsigned long long keep2 = a2 + 30;
  unsigned long long keep3 = a3 + 40;
  unsigned long long keep4 = a4 + 50;
  unsigned long long keep5 = a5 + 60;
  unsigned long long keep6 = a6 + 70;
  unsigned long long keep7 = a7 + 80;
  unsigned long long keep8 = a8 + 90;
  unsigned long long keep9 = a0 + a8 + 100;
#endif

  asm volatile("" : "+r"(keep0), "+r"(keep1), "+r"(keep2),
      "+r"(keep3), "+r"(keep4), "+r"(keep5), "+r"(keep6),
      "+r"(keep7), "+r"(keep8), "+r"(keep9));
  unsigned long long imported =
    poly_import_x86_sum8(a0, a1, a2, a3, a4, a5, a6, a7);
  asm volatile("" : "+r"(keep0), "+r"(keep1), "+r"(keep2),
      "+r"(keep3), "+r"(keep4), "+r"(keep5), "+r"(keep6),
      "+r"(keep7), "+r"(keep8), "+r"(keep9));

  return imported + keep0 + keep1 * 2 + keep2 * 3 + keep3 * 4 +
    keep4 * 5 + keep5 * 6 + keep6 * 7 + keep7 * 8 + keep8 * 9 +
    keep9 * 10;
}
