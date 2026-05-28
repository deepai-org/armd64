extern double poly_import_x86_mixed_u64_fp64(unsigned long long, double,
    unsigned long long, double, unsigned long long, double);

double poly_entry(unsigned long long a0, double a1, unsigned long long a2,
    double a3, unsigned long long a4, double a5)
{
#if defined(__aarch64__)
  register unsigned long long g0 asm("x19") = a0 + 10;
  register unsigned long long g1 asm("x20") = a2 + 20;
  register unsigned long long g2 asm("x21") = a4 + 30;
  register unsigned long long g3 asm("x22") = a0 + a2 + 40;
  register unsigned long long g4 asm("x23") = a2 + a4 + 50;
  register unsigned long long g5 asm("x24") = a0 + a4 + 60;
  register unsigned long long g6 asm("x25") = a0 + a2 + a4 + 70;
  register unsigned long long g7 asm("x26") = a0 * 3 + a4 + 80;
  register unsigned long long g8 asm("x27") = a2 * 4 + a4 + 90;
  register unsigned long long g9 asm("x28") = a0 + a2 * 2 + a4 * 3 + 100;
  register double f0 asm("d8") = a1 + 1.0;
  register double f1 asm("d9") = a3 + 2.0;
  register double f2 asm("d10") = a5 + 3.0;
  register double f3 asm("d11") = a1 + a3 + 4.0;
  register double f4 asm("d12") = a3 + a5 + 5.0;
  register double f5 asm("d13") = a1 + a5 + 6.0;
  register double f6 asm("d14") = a1 + a3 + a5 + 7.0;
  register double f7 asm("d15") = a1 * 2.0 + a5 + 8.0;
  asm volatile("" : "+r"(g0), "+r"(g1), "+r"(g2), "+r"(g3),
      "+r"(g4), "+r"(g5), "+r"(g6), "+r"(g7), "+r"(g8),
      "+r"(g9));
  asm volatile("" : "+w"(f0), "+w"(f1), "+w"(f2), "+w"(f3),
      "+w"(f4), "+w"(f5), "+w"(f6), "+w"(f7));
#elif defined(__riscv)
  register unsigned long long g0 asm("s0") = a0 + 10;
  register unsigned long long g1 asm("s1") = a2 + 20;
  register unsigned long long g2 asm("s2") = a4 + 30;
  register unsigned long long g3 asm("s3") = a0 + a2 + 40;
  register unsigned long long g4 asm("s4") = a2 + a4 + 50;
  register unsigned long long g5 asm("s5") = a0 + a4 + 60;
  register unsigned long long g6 asm("s6") = a0 + a2 + a4 + 70;
  register unsigned long long g7 asm("s7") = a0 * 3 + a4 + 80;
  register unsigned long long g8 asm("s8") = a2 * 4 + a4 + 90;
  register unsigned long long g9 asm("s9") = a0 + a2 * 2 + a4 * 3 + 100;
  register double f0 asm("fs0") = a1 + 1.0;
  register double f1 asm("fs1") = a3 + 2.0;
  register double f2 asm("fs2") = a5 + 3.0;
  register double f3 asm("fs3") = a1 + a3 + 4.0;
  register double f4 asm("fs4") = a3 + a5 + 5.0;
  register double f5 asm("fs5") = a1 + a5 + 6.0;
  register double f6 asm("fs6") = a1 + a3 + a5 + 7.0;
  register double f7 asm("fs7") = a1 * 2.0 + a5 + 8.0;
  asm volatile("" : "+r"(g0), "+r"(g1), "+r"(g2), "+r"(g3),
      "+r"(g4), "+r"(g5), "+r"(g6), "+r"(g7), "+r"(g8),
      "+r"(g9));
  asm volatile("" : "+f"(f0), "+f"(f1), "+f"(f2), "+f"(f3),
      "+f"(f4), "+f"(f5), "+f"(f6), "+f"(f7));
#else
  unsigned long long g0 = a0 + 10;
  unsigned long long g1 = a2 + 20;
  unsigned long long g2 = a4 + 30;
  unsigned long long g3 = a0 + a2 + 40;
  unsigned long long g4 = a2 + a4 + 50;
  unsigned long long g5 = a0 + a4 + 60;
  unsigned long long g6 = a0 + a2 + a4 + 70;
  unsigned long long g7 = a0 * 3 + a4 + 80;
  unsigned long long g8 = a2 * 4 + a4 + 90;
  unsigned long long g9 = a0 + a2 * 2 + a4 * 3 + 100;
  double f0 = a1 + 1.0;
  double f1 = a3 + 2.0;
  double f2 = a5 + 3.0;
  double f3 = a1 + a3 + 4.0;
  double f4 = a3 + a5 + 5.0;
  double f5 = a1 + a5 + 6.0;
  double f6 = a1 + a3 + a5 + 7.0;
  double f7 = a1 * 2.0 + a5 + 8.0;
#endif

  double imported = poly_import_x86_mixed_u64_fp64(a0, a1, a2, a3, a4, a5);

#if defined(__aarch64__)
  asm volatile("" : "+r"(g0), "+r"(g1), "+r"(g2), "+r"(g3),
      "+r"(g4), "+r"(g5), "+r"(g6), "+r"(g7), "+r"(g8),
      "+r"(g9));
  asm volatile("" : "+w"(f0), "+w"(f1), "+w"(f2), "+w"(f3),
      "+w"(f4), "+w"(f5), "+w"(f6), "+w"(f7));
#elif defined(__riscv)
  asm volatile("" : "+r"(g0), "+r"(g1), "+r"(g2), "+r"(g3),
      "+r"(g4), "+r"(g5), "+r"(g6), "+r"(g7), "+r"(g8),
      "+r"(g9));
  asm volatile("" : "+f"(f0), "+f"(f1), "+f"(f2), "+f"(f3),
      "+f"(f4), "+f"(f5), "+f"(f6), "+f"(f7));
#endif

  return imported + (double) g0 + (double) g1 * 2.0 + (double) g2 * 3.0 +
    (double) g3 * 4.0 + (double) g4 * 5.0 + (double) g5 * 6.0 +
    (double) g6 * 7.0 + (double) g7 * 8.0 + (double) g8 * 9.0 +
    (double) g9 * 10.0 + f0 + f1 * 2.0 + f2 * 3.0 + f3 * 4.0 +
    f4 * 5.0 + f5 * 6.0 + f6 * 7.0 + f7 * 8.0;
}
