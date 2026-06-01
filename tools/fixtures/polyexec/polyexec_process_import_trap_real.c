#include <stdint.h>

#include "../../include/polycpuid.h"

static char poly_import_text[] = "polyglot-libc";
static char poly_import_source[] = "trap-packets";
static char poly_import_dest[16];
static char poly_import_scratch[16];

static uint64_t poly_import_target(uint64_t import_id) {
  return POLY_IMPORT_CALL_BASE + (import_id * POLY_IMPORT_CALL_STRIDE);
}

static uint64_t poly_call_import3(uint64_t import_id, uint64_t arg0,
    uint64_t arg1, uint64_t arg2) {
#if defined(__aarch64__)
  register uint64_t x0 __asm__("x0") = arg0;
  register uint64_t x1 __asm__("x1") = arg1;
  register uint64_t x2 __asm__("x2") = arg2;
  register uint64_t x16 __asm__("x16") = poly_import_target(import_id);
  __asm__ volatile("blr x16" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x16) :
    "x30", "memory");
  return x0;
#elif defined(__riscv)
  register uint64_t a0 __asm__("a0") = arg0;
  register uint64_t a1 __asm__("a1") = arg1;
  register uint64_t a2 __asm__("a2") = arg2;
  register uint64_t t0 __asm__("t0") = poly_import_target(import_id);
  __asm__ volatile("jalr ra,0(t0)" : "+r"(a0) : "r"(a1), "r"(a2), "r"(t0) :
    "ra", "memory");
  return a0;
#else
#error unsupported architecture
#endif
}

static uint64_t poly_process_main(void) {
  uint64_t text = (uint64_t) (uintptr_t) poly_import_text;
  uint64_t source = (uint64_t) (uintptr_t) poly_import_source;
  uint64_t dest = (uint64_t) (uintptr_t) poly_import_dest;
  uint64_t scratch = (uint64_t) (uintptr_t) poly_import_scratch;

  uint64_t len = poly_call_import3(POLY_IMPORT_FUNC_STRLEN, text, 0, 0);
  uint64_t memset_ret = poly_call_import3(POLY_IMPORT_FUNC_MEMSET, scratch,
    0x5a, sizeof(poly_import_scratch));
  uint64_t memcpy_ret = poly_call_import3(POLY_IMPORT_FUNC_MEMCPY, dest,
    source, sizeof("trap-packets"));
  uint64_t cmp = poly_call_import3(POLY_IMPORT_FUNC_MEMCMP, dest, source,
    sizeof("trap-packets") - 1);

  return len + (memset_ret == scratch ? 5 : 0) +
    (memcpy_ret == dest ? 7 : 0) + (cmp == 0 ? 17 : 0);
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
