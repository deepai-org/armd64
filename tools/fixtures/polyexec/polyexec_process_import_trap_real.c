#include <stdint.h>

enum {
  POLY_IMPORT_CALL_STRLEN = 0xffffffffffffe080ULL,
};

static char poly_import_text[] = "polyglot-libc";

static uint64_t poly_process_main(void) {
#if defined(__aarch64__)
  register uint64_t x0 __asm__("x0") = (uint64_t) (uintptr_t) poly_import_text;
  register uint64_t x16 __asm__("x16") = POLY_IMPORT_CALL_STRLEN;
  __asm__ volatile("blr x16" : "+r"(x0) : "r"(x16) : "x30", "memory");
  return x0;
#elif defined(__riscv)
  register uint64_t a0 __asm__("a0") = (uint64_t) (uintptr_t) poly_import_text;
  register uint64_t t0 __asm__("t0") = POLY_IMPORT_CALL_STRLEN;
  __asm__ volatile("jalr ra,0(t0)" : "+r"(a0) : "r"(t0) : "ra", "memory");
  return a0;
#else
#error unsupported architecture
#endif
}

void _start(void) {
#if defined(__aarch64__)
  register uint64_t x0 __asm__("x0") = poly_process_main();
  register uint64_t x8 __asm__("x8") = 93;
  __asm__ volatile("svc #0" : : "r"(x0), "r"(x8) : "memory");
#elif defined(__riscv)
  register uint64_t a0 __asm__("a0") = poly_process_main();
  register uint64_t a7 __asm__("a7") = 93;
  __asm__ volatile("ecall" : : "r"(a0), "r"(a7) : "memory");
#else
#error unsupported architecture
#endif
  __builtin_unreachable();
}
