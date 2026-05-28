struct pair_fp32 {
  float lo;
  float hi;
};

extern struct pair_fp32 poly_import_x86_fpair32(float, float, float);

float poly_entry(float a0, float a1, float a2)
{
#if defined(__aarch64__)
  register float keep0 asm("s8") = a0 + 1.0f;
  register float keep1 asm("s9") = a1 + 2.0f;
  register float keep2 asm("s10") = a2 + 3.0f;
  register float keep3 asm("s11") = a0 + a1 + 4.0f;
  register float keep4 asm("s12") = a1 + a2 + 5.0f;
  register float keep5 asm("s13") = a0 + a2 + 6.0f;
  register float keep6 asm("s14") = a0 + a1 + a2 + 7.0f;
  register float keep7 asm("s15") = a0 + 15.5f;
  float keep8 = a1 + 18.0f;
  float keep9 = a2 + 19.0f;
  float keep10 = a0 + a2 + 20.0f;
  float keep11 = a0 + a1 + a2 + 21.0f;
  asm volatile("" : "+w"(keep0), "+w"(keep1), "+w"(keep2),
      "+w"(keep3), "+w"(keep4), "+w"(keep5), "+w"(keep6),
      "+w"(keep7));
#elif defined(__riscv)
  register float keep0 asm("fs0") = a0 + 1.0f;
  register float keep1 asm("fs1") = a1 + 2.0f;
  register float keep2 asm("fs2") = a2 + 3.0f;
  register float keep3 asm("fs3") = a0 + a1 + 4.0f;
  register float keep4 asm("fs4") = a1 + a2 + 5.0f;
  register float keep5 asm("fs5") = a0 + a2 + 6.0f;
  register float keep6 asm("fs6") = a0 + a1 + a2 + 7.0f;
  register float keep7 asm("fs7") = a0 + 15.5f;
  register float keep8 asm("fs8") = a1 + 18.0f;
  register float keep9 asm("fs9") = a2 + 19.0f;
  register float keep10 asm("fs10") = a0 + a2 + 20.0f;
  register float keep11 asm("fs11") = a0 + a1 + a2 + 21.0f;
  asm volatile("" : "+f"(keep0), "+f"(keep1), "+f"(keep2),
      "+f"(keep3), "+f"(keep4), "+f"(keep5), "+f"(keep6),
      "+f"(keep7), "+f"(keep8), "+f"(keep9), "+f"(keep10),
      "+f"(keep11));
#else
  float keep0 = a0 + 1.0f;
  float keep1 = a1 + 2.0f;
  float keep2 = a2 + 3.0f;
  float keep3 = a0 + a1 + 4.0f;
  float keep4 = a1 + a2 + 5.0f;
  float keep5 = a0 + a2 + 6.0f;
  float keep6 = a0 + a1 + a2 + 7.0f;
  float keep7 = a0 + 15.5f;
  float keep8 = a1 + 18.0f;
  float keep9 = a2 + 19.0f;
  float keep10 = a0 + a2 + 20.0f;
  float keep11 = a0 + a1 + a2 + 21.0f;
#endif

  struct pair_fp32 result = poly_import_x86_fpair32(a0, a1, a2);

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

  float imported = result.lo + result.hi;
  return imported + keep0 + keep1 * 2.0f + keep2 * 3.0f +
    keep3 * 4.0f + keep4 * 5.0f + keep5 * 6.0f + keep6 * 7.0f +
    keep7 * 8.0f + keep8 * 9.0f + keep9 * 10.0f +
    keep10 * 11.0f + keep11 * 12.0f;
}
