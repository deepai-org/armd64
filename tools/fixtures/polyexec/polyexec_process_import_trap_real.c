#include <stdint.h>

#include "../../include/polycpuid.h"

static char poly_import_text[] = "polyglot-libc";
static char poly_import_source[] = "trap-packets";
static char poly_import_needle[] = "pack";
static char poly_import_signed_decimal[] = " -42";
static char poly_import_long_decimal[] = "123";
static char poly_import_llong_decimal[] = "-99";
static char poly_import_hex[] = "7b";
static char poly_import_octal[] = "77";
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
  uint64_t signed_decimal =
    (uint64_t) (uintptr_t) poly_import_signed_decimal;
  uint64_t long_decimal = (uint64_t) (uintptr_t) poly_import_long_decimal;
  uint64_t llong_decimal = (uint64_t) (uintptr_t) poly_import_llong_decimal;
  uint64_t hex = (uint64_t) (uintptr_t) poly_import_hex;
  uint64_t octal = (uint64_t) (uintptr_t) poly_import_octal;
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
  uint64_t atoi_result =
    poly_call_import3(POLY_IMPORT_FUNC_ATOI, signed_decimal, 0, 0);
  uint64_t atol_result =
    poly_call_import3(POLY_IMPORT_FUNC_ATOL, long_decimal, 0, 0);
  uint64_t atoll_result =
    poly_call_import3(POLY_IMPORT_FUNC_ATOLL, llong_decimal, 0, 0);
  uint64_t strtol_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRTOL, hex, 0, 16);
  uint64_t strtoul_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRTOUL, octal, 0, 8);
  uint64_t strtoll_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRTOLL, signed_decimal, 0, 10);
  uint64_t strtoull_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRTOULL, hex, 0, 16);
  uint64_t isalnum_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISALNUM, '9', 0, 0);
  uint64_t isalpha_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISALPHA, 'A', 0, 0);
  uint64_t isdigit_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISDIGIT, '7', 0, 0);
  uint64_t islower_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISLOWER, 'q', 0, 0);
  uint64_t isspace_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISSPACE, ' ', 0, 0);
  uint64_t isupper_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISUPPER, 'Q', 0, 0);
  uint64_t isxdigit_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISXDIGIT, 'f', 0, 0);
  uint64_t isblank_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISBLANK, '\t', 0, 0);
  uint64_t iscntrl_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISCNTRL, '\n', 0, 0);
  uint64_t isgraph_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISGRAPH, '!', 0, 0);
  uint64_t isprint_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISPRINT, ' ', 0, 0);
  uint64_t ispunct_result =
    poly_call_import3(POLY_IMPORT_FUNC_ISPUNCT, '.', 0, 0);
  uint64_t tolower_result =
    poly_call_import3(POLY_IMPORT_FUNC_TOLOWER, 'Q', 0, 0);
  uint64_t toupper_result =
    poly_call_import3(POLY_IMPORT_FUNC_TOUPPER, 'q', 0, 0);
  uint64_t abs_result = poly_call_import3(POLY_IMPORT_FUNC_ABS,
    (uint64_t) (int64_t) -7, 0, 0);
  uint64_t labs_result = poly_call_import3(POLY_IMPORT_FUNC_LABS,
    (uint64_t) (int64_t) -11, 0, 0);
  uint64_t llabs_result = poly_call_import3(POLY_IMPORT_FUNC_LLABS,
    (uint64_t) (int64_t) -13, 0, 0);
  uint64_t ffs_result = poly_call_import3(POLY_IMPORT_FUNC_FFS, 0x10, 0, 0);
  uint64_t ffsl_result = poly_call_import3(POLY_IMPORT_FUNC_FFSL, 0x20, 0, 0);
  uint64_t ffsll_result =
    poly_call_import3(POLY_IMPORT_FUNC_FFSLL, 0x40, 0, 0);
  uint64_t numeric_ok = ((int64_t) atoi_result == -42) &&
    (atol_result == 123) && ((int64_t) atoll_result == -99) &&
    (strtol_result == 123) && (strtoul_result == 63) &&
    ((int64_t) strtoll_result == -42) && (strtoull_result == 123);
  uint64_t ctype_ok = isalnum_result && isalpha_result && isdigit_result &&
    islower_result && isspace_result && isupper_result && isxdigit_result &&
    isblank_result && iscntrl_result && isgraph_result && isprint_result &&
    ispunct_result && (tolower_result == 'q') && (toupper_result == 'Q');
  uint64_t integer_ok = (abs_result == 7) && (labs_result == 11) &&
    (llabs_result == 13) && (ffs_result == 5) && (ffsl_result == 6) &&
    (ffsll_result == 7);

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
    (strncpy_ret == ncopy_dest && strncpy_cmp == 0 ? 6 : 0) +
    (numeric_ok ? 5 : 0) + (ctype_ok ? 6 : 0) + (integer_ok ? 7 : 0);
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
