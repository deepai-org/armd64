typedef unsigned int u32x4 __attribute__((vector_size(16)));

extern u32x4 poly_import_x86_vec128_u32(u32x4, u32x4);

u32x4 poly_entry(u32x4 a, u32x4 b)
{
#if defined(__aarch64__)
  register double keep0 asm("d8") = 1.0;
  register double keep1 asm("d9") = 2.0;
  register double keep2 asm("d10") = 3.0;
  register double keep3 asm("d11") = 4.0;
  register double keep4 asm("d12") = 5.0;
  register double keep5 asm("d13") = 6.0;
  register double keep6 asm("d14") = 7.0;
  register double keep7 asm("d15") = 8.0;
  asm volatile("" : "+w"(keep0), "+w"(keep1), "+w"(keep2),
      "+w"(keep3), "+w"(keep4), "+w"(keep5), "+w"(keep6),
      "+w"(keep7));
#elif defined(__riscv)
  register double keep0 asm("fs0") = 1.0;
  register double keep1 asm("fs1") = 2.0;
  register double keep2 asm("fs2") = 3.0;
  register double keep3 asm("fs3") = 4.0;
  register double keep4 asm("fs4") = 5.0;
  register double keep5 asm("fs5") = 6.0;
  register double keep6 asm("fs6") = 7.0;
  register double keep7 asm("fs7") = 8.0;
  asm volatile("" : "+f"(keep0), "+f"(keep1), "+f"(keep2),
      "+f"(keep3), "+f"(keep4), "+f"(keep5), "+f"(keep6),
      "+f"(keep7));
#else
  double keep0 = 1.0;
  double keep1 = 2.0;
  double keep2 = 3.0;
  double keep3 = 4.0;
  double keep4 = 5.0;
  double keep5 = 6.0;
  double keep6 = 7.0;
  double keep7 = 8.0;
#endif

  u32x4 result = poly_import_x86_vec128_u32(a, b);

#if defined(__aarch64__)
  asm volatile("" : "+w"(keep0), "+w"(keep1), "+w"(keep2),
      "+w"(keep3), "+w"(keep4), "+w"(keep5), "+w"(keep6),
      "+w"(keep7));
#elif defined(__riscv)
  asm volatile("" : "+f"(keep0), "+f"(keep1), "+f"(keep2),
      "+f"(keep3), "+f"(keep4), "+f"(keep5), "+f"(keep6),
      "+f"(keep7));
#endif

  unsigned int bias = (unsigned int) (keep0 + keep1 + keep2 + keep3 +
    keep4 + keep5 + keep6 + keep7);
  u32x4 extra = { bias, bias, bias, bias };
  return result + b + extra;
}
