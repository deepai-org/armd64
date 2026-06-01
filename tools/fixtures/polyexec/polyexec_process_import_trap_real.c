#include <stdint.h>

#include "../../include/polycpuid.h"

static char poly_import_text[] = "polyglot-libc";
static char poly_import_source[] = "trap-packets";
static char poly_import_needle[] = "pack";
static char poly_import_dest[16];
static char poly_import_scratch[16];
static char poly_import_copy_dest[16];
static char poly_import_ncopy_dest[16];
static char poly_import_overlap[] = "0123456789";
static char poly_import_overlap_expected[] = "0101234789";

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
  __asm__ volatile("blr x16" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x16) :
    : "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12",
      "x13", "x14", "x15", "x17", "x18", "x19", "x20", "x21", "x22",
      "x23", "x24", "x25", "x26", "x27", "x28", "x30", "cc", "memory");
  return x0;
#elif defined(__riscv)
  register uint64_t a0 __asm__("a0") = arg0;
  register uint64_t a1 __asm__("a1") = arg1;
  register uint64_t a2 __asm__("a2") = arg2;
  register uint64_t t0 __asm__("t0") = poly_import_target(import_id);
  __asm__ volatile("jalr ra,0(t0)" : "+r"(a0), "+r"(a1), "+r"(a2), "+r"(t0) :
    : "a3", "a4", "a5", "a6", "a7", "t1", "t2", "t3", "t4", "t5", "t6",
      "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
      "ra", "memory");
  return a0;
#else
#error unsupported architecture
#endif
}

static uint64_t poly_process_main(void) {
  uint64_t text = (uint64_t) (uintptr_t) poly_import_text;
  uint64_t source = (uint64_t) (uintptr_t) poly_import_source;
  uint64_t needle = (uint64_t) (uintptr_t) poly_import_needle;
  uint64_t dest = (uint64_t) (uintptr_t) poly_import_dest;
  uint64_t scratch = (uint64_t) (uintptr_t) poly_import_scratch;
  uint64_t copy_dest = (uint64_t) (uintptr_t) poly_import_copy_dest;
  uint64_t ncopy_dest = (uint64_t) (uintptr_t) poly_import_ncopy_dest;
  uint64_t overlap = (uint64_t) (uintptr_t) poly_import_overlap;
  uint64_t overlap_expected =
    (uint64_t) (uintptr_t) poly_import_overlap_expected;

  uint64_t len = poly_call_import3(POLY_IMPORT_FUNC_STRLEN, text, 0, 0);
  uint64_t memset_ret = poly_call_import3(POLY_IMPORT_FUNC_MEMSET, scratch,
    0x5a, sizeof(poly_import_scratch));
  uint64_t memcpy_ret = poly_call_import3(POLY_IMPORT_FUNC_MEMCPY, dest,
    source, sizeof("trap-packets"));
  uint64_t cmp = poly_call_import3(POLY_IMPORT_FUNC_MEMCMP, dest, source,
    sizeof("trap-packets") - 1);
  uint64_t strcmp_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRCMP, dest, source, 0);
  uint64_t strncmp_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRNCMP, dest, source, 4);
  uint64_t strnlen_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRNLEN, text, 4, 0);
  uint64_t memchr_result = poly_call_import3(POLY_IMPORT_FUNC_MEMCHR, source,
    'p', sizeof("trap-packets") - 1);
  uint64_t strchr_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRCHR, source, 'p', 0);
  uint64_t strrchr_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRRCHR, source, 't', 0);
  uint64_t strstr_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRSTR, source, needle, 0);
  uint64_t memmove_ret = poly_call_import3(POLY_IMPORT_FUNC_MEMMOVE,
    overlap + 2, overlap, 5);
  uint64_t overlap_cmp = poly_call_import3(POLY_IMPORT_FUNC_MEMCMP, overlap,
    overlap_expected, sizeof("0101234789") - 1);
  uint64_t strcpy_ret =
    poly_call_import3(POLY_IMPORT_FUNC_STRCPY, copy_dest, text, 0);
  uint64_t strcpy_cmp =
    poly_call_import3(POLY_IMPORT_FUNC_STRCMP, copy_dest, text, 0);
  uint64_t strncpy_ret =
    poly_call_import3(POLY_IMPORT_FUNC_STRNCPY, ncopy_dest, source, 4);
  uint64_t strncpy_cmp =
    poly_call_import3(POLY_IMPORT_FUNC_MEMCMP, ncopy_dest, source, 4);

  return len + (memset_ret == scratch ? 2 : 0) +
    (memcpy_ret == dest ? 3 : 0) + (cmp == 0 ? 5 : 0) +
    (strcmp_result == 0 ? 7 : 0) + (strncmp_result == 0 ? 11 : 0) +
    (strnlen_result == 4 ? 13 : 0) +
    (memchr_result == source + 3 ? 17 : 0) +
    (strchr_result == source + 3 ? 19 : 0) +
    (strrchr_result == source + 10 ? 23 : 0) +
    (strstr_result == source + 5 ? 29 : 0) +
    (memmove_ret == overlap + 2 ? 31 : 0) + (overlap_cmp == 0 ? 37 : 0) +
    (strcpy_ret == copy_dest && strcpy_cmp == 0 ? 4 : 0) +
    (strncpy_ret == ncopy_dest && strncpy_cmp == 0 ? 6 : 0);
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
