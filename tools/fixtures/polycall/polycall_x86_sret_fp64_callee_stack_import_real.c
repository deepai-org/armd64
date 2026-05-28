struct poly_x86_sret_u64 {
  unsigned long long a;
  unsigned long long b;
  unsigned long long c;
  unsigned long long d;
};

extern struct poly_x86_sret_u64 poly_import_x86_sret_u64_stack10(
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long);

double poly_entry(double a0, double a1, double a2)
{
#if defined(__aarch64__)
  register double keep0 asm("d8") = a0 + 1.0;
  register double keep1 asm("d9") = a1 + 2.0;
  register double keep2 asm("d10") = a2 + 3.0;
  register double keep3 asm("d11") = a0 + a1 + 4.0;
  register double keep4 asm("d12") = a1 + a2 + 5.0;
  register double keep5 asm("d13") = a0 + a2 + 6.0;
  register double keep6 asm("d14") = a0 + a1 + a2 + 7.0;
  register double keep7 asm("d15") = a0 + 15.5;
  double keep8 = a1 + 18.0;
  double keep9 = a2 + 19.0;
  double keep10 = a0 + a2 + 20.0;
  double keep11 = a0 + a1 + a2 + 21.0;
  asm volatile("" : "+w"(keep0), "+w"(keep1), "+w"(keep2),
      "+w"(keep3), "+w"(keep4), "+w"(keep5), "+w"(keep6),
      "+w"(keep7));
#elif defined(__riscv)
  register double keep0 asm("fs0") = a0 + 1.0;
  register double keep1 asm("fs1") = a1 + 2.0;
  register double keep2 asm("fs2") = a2 + 3.0;
  register double keep3 asm("fs3") = a0 + a1 + 4.0;
  register double keep4 asm("fs4") = a1 + a2 + 5.0;
  register double keep5 asm("fs5") = a0 + a2 + 6.0;
  register double keep6 asm("fs6") = a0 + a1 + a2 + 7.0;
  register double keep7 asm("fs7") = a0 + 15.5;
  register double keep8 asm("fs8") = a1 + 18.0;
  register double keep9 asm("fs9") = a2 + 19.0;
  register double keep10 asm("fs10") = a0 + a2 + 20.0;
  register double keep11 asm("fs11") = a0 + a1 + a2 + 21.0;
  asm volatile("" : "+f"(keep0), "+f"(keep1), "+f"(keep2),
      "+f"(keep3), "+f"(keep4), "+f"(keep5), "+f"(keep6),
      "+f"(keep7), "+f"(keep8), "+f"(keep9), "+f"(keep10),
      "+f"(keep11));
#else
  double keep0 = a0 + 1.0;
  double keep1 = a1 + 2.0;
  double keep2 = a2 + 3.0;
  double keep3 = a0 + a1 + 4.0;
  double keep4 = a1 + a2 + 5.0;
  double keep5 = a0 + a2 + 6.0;
  double keep6 = a0 + a1 + a2 + 7.0;
  double keep7 = a0 + 15.5;
  double keep8 = a1 + 18.0;
  double keep9 = a2 + 19.0;
  double keep10 = a0 + a2 + 20.0;
  double keep11 = a0 + a1 + a2 + 21.0;
#endif

  struct poly_x86_sret_u64 result =
    poly_import_x86_sret_u64_stack10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

#if defined(__aarch64__)
  asm volatile("" : "+w"(keep0), "+w"(keep1), "+w"(keep2),
      "+w"(keep3), "+w"(keep4), "+w"(keep5), "+w"(keep6),
      "+w"(keep7));
#elif defined(__riscv)
  asm volatile("" : "+f"(keep0), "+f"(keep1), "+f"(keep2),
      "+f"(keep3), "+f"(keep4), "+f"(keep5), "+f"(keep6),
      "+f"(keep7), "+f"(keep8), "+f"(keep9), "+f"(keep10),
      "+f"(keep11));
#endif

  double imported = (double) (result.a + result.b + result.c + result.d);
  return imported + keep0 + keep1 * 2.0 + keep2 * 3.0 + keep3 * 4.0 +
    keep4 * 5.0 + keep5 * 6.0 + keep6 * 7.0 + keep7 * 8.0 +
    keep8 * 9.0 + keep9 * 10.0 + keep10 * 11.0 + keep11 * 12.0;
}
