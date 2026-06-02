#include <stdint.h>

#include "../../include/polycpuid.h"
#include "../../include/polyruntime_imports.h"

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
static char poly_import_cat_dest[32] = "poly";
static char poly_import_cat_expected[] = "poly-isa";
static char poly_import_ncat_dest[32] = "trap";
static char poly_import_ncat_expected[] = "trap-pack";
static char poly_import_stpcpy_dest[16];
static char poly_import_stpncpy_dest[16];
static char poly_import_mempcpy_dest[16];
static char poly_import_span_text[] = "abc123";
static char poly_import_span_accept[] = "abc";
static char poly_import_span_reject[] = "123";
static char poly_import_pbrk_accept[] = "39";
static char poly_import_rawmemchr_text[] = "raw-helper";
static char poly_import_strchrnul_text[] = "none";
static char poly_import_memrchr_text[] = "abacad";
static char poly_import_memmem_haystack[] = "abcneedlezzz";
static char poly_import_memmem_needle[] = "needle";
static char poly_import_bcmp_expected[] = "trap-packets";
static char poly_import_bcopy_dest[16];
static char poly_import_bzero_dest[] = "zero";
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

static uint64_t poly_call_import4(uint64_t import_id, uint64_t arg0,
    uint64_t arg1, uint64_t arg2, uint64_t arg3) {
#if defined(__aarch64__)
  register uint64_t x0 __asm__("x0") = arg0;
  register uint64_t x1 __asm__("x1") = arg1;
  register uint64_t x2 __asm__("x2") = arg2;
  register uint64_t x3 __asm__("x3") = arg3;
  register uint64_t x16 __asm__("x16") = poly_import_target(import_id);
  __asm__ volatile("blr x16" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3),
    "+r"(x16) : : "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
      "x12", "x13", "x14", "x15", "x17", "x18", "x19", "x20", "x21",
      "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x30", "cc",
      "memory");
  return x0;
#elif defined(__riscv)
  register uint64_t a0 __asm__("a0") = arg0;
  register uint64_t a1 __asm__("a1") = arg1;
  register uint64_t a2 __asm__("a2") = arg2;
  register uint64_t a3 __asm__("a3") = arg3;
  register uint64_t t0 __asm__("t0") = poly_import_target(import_id);
  __asm__ volatile("jalr ra,0(t0)" : "+r"(a0), "+r"(a1), "+r"(a2),
    "+r"(a3), "+r"(t0) : : "a4", "a5", "a6", "a7", "t1", "t2", "t3",
      "t4", "t5", "t6", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
      "s8", "s9", "s10", "s11", "ra", "memory");
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
  uint64_t cat_dest = (uint64_t) (uintptr_t) poly_import_cat_dest;
  uint64_t cat_expected = (uint64_t) (uintptr_t) poly_import_cat_expected;
  uint64_t ncat_dest = (uint64_t) (uintptr_t) poly_import_ncat_dest;
  uint64_t ncat_expected = (uint64_t) (uintptr_t) poly_import_ncat_expected;
  uint64_t stpcpy_dest = (uint64_t) (uintptr_t) poly_import_stpcpy_dest;
  uint64_t stpncpy_dest = (uint64_t) (uintptr_t) poly_import_stpncpy_dest;
  uint64_t mempcpy_dest = (uint64_t) (uintptr_t) poly_import_mempcpy_dest;
  uint64_t span_text = (uint64_t) (uintptr_t) poly_import_span_text;
  uint64_t span_accept = (uint64_t) (uintptr_t) poly_import_span_accept;
  uint64_t span_reject = (uint64_t) (uintptr_t) poly_import_span_reject;
  uint64_t pbrk_accept = (uint64_t) (uintptr_t) poly_import_pbrk_accept;
  uint64_t rawmemchr_text =
    (uint64_t) (uintptr_t) poly_import_rawmemchr_text;
  uint64_t strchrnul_text =
    (uint64_t) (uintptr_t) poly_import_strchrnul_text;
  uint64_t memrchr_text = (uint64_t) (uintptr_t) poly_import_memrchr_text;
  uint64_t memmem_haystack =
    (uint64_t) (uintptr_t) poly_import_memmem_haystack;
  uint64_t memmem_needle = (uint64_t) (uintptr_t) poly_import_memmem_needle;
  uint64_t bcmp_expected =
    (uint64_t) (uintptr_t) poly_import_bcmp_expected;
  uint64_t bcopy_dest = (uint64_t) (uintptr_t) poly_import_bcopy_dest;
  uint64_t bzero_dest = (uint64_t) (uintptr_t) poly_import_bzero_dest;
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
  uint64_t strcat_ret =
    poly_call_import3(POLY_IMPORT_FUNC_STRCAT, cat_dest,
      (uint64_t) (uintptr_t) "-isa", 0);
  uint64_t strcat_cmp =
    poly_call_import3(POLY_IMPORT_FUNC_STRCMP, cat_dest, cat_expected, 0);
  uint64_t strncat_ret =
    poly_call_import3(POLY_IMPORT_FUNC_STRNCAT, ncat_dest,
      (uint64_t) (uintptr_t) "-packets", 5);
  uint64_t strncat_cmp =
    poly_call_import3(POLY_IMPORT_FUNC_STRCMP, ncat_dest, ncat_expected, 0);
  uint64_t stpcpy_ret =
    poly_call_import3(POLY_IMPORT_FUNC_STPCPY, stpcpy_dest, text, 0);
  uint64_t stpcpy_cmp =
    poly_call_import3(POLY_IMPORT_FUNC_STRCMP, stpcpy_dest, text, 0);
  uint64_t stpncpy_ret =
    poly_call_import3(POLY_IMPORT_FUNC_STPNCPY, stpncpy_dest, source, 6);
  uint64_t stpncpy_cmp =
    poly_call_import3(POLY_IMPORT_FUNC_MEMCMP, stpncpy_dest, source, 6);
  uint64_t mempcpy_ret =
    poly_call_import3(POLY_IMPORT_FUNC_MEMPCPY, mempcpy_dest, source,
      sizeof("trap-packets"));
  uint64_t mempcpy_cmp =
    poly_call_import3(POLY_IMPORT_FUNC_MEMCMP, mempcpy_dest, source,
      sizeof("trap-packets"));
  uint64_t strspn_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRSPN, span_text, span_accept, 0);
  uint64_t strcspn_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRCSPN, span_text, span_reject, 0);
  uint64_t strpbrk_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRPBRK, span_text, pbrk_accept, 0);
  uint64_t rawmemchr_result =
    poly_call_import3(POLY_IMPORT_FUNC_RAWMEMCHR, rawmemchr_text, 'h', 0);
  uint64_t strchrnul_result =
    poly_call_import3(POLY_IMPORT_FUNC_STRCHRNUL, strchrnul_text, 'z', 0);
  uint64_t memrchr_result =
    poly_call_import3(POLY_IMPORT_FUNC_MEMRCHR, memrchr_text, 'a',
      sizeof("abacad") - 1);
  uint64_t memmem_result =
    poly_call_import4(POLY_IMPORT_FUNC_MEMMEM, memmem_haystack,
      sizeof("abcneedlezzz") - 1, memmem_needle, sizeof("needle") - 1);
  uint64_t bcmp_result =
    poly_call_import3(POLY_IMPORT_FUNC_BCMP, source, bcmp_expected,
      sizeof("trap-packets") - 1);
  uint64_t bcopy_result =
    poly_call_import3(POLY_IMPORT_FUNC_BCOPY, source, bcopy_dest,
      sizeof("trap-packets"));
  uint64_t bcopy_cmp =
    poly_call_import3(POLY_IMPORT_FUNC_MEMCMP, bcopy_dest, source,
      sizeof("trap-packets"));
  uint64_t bzero_result =
    poly_call_import3(POLY_IMPORT_FUNC_BZERO, bzero_dest, 2, 0);
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
    (strcat_ret == cat_dest && strcat_cmp == 0 ? 41 : 0) +
    (strncat_ret == ncat_dest && strncat_cmp == 0 ? 43 : 0) +
    (stpcpy_ret == stpcpy_dest + len && stpcpy_cmp == 0 ? 73 : 0) +
    (stpncpy_ret == stpncpy_dest + 6 && stpncpy_cmp == 0 ? 79 : 0) +
    (mempcpy_ret == mempcpy_dest + sizeof("trap-packets") &&
      mempcpy_cmp == 0 ? 83 : 0) +
    (strspn_result == 3 ? 47 : 0) + (strcspn_result == 3 ? 53 : 0) +
    (strpbrk_result == span_text + 5 ? 59 : 0) +
    (rawmemchr_result == rawmemchr_text + 4 ? 89 : 0) +
    (strchrnul_result == strchrnul_text + 4 ? 97 : 0) +
    (memrchr_result == memrchr_text + 4 ? 101 : 0) +
    (memmem_result == memmem_haystack + 3 ? 103 : 0) +
    (bcmp_result == 0 ? 61 : 0) +
    (bcopy_result == 0 && bcopy_cmp == 0 ? 67 : 0) +
    (bzero_result == 0 && poly_import_bzero_dest[0] == 0 &&
      poly_import_bzero_dest[1] == 0 &&
      poly_import_bzero_dest[2] == 'r' ? 71 : 0) +
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
