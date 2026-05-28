extern double poly_import_x86_fp64_add(double, double);

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
  asm volatile("" : "+f"(keep0), "+f"(keep1), "+f"(keep2),
      "+f"(keep3), "+f"(keep4), "+f"(keep5), "+f"(keep6),
      "+f"(keep7));
#else
  double keep0 = a0 + 1.0;
  double keep1 = a1 + 2.0;
  double keep2 = a2 + 3.0;
  double keep3 = a0 + a1 + 4.0;
  double keep4 = a1 + a2 + 5.0;
  double keep5 = a0 + a2 + 6.0;
  double keep6 = a0 + a1 + a2 + 7.0;
  double keep7 = a0 + 15.5;
#endif

  double imported = poly_import_x86_fp64_add(a0, a1);

#if defined(__aarch64__)
  asm volatile("" : "+w"(keep0), "+w"(keep1), "+w"(keep2),
      "+w"(keep3), "+w"(keep4), "+w"(keep5), "+w"(keep6),
      "+w"(keep7));
#elif defined(__riscv)
  asm volatile("" : "+f"(keep0), "+f"(keep1), "+f"(keep2),
      "+f"(keep3), "+f"(keep4), "+f"(keep5), "+f"(keep6),
      "+f"(keep7));
#endif

  return imported + keep0 + keep1 * 2.0 + keep2 * 3.0 + keep3 * 4.0 +
    keep4 * 5.0 + keep5 * 6.0 + keep6 * 7.0 + keep7 * 8.0;
}
