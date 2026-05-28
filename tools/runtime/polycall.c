#define _GNU_SOURCE

#include <errno.h>
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif
#ifndef DT_GNU_HASH
#define DT_GNU_HASH 0x6ffffef5
#endif
#ifndef DT_VERSYM
#define DT_VERSYM 0x6ffffff0
#endif
#ifndef DT_VERDEF
#define DT_VERDEF 0x6ffffffc
#endif
#ifndef DT_VERDEFNUM
#define DT_VERDEFNUM 0x6ffffffd
#endif
#ifndef DT_VERNEED
#define DT_VERNEED 0x6ffffffe
#endif
#ifndef DT_VERNEEDNUM
#define DT_VERNEEDNUM 0x6fffffff
#endif
#ifndef DT_RELR
#define DT_RELR 36
#endif
#ifndef DT_SYMBOLIC
#define DT_SYMBOLIC 16
#endif
#ifndef DT_FLAGS
#define DT_FLAGS 30
#endif
#ifndef DT_RELRSZ
#define DT_RELRSZ 35
#endif
#ifndef DT_RELRENT
#define DT_RELRENT 37
#endif
#ifndef DF_1_INITFIRST
#define DF_1_INITFIRST 0x20
#endif
#ifndef DF_SYMBOLIC
#define DF_SYMBOLIC 0x2
#endif
#ifndef R_AARCH64_NONE
#define R_AARCH64_NONE 0
#endif
#ifndef R_AARCH64_IRELATIVE
#define R_AARCH64_IRELATIVE 1032
#endif
#ifndef R_AARCH64_COPY
#define R_AARCH64_COPY 1024
#endif
#ifndef R_AARCH64_TLSDESC
#define R_AARCH64_TLSDESC 1031
#endif
#ifndef R_RISCV_COPY
#define R_RISCV_COPY 4
#endif
#ifndef R_RISCV_NONE
#define R_RISCV_NONE 0
#endif
#ifndef R_AARCH64_TLS_DTPMOD64
#define R_AARCH64_TLS_DTPMOD64 1028
#endif
#ifndef R_AARCH64_TLS_DTPREL64
#define R_AARCH64_TLS_DTPREL64 1029
#endif
#ifndef R_AARCH64_TLS_TPREL64
#define R_AARCH64_TLS_TPREL64 1030
#endif
#ifndef R_RISCV_IRELATIVE
#define R_RISCV_IRELATIVE 58
#endif
#ifndef R_RISCV_TLS_DTPMOD64
#define R_RISCV_TLS_DTPMOD64 7
#endif
#ifndef R_RISCV_TLS_DTPREL64
#define R_RISCV_TLS_DTPREL64 9
#endif
#ifndef R_RISCV_TLS_TPREL64
#define R_RISCV_TLS_TPREL64 11
#endif
#ifndef STT_GNU_IFUNC
#define STT_GNU_IFUNC 10
#endif
#ifndef STB_GNU_UNIQUE
#define STB_GNU_UNIQUE 10
#endif
#ifndef VER_NDX_LOCAL
#define VER_NDX_LOCAL 0
#endif
#ifndef VER_NDX_GLOBAL
#define VER_NDX_GLOBAL 1
#endif
#ifndef VERSYM_HIDDEN
#define VERSYM_HIDDEN 0x8000
#endif
#ifndef VERSYM_VERSION
#define VERSYM_VERSION 0x7fff
#endif

enum {
  POLY_MODE_X86 = 0,
  POLY_MODE_RAW_AARCH64 = 3,
  POLY_MODE_RAW_RISCV = 4,
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
  POLY_CALL_U64 = 0,
  POLY_CALL_FP64 = 1,
  POLY_CALL_FP32 = 2,
  POLY_CALL_PAIR_U64 = 3,
  POLY_CALL_SRET_U64 = 4,
  POLY_CALL_FPAIR64 = 5,
  POLY_CALL_FPAIR64_ARG = 6,
  POLY_CALL_MIXED_ARGS = 7,
  POLY_CALL_FINI_RESULT = 8,
  POLY_CALL_FPAIR32 = 9,
  POLY_CALL_FPAIR32_ARG = 10,
  POLY_CALL_HETERO_U64_F64 = 11,
  POLY_CALL_HETERO_F64_U64 = 12,
  POLY_CALL_HETERO_U64_F32 = 13,
  POLY_CALL_HETERO_F32_U64 = 14,
  POLY_CALL_HETERO_U32_F64 = 15,
  POLY_CALL_HETERO_F64_U32 = 16,
  POLY_CALL_COMPACT_U32_F32 = 17,
  POLY_CALL_COMPACT_F32_U32 = 18,
  POLY_CALL_FP64_STACK = 19,
  POLY_CALL_DEP_FINI_RESULT = 20,
  POLY_CALL_VEC128_U32 = 21,
  POLY_CALL_ONE_U64 = 22,
  POLY_CALL_AARCH64_HFA3_F64 = 23,
  POLY_CALL_AARCH64_HFA4_F64 = 24,
  POLY_CALL_AARCH64_HFA3_F32 = 25,
  POLY_CALL_AARCH64_HFA4_F32 = 26,
  POLY_CALL_AARCH64_HFA3_F64_ARG = 27,
  POLY_CALL_AARCH64_HFA4_F64_ARG = 28,
  POLY_CALL_AARCH64_HFA3_F32_ARG = 29,
  POLY_CALL_AARCH64_HFA4_F32_ARG = 30,
  MAX_PROGRAM_BYTES = 1024 * 1024,
  MAX_DYNAMIC_RELOCS = 4096,
  MAX_TLS_BYTES = 4096,
  MAX_TOTAL_TLS_BYTES = 64 * 1024,
  POLY_ERRNO_TLS_OFFSET = 4096,
  POLY_ERRNO_TLS_SIZE = 4104,
  MAX_NEEDED_DEPS = 32,
  MAX_ATEXIT_CALLBACKS = 128,
  MAX_LOAD_SEGMENTS = 16,
  MAX_DEP_PATH = 192,
  MAX_BRIDGE_SPECS = 64,
  MAX_BRIDGE_SYMBOL = 128,
  RELOC_BASE_ABSOLUTE = 0,
  RELOC_BASE_LOAD_BIAS = 1,
  RELOC_BASE_IMPORT_PAGE = 2,
  RELOC_BASE_IMPORT_CALL = 3,
  RELOC_BASE_IRELATIVE = 4,
  RELOC_BASE_TLS_OFFSET = 5,
  RELOC_BASE_ROOT_LOAD_BIAS = 6,
  RELOC_BASE_ROOT_TLS_OFFSET = 7,
  RELOC_BASE_ROOT_IFUNC = 8,
  RELOC_BASE_ROOT_CROSS_STUB = 9,
  RELOC_BASE_ROOT_CROSS_STUB_VEC128 = 10,
  RELOC_BASE_ROOT_CROSS_STUB_COMPACT_U32_F32 = 11,
  RELOC_BASE_ROOT_CROSS_STUB_COMPACT_F32_U32 = 12,
  RELOC_BASE_ROOT_CROSS_STUB_FP64_STACK = 13,
  RELOC_BASE_ROOT_IFUNC_VEC128 = 14,
  RELOC_BASE_ROOT_IFUNC_COMPACT_U32_F32 = 15,
  RELOC_BASE_ROOT_IFUNC_COMPACT_F32_U32 = 16,
  RELOC_BASE_ROOT_IFUNC_FP64_STACK = 17,
  RELOC_BASE_DEP_LOAD_BIAS = 100,
  RELOC_BASE_DEP_COPY = 200,
  RELOC_BASE_DEP_IFUNC = 300,
  RELOC_BASE_DEP_TLS_OFFSET = 400,
  RELOC_BASE_DEP_CROSS_STUB = 500,
  RELOC_BASE_DEP_CROSS_IFUNC = 600,
  RELOC_BASE_DEP_CROSS_STUB_VEC128 = 700,
  RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32 = 800,
  RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32 = 900,
  RELOC_BASE_DEP_CROSS_STUB_FP64_STACK = 1000,
  RELOC_BASE_DEP_CROSS_IFUNC_VEC128 = 1100,
  RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 = 1200,
  RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 = 1300,
  RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK = 1400,
  POLY_CROSS_BRIDGE_DEFAULT = 0,
  POLY_CROSS_BRIDGE_COMPACT_U32_F32 = 1,
  POLY_CROSS_BRIDGE_COMPACT_F32_U32 = 2,
  POLY_CROSS_BRIDGE_FP64_STACK = 3,
  POLY_CROSS_BRIDGE_VEC128_U32 = 4
};

static const uint32_t POLY_CPUID_BASE = 0x40000000U;
static const uint32_t POLY_CPUID_MAX = 0x40000009U;
static const uint32_t POLY_CPUID_ABI_VERSION = 1U;
static const uint64_t POLY_IMPORT_CALL_BASE = 0xffffffffffffe000ULL;
static const uint64_t POLY_IMPORT_CALL_STRIDE = 0x10;
static const size_t POLY_X86_IMPORT_DESCRIPTOR_SIZE = 32;

enum {
  POLY_CPUID_FEATURE_RAW_AARCH64 = (1U << 0),
  POLY_CPUID_FEATURE_RAW_RISCV = (1U << 1),
  POLY_CPUID_FEATURE_NEUTRAL_SWITCH = (1U << 2),
  POLY_CPUID_FEATURE_NATIVE_RET = (1U << 3),
  POLY_CPUID_FEATURE_PCALL_SYSV = (1U << 4),
  POLY_CPUID_FEATURE_PCALL_SRET = (1U << 5),
  POLY_CPUID_FEATURE_FP_BRIDGE = (1U << 6),
  POLY_CPUID_FEATURE_X86_POLY_OPCODES = (1U << 12),
  POLY_CPUID_FEATURE_FPAIR32_RET = (1U << 13),
  POLY_CPUID_FEATURE_FPAIR32_ARG = (1U << 14),
  POLY_CPUID_FEATURE_HETERO_U64_F64 = (1U << 15),
  POLY_CPUID_FEATURE_HETERO_F64_U64 = (1U << 16),
  POLY_CPUID_FEATURE_HETERO_U64_F32 = (1U << 17),
  POLY_CPUID_FEATURE_HETERO_F32_U64 = (1U << 18),
  POLY_CPUID_FEATURE_COMPACT_U32_F32 = (1U << 19),
  POLY_CPUID_FEATURE_COMPACT_F32_U32 = (1U << 20),
  POLY_CPUID_FEATURE_X86_IMPORT_DESCRIPTORS = (1U << 22),
  POLY_CPUID_FEATURE_FP64_STACK_ARGS = (1U << 23),
  POLY_CPUID_FEATURE_VEC128_BRIDGE = (1U << 27),
  POLY_CPUID_FEATURE_AARCH64_HFA64_RET = (1U << 28),
  POLY_CPUID_FEATURE_AARCH64_HFA32_RET = (1U << 29),
  POLY_CPUID_FEATURE_AARCH64_HFA_ARGS = (1U << 30),
  POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS = (1U << 0),
  POLY_IMPORT_X86_DESCRIPTOR_RETURN_I128 = (1U << 1),
  POLY_IMPORT_X86_DESCRIPTOR_RETURN_FP128 = (1U << 2),
  POLY_IMPORT_X86_DESCRIPTOR_STACK_FROM_MEMORY = (1U << 3),
  POLY_IMPORT_X86_DESCRIPTOR_STACK_FROM_GPR0 = (1U << 4),
  POLY_IMPORT_X86_DESCRIPTOR_RETURN_FPAIR64 = (1U << 5),
  POLY_IMPORT_X86_DESCRIPTOR_RETURN_FPAIR32 = (1U << 6),
  POLY_IMPORT_X86_DESCRIPTOR_RETURN_VEC128 = (1U << 7),
  POLY_IMPORT_X86_DESCRIPTOR_VEC128_FROM_GPR_PAIRS = (1U << 8),
  POLY_IMPORT_X86_DESCRIPTOR_AARCH64_SRET_X8 = (1U << 9),
  POLY_ABI_BRIDGE_ABI_VERSION = 1,
  POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64 = (1U << 0),
  POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV = (1U << 1),
  POLY_ABI_BRIDGE_FLAG_SRET = (1U << 2),
  POLY_ABI_BRIDGE_FLAG_SCALAR_FP = (1U << 3),
  POLY_ABI_BRIDGE_FLAG_FOCUSED_AGGREGATES = (1U << 4),
  POLY_ABI_BRIDGE_FLAG_FP64_STACK = (1U << 5),
  POLY_ABI_BRIDGE_FLAG_DESCRIPTOR_IMPORTS = (1U << 6),
  POLY_ABI_BRIDGE_FLAG_TLS_BASE = (1U << 7),
  POLY_ABI_BRIDGE_FLAG_USER_DESCRIPTORS = (1U << 8),
  POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK = (1U << 9),
  POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET = (1U << 10),
  POLY_ABI_BRIDGE_FLAG_VEC128 = (1U << 11),
  POLY_ABI_BRIDGE_GPR_ARG_COUNT = 8,
  POLY_ABI_BRIDGE_FP_ARG_COUNT = 8,
  POLY_ABI_BRIDGE_STACK_ALIGN = 16
};

static const uint32_t POLY_ABI_BRIDGE_REQUIRED_FLAGS =
  POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64 |
  POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV |
  POLY_ABI_BRIDGE_FLAG_SRET |
  POLY_ABI_BRIDGE_FLAG_SCALAR_FP |
  POLY_ABI_BRIDGE_FLAG_FOCUSED_AGGREGATES |
  POLY_ABI_BRIDGE_FLAG_FP64_STACK |
  POLY_ABI_BRIDGE_FLAG_DESCRIPTOR_IMPORTS |
  POLY_ABI_BRIDGE_FLAG_TLS_BASE |
  POLY_ABI_BRIDGE_FLAG_USER_DESCRIPTORS |
  POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK |
  POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET |
  POLY_ABI_BRIDGE_FLAG_VEC128;

static const uint32_t POLY_CPUID_REQUIRED_MODES =
  (1U << POLY_MODE_X86) |
  (1U << POLY_MODE_RAW_AARCH64) |
  (1U << POLY_MODE_RAW_RISCV);

static const uint32_t POLY_CPUID_REQUIRED_FEATURES =
  POLY_CPUID_FEATURE_RAW_AARCH64 |
  POLY_CPUID_FEATURE_RAW_RISCV |
  POLY_CPUID_FEATURE_NEUTRAL_SWITCH |
  POLY_CPUID_FEATURE_NATIVE_RET |
  POLY_CPUID_FEATURE_PCALL_SYSV |
  POLY_CPUID_FEATURE_PCALL_SRET |
  POLY_CPUID_FEATURE_FP_BRIDGE |
  POLY_CPUID_FEATURE_X86_POLY_OPCODES |
  POLY_CPUID_FEATURE_FPAIR32_RET |
  POLY_CPUID_FEATURE_FPAIR32_ARG |
  POLY_CPUID_FEATURE_HETERO_U64_F64 |
  POLY_CPUID_FEATURE_HETERO_F64_U64 |
  POLY_CPUID_FEATURE_HETERO_U64_F32 |
  POLY_CPUID_FEATURE_HETERO_F32_U64 |
  POLY_CPUID_FEATURE_COMPACT_U32_F32 |
  POLY_CPUID_FEATURE_COMPACT_F32_U32 |
  POLY_CPUID_FEATURE_X86_IMPORT_DESCRIPTORS |
  POLY_CPUID_FEATURE_FP64_STACK_ARGS |
  POLY_CPUID_FEATURE_VEC128_BRIDGE |
  POLY_CPUID_FEATURE_AARCH64_HFA64_RET |
  POLY_CPUID_FEATURE_AARCH64_HFA32_RET |
  POLY_CPUID_FEATURE_AARCH64_HFA_ARGS;

enum {
  POLY_IMPORT_FUNC_ADD = 0,
  POLY_IMPORT_FUNC_MUL = 1,
  POLY_IMPORT_FUNC_RESERVED_LEGACY_X86_ADD = 2,
  POLY_IMPORT_FUNC_FP64_ADD = 3,
  POLY_IMPORT_FUNC_AARCH64_LDADD8_ACQ_REL = 4,
  POLY_IMPORT_FUNC_AARCH64_SWP8_ACQ_REL = 5,
  POLY_IMPORT_FUNC_AARCH64_LDSET4_RELAX = 6,
  POLY_IMPORT_FUNC_AARCH64_CAS8_ACQ_REL = 7,
  POLY_IMPORT_FUNC_STRLEN = 8,
  POLY_IMPORT_FUNC_MEMCPY = 9,
  POLY_IMPORT_FUNC_MEMSET = 10,
  POLY_IMPORT_FUNC_MEMCMP = 11,
  POLY_IMPORT_FUNC_AARCH64_TLSDESC = 12,
  POLY_IMPORT_FUNC_RISCV_TLS_GET_ADDR = 13,
  POLY_IMPORT_FUNC_FP32_ADD = 14,
  POLY_IMPORT_FUNC_MEMMOVE = 15,
  POLY_IMPORT_FUNC_STRCMP = 16,
  POLY_IMPORT_FUNC_STRNCMP = 17,
  POLY_IMPORT_FUNC_MEMCHR = 18,
  POLY_IMPORT_FUNC_STRCHR = 19,
  POLY_IMPORT_FUNC_STRRCHR = 20,
  POLY_IMPORT_FUNC_STRSTR = 21,
  POLY_IMPORT_FUNC_STRCPY = 22,
  POLY_IMPORT_FUNC_STRNCPY = 23,
  POLY_IMPORT_FUNC_STRNLEN = 24,
  POLY_IMPORT_FUNC_STRCAT = 25,
  POLY_IMPORT_FUNC_STRNCAT = 26,
  POLY_IMPORT_FUNC_STRSPN = 27,
  POLY_IMPORT_FUNC_STRCSPN = 28,
  POLY_IMPORT_FUNC_STRPBRK = 29,
  POLY_IMPORT_FUNC_STPCPY = 30,
  POLY_IMPORT_FUNC_STPNCPY = 31,
  POLY_IMPORT_FUNC_MEMPCPY = 32,
  POLY_IMPORT_FUNC_RAWMEMCHR = 33,
  POLY_IMPORT_FUNC_STRCHRNUL = 34,
  POLY_IMPORT_FUNC_BCMP = 35,
  POLY_IMPORT_FUNC_BCOPY = 36,
  POLY_IMPORT_FUNC_BZERO = 37,
  POLY_IMPORT_FUNC_MEMRCHR = 38,
  POLY_IMPORT_FUNC_MEMMEM = 39,
  POLY_IMPORT_FUNC_STRCASECMP = 40,
  POLY_IMPORT_FUNC_STRNCASECMP = 41,
  POLY_IMPORT_FUNC_STRCASESTR = 42,
  POLY_IMPORT_FUNC_AARCH64_CAS4_ACQ_REL = 43,
  POLY_IMPORT_FUNC_AARCH64_LDADD4_ACQ_REL = 44,
  POLY_IMPORT_FUNC_AARCH64_SWP4_ACQ_REL = 45,
  POLY_IMPORT_FUNC_AARCH64_LDCLR8_ACQ_REL = 46,
  POLY_IMPORT_FUNC_AARCH64_LDEOR8_ACQ_REL = 47,
  POLY_IMPORT_FUNC_AARCH64_LDCLR4_ACQ_REL = 48,
  POLY_IMPORT_FUNC_AARCH64_LDEOR4_ACQ_REL = 49,
  POLY_IMPORT_FUNC_AARCH64_LDSET8_ACQ_REL = 50,
  POLY_IMPORT_FUNC_AARCH64_LDSET4_ACQ_REL = 51,
  POLY_IMPORT_FUNC_AARCH64_LDADD2_ACQ_REL = 52,
  POLY_IMPORT_FUNC_AARCH64_LDADD1_ACQ_REL = 53,
  POLY_IMPORT_FUNC_AARCH64_SWP2_ACQ_REL = 54,
  POLY_IMPORT_FUNC_AARCH64_SWP1_ACQ_REL = 55,
  POLY_IMPORT_FUNC_AARCH64_LDCLR2_ACQ_REL = 56,
  POLY_IMPORT_FUNC_AARCH64_LDCLR1_ACQ_REL = 57,
  POLY_IMPORT_FUNC_AARCH64_LDEOR2_ACQ_REL = 58,
  POLY_IMPORT_FUNC_AARCH64_LDEOR1_ACQ_REL = 59,
  POLY_IMPORT_FUNC_AARCH64_LDSET2_ACQ_REL = 60,
  POLY_IMPORT_FUNC_AARCH64_LDSET1_ACQ_REL = 61,
  POLY_IMPORT_FUNC_AARCH64_CAS2_ACQ_REL = 62,
  POLY_IMPORT_FUNC_AARCH64_CAS1_ACQ_REL = 63,
  POLY_IMPORT_FUNC_ATOMIC_COMPARE_EXCHANGE_16 = 64,
  POLY_IMPORT_FUNC_ATOMIC_LOAD_16 = 65,
  POLY_IMPORT_FUNC_ATOMIC_STORE_16 = 66,
  POLY_IMPORT_FUNC_UDIVTI3 = 67,
  POLY_IMPORT_FUNC_UMODTI3 = 68,
  POLY_IMPORT_FUNC_DIVTI3 = 69,
  POLY_IMPORT_FUNC_MODTI3 = 70,
  POLY_IMPORT_FUNC_FIXDFTI = 71,
  POLY_IMPORT_FUNC_FIXUNSDFTI = 72,
  POLY_IMPORT_FUNC_FLOATTIDF = 73,
  POLY_IMPORT_FUNC_FLOATUNTIDF = 74,
  POLY_IMPORT_FUNC_FIXSFTI = 75,
  POLY_IMPORT_FUNC_FIXUNSSFTI = 76,
  POLY_IMPORT_FUNC_FLOATTISF = 77,
  POLY_IMPORT_FUNC_FLOATUNTISF = 78,
  POLY_IMPORT_FUNC_CLZDI2 = 79,
  POLY_IMPORT_FUNC_CTZDI2 = 80,
  POLY_IMPORT_FUNC_PARITYDI2 = 81,
  POLY_IMPORT_FUNC_POPCOUNTDI2 = 82,
  POLY_IMPORT_FUNC_ADDTF3 = 83,
  POLY_IMPORT_FUNC_SUBTF3 = 84,
  POLY_IMPORT_FUNC_MULTF3 = 85,
  POLY_IMPORT_FUNC_DIVTF3 = 86,
  POLY_IMPORT_FUNC_FLOATUNDITF = 87,
  POLY_IMPORT_FUNC_FIXUNSTFDI = 88,
  POLY_IMPORT_FUNC_FLOATDITF = 89,
  POLY_IMPORT_FUNC_FLOATSITF = 90,
  POLY_IMPORT_FUNC_FIXTFDI = 91,
  POLY_IMPORT_FUNC_EQTF2 = 92,
  POLY_IMPORT_FUNC_LTTF2 = 93,
  POLY_IMPORT_FUNC_LETF2 = 94,
  POLY_IMPORT_FUNC_GTTF2 = 95,
  POLY_IMPORT_FUNC_GETF2 = 96,
  POLY_IMPORT_FUNC_EXTENDSFTF2 = 97,
  POLY_IMPORT_FUNC_EXTENDDFTF2 = 98,
  POLY_IMPORT_FUNC_TRUNCTFSF2 = 99,
  POLY_IMPORT_FUNC_TRUNCTFDF2 = 100,
  POLY_IMPORT_FUNC_NETF2 = 101,
  POLY_IMPORT_FUNC_UNORDTF2 = 102,
  POLY_IMPORT_FUNC_FLOATUNSITF = 103,
  POLY_IMPORT_FUNC_FIXTFSI = 104,
  POLY_IMPORT_FUNC_FIXUNSTFSI = 105,
  POLY_IMPORT_FUNC_X86_SLOT0 = 106,
  POLY_IMPORT_FUNC_X86_SLOT1 = 107,
  POLY_IMPORT_FUNC_X86_SLOT2 = 108,
  POLY_IMPORT_FUNC_X86_SLOT3 = 109,
  POLY_IMPORT_FUNC_X86_SLOT4 = 110,
  POLY_IMPORT_FUNC_X86_SLOT5 = 111,
  POLY_IMPORT_FUNC_X86_SLOT6 = 112,
  POLY_IMPORT_FUNC_X86_SLOT7 = 113,
  POLY_IMPORT_FUNC_STACK_CHK_FAIL = 114,
  POLY_IMPORT_FUNC_ERRNO_LOCATION = 115,
  POLY_IMPORT_FUNC_GETAUXVAL = 116,
  POLY_IMPORT_FUNC_GETPAGESIZE = 117,
  POLY_IMPORT_FUNC_SYSCONF = 118,
  POLY_IMPORT_FUNC_GETENV = 119,
  POLY_IMPORT_FUNC_SECURE_GETENV = 120,
  POLY_IMPORT_FUNC_MALLOC = 121,
  POLY_IMPORT_FUNC_CALLOC = 122,
  POLY_IMPORT_FUNC_REALLOC = 123,
  POLY_IMPORT_FUNC_FREE = 124,
  POLY_IMPORT_FUNC_STRDUP = 125,
  POLY_IMPORT_FUNC_STRNDUP = 126,
  POLY_IMPORT_FUNC_POSIX_MEMALIGN = 127,
  POLY_IMPORT_FUNC_ALIGNED_ALLOC = 128,
  POLY_IMPORT_FUNC_MEMALIGN = 129,
  POLY_IMPORT_FUNC_ATEXIT = 130,
  POLY_IMPORT_FUNC_CXA_ATEXIT = 131,
  POLY_IMPORT_FUNC_CXA_FINALIZE = 132,
  POLY_IMPORT_FUNC_GETPID = 133,
  POLY_IMPORT_FUNC_GETPPID = 134,
  POLY_IMPORT_FUNC_GETUID = 135,
  POLY_IMPORT_FUNC_GETEUID = 136,
  POLY_IMPORT_FUNC_GETGID = 137,
  POLY_IMPORT_FUNC_GETEGID = 138,
  POLY_IMPORT_FUNC_GETTID = 139,
  POLY_IMPORT_FUNC_PUTS = 140,
  POLY_IMPORT_FUNC_CXA_GUARD_ACQUIRE = 141,
  POLY_IMPORT_FUNC_CXA_GUARD_RELEASE = 142,
  POLY_IMPORT_FUNC_CXA_GUARD_ABORT = 143,
  POLY_IMPORT_FUNC_X86_SUM10 = 144,
  POLY_IMPORT_FUNC_X86_FP64_SUM10 = 145,
  POLY_IMPORT_FUNC_X86_FPAIR64 = 146,
  POLY_IMPORT_FUNC_X86_FPAIR32 = 147,
  POLY_IMPORT_FUNC_X86_VEC128_U32 = 148,
  POLY_IMPORT_FUNC_X86_SRET_U64 = 149,
  POLY_IMPORT_FUNC_X86_SRET_U64_STACK = 150,
  POLY_IMPORT_FUNC_X86_SRET_U64_STACK10 = 151,
  POLY_IMPORT_FUNC_X86_SUM14 = 152,
  POLY_IMPORT_FUNC_X86_ALIGN14 = 153,
  POLY_IMPORT_FUNC_X86_I128 = 154,
  POLY_IMPORT_FUNC_QSORT = 155,
  POLY_IMPORT_FUNC_BSEARCH = 156,
  POLY_IMPORT_FUNC_QSORT_R = 157,
  POLY_IMPORT_FUNC_PTHREAD_ONCE = 158,
  POLY_IMPORT_FUNC_PTHREAD_KEY_CREATE = 159,
  POLY_IMPORT_FUNC_PTHREAD_KEY_DELETE = 160,
  POLY_IMPORT_FUNC_PTHREAD_GETSPECIFIC = 161,
  POLY_IMPORT_FUNC_PTHREAD_SETSPECIFIC = 162,
  POLY_IMPORT_FUNC_PTHREAD_MUTEX_INIT = 163,
  POLY_IMPORT_FUNC_PTHREAD_MUTEX_DESTROY = 164,
  POLY_IMPORT_FUNC_PTHREAD_MUTEX_LOCK = 165,
  POLY_IMPORT_FUNC_PTHREAD_MUTEX_TRYLOCK = 166,
  POLY_IMPORT_FUNC_PTHREAD_MUTEX_UNLOCK = 167,
  POLY_IMPORT_FUNC_PTHREAD_SELF = 168,
  POLY_IMPORT_FUNC_PTHREAD_EQUAL = 169,
  POLY_IMPORT_FUNC_PTHREAD_RWLOCK_INIT = 170,
  POLY_IMPORT_FUNC_PTHREAD_RWLOCK_DESTROY = 171,
  POLY_IMPORT_FUNC_PTHREAD_RWLOCK_RDLOCK = 172,
  POLY_IMPORT_FUNC_PTHREAD_RWLOCK_TRYRDLOCK = 173,
  POLY_IMPORT_FUNC_PTHREAD_RWLOCK_WRLOCK = 174,
  POLY_IMPORT_FUNC_PTHREAD_RWLOCK_TRYWRLOCK = 175,
  POLY_IMPORT_FUNC_PTHREAD_RWLOCK_UNLOCK = 176,
  POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_INIT = 177,
  POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_DESTROY = 178,
  POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_SETTYPE = 179,
  POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_GETTYPE = 180,
  POLY_IMPORT_FUNC_PTHREAD_SPIN_INIT = 181,
  POLY_IMPORT_FUNC_PTHREAD_SPIN_DESTROY = 182,
  POLY_IMPORT_FUNC_PTHREAD_SPIN_LOCK = 183,
  POLY_IMPORT_FUNC_PTHREAD_SPIN_TRYLOCK = 184,
  POLY_IMPORT_FUNC_PTHREAD_SPIN_UNLOCK = 185,
  POLY_IMPORT_FUNC_PTHREAD_COND_INIT = 186,
  POLY_IMPORT_FUNC_PTHREAD_COND_DESTROY = 187,
  POLY_IMPORT_FUNC_PTHREAD_COND_SIGNAL = 188,
  POLY_IMPORT_FUNC_PTHREAD_COND_BROADCAST = 189,
  POLY_IMPORT_FUNC_CLOCK_GETTIME = 190,
  POLY_IMPORT_FUNC_CLOCK_GETRES = 191,
  POLY_IMPORT_FUNC_TIME = 192,
  POLY_IMPORT_FUNC_GETTIMEOFDAY = 193,
  POLY_IMPORT_FUNC_CLOCK = 194,
  POLY_IMPORT_FUNC_COUNT = 195
};

enum {
  POLY_IMPORT_PAGE_VALUE_OFFSET = 0,
  POLY_IMPORT_PAGE_STACK_GUARD_OFFSET = 8,
  POLY_IMPORT_PAGE_HEAP_BASE_OFFSET = 16,
  POLY_IMPORT_PAGE_HEAP_SIZE_OFFSET = 24,
  POLY_IMPORT_PAGE_HEAP_CURSOR_OFFSET = 32,
  POLY_IMPORT_HEAP_SIZE = 64 * 1024
};

struct poly_cpuid_regs {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

struct poly_import_contract {
  uint64_t call_base;
  uint32_t call_stride;
  uint32_t import_count;
  uint32_t x86_slot0;
  uint32_t x86_slot_count;
  uint32_t x86_descriptor_size;
  uint32_t abi_flags;
  uint32_t gpr_arg_count;
  uint32_t fp_arg_count;
  uint32_t stack_align;
};

struct poly_dynamic_reloc {
  size_t offset;
  size_t size;
  uint64_t value;
  int base_kind;
};

struct poly_symbol_table {
  const Elf64_Sym *symbols;
  size_t symbol_count;
  const char *strings;
  size_t strings_size;
  const Elf64_Half *versym;
  size_t versym_count;
  const uint8_t *verdef;
  size_t verdef_size;
  size_t verdef_count;
  const uint8_t *verneed;
  size_t verneed_size;
  size_t verneed_count;
};

struct poly_load_segment {
  uint64_t vaddr;
  uint64_t memsz;
  uint32_t flags;
};

struct poly_bridge_spec {
  char symbol[MAX_BRIDGE_SYMBOL];
  int bridge_kind;
};

struct poly_dependency {
  char path[MAX_DEP_PATH];
  char soname[MAX_DEP_PATH];
  const char *arch_name;
  int arch;
  uint8_t *image;
  size_t image_size;
  uint64_t base_vaddr;
  size_t loaded_bytes;
  struct poly_load_segment load_segments[MAX_LOAD_SEGMENTS];
  size_t load_segment_count;
  uint64_t init_vaddr;
  uint64_t init_array_vaddr;
  uint64_t init_array_size;
  size_t init_count;
  int init_first;
  uint64_t fini_vaddr;
  uint64_t fini_array_vaddr;
  uint64_t fini_array_size;
  size_t fini_count;
  uint64_t relro_vaddr;
  uint64_t relro_size;
  uint64_t tls_vaddr;
  uint64_t tls_filesz;
  uint64_t tls_memsz;
  uint64_t tls_align;
  size_t tls_offset;
  size_t needed_depth;
  size_t lookup_rank;
  int symbolic_binding;
  struct poly_symbol_table dynsym;
  struct poly_bridge_spec bridge_specs[MAX_BRIDGE_SPECS];
  size_t bridge_spec_count;
  struct poly_dynamic_reloc *relocs;
  size_t reloc_count;
};

struct poly_version_requirement {
  const char *name;
  const char *filename;
};

struct poly_program {
  const char *path;
  const char *arch_name;
  int arch;
  int root_arch;
  int elf_type;
  uint8_t *image;
  size_t image_size;
  size_t entry_offset;
  uint64_t base_vaddr;
  size_t loaded_bytes;
  struct poly_load_segment load_segments[MAX_LOAD_SEGMENTS];
  size_t load_segment_count;
  uint64_t preinit_array_vaddr;
  uint64_t preinit_array_size;
  size_t preinit_count;
  uint64_t tls_vaddr;
  uint64_t tls_filesz;
  uint64_t tls_memsz;
  uint64_t tls_align;
  size_t tls_offset;
  size_t tls_total_size;
  uint64_t init_vaddr;
  uint64_t init_array_vaddr;
  uint64_t init_array_size;
  size_t init_count;
  int init_first;
  uint64_t fini_vaddr;
  uint64_t fini_array_vaddr;
  uint64_t fini_array_size;
  uint64_t fini_result_vaddr;
  int fini_result_base_kind;
  int fini_result_bridge_kind;
  size_t fini_count;
  uint64_t relro_vaddr;
  uint64_t relro_size;
  struct poly_dynamic_reloc *relocs;
  size_t reloc_count;
  struct poly_symbol_table root_dynsym;
  struct poly_bridge_spec root_bridge_specs[MAX_BRIDGE_SPECS];
  size_t root_bridge_spec_count;
  struct poly_dependency deps[MAX_NEEDED_DEPS];
  size_t dep_count;
  size_t direct_dep_count;
  int symbolic_binding;
  int dependency_view;
  int needs_x86_import;
  int needs_errno_location;
};

struct poly_request {
  char path[160];
  char symbol[96];
  uint64_t expected;
  unsigned repeat_count;
  int check_expected;
  int call_kind;
};

struct poly_atexit_callback {
  uint64_t callback;
  uint64_t arg;
  uint64_t dso_handle;
  int active;
};

static struct poly_atexit_callback poly_atexit_callbacks[MAX_ATEXIT_CALLBACKS];
static size_t poly_atexit_callback_count;
static uint8_t *poly_atexit_call_code;
static size_t poly_atexit_target_imm_offset;

void poly_runtime_reset_atexit_callbacks(void)
{
  memset(poly_atexit_callbacks, 0, sizeof(poly_atexit_callbacks));
  poly_atexit_callback_count = 0;
  poly_atexit_call_code = NULL;
  poly_atexit_target_imm_offset = 0;
}

uint64_t poly_runtime_register_atexit_callback(void *callback, void *arg,
    void *dso_handle)
{
  if (callback == NULL || poly_atexit_callback_count >= MAX_ATEXIT_CALLBACKS)
    return 1;

  struct poly_atexit_callback *entry =
    &poly_atexit_callbacks[poly_atexit_callback_count++];
  entry->callback = (uint64_t) (uintptr_t) callback;
  entry->arg = (uint64_t) (uintptr_t) arg;
  entry->dso_handle = (uint64_t) (uintptr_t) dso_handle;
  entry->active = 1;
  return 0;
}

void poly_runtime_set_atexit_call_stub(uint8_t *code, size_t target_imm_offset)
{
  poly_atexit_call_code = code;
  poly_atexit_target_imm_offset = target_imm_offset;
}

static struct poly_cpuid_regs read_cpuid(uint32_t leaf, uint32_t subleaf) {
  struct poly_cpuid_regs regs;
  asm volatile("cpuid"
      : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx), "=d"(regs.edx)
      : "a"(leaf), "c"(subleaf)
      : "memory");
  return regs;
}

static int poly_cpuid_vendor_matches(const struct poly_cpuid_regs *regs) {
  return regs->ebx == 0x796c6f50U &&
    regs->edx == 0x746f6c67U &&
    regs->ecx == 0x21555043U;
}

static int read_poly_base_contract(void) {
  const struct poly_cpuid_regs base = read_cpuid(POLY_CPUID_BASE, 0);
  if (base.eax < POLY_CPUID_MAX || !poly_cpuid_vendor_matches(&base)) {
    fprintf(stderr,
      "POLYCALL_FAIL: poly CPUID missing base=(0x%x,0x%x,0x%x,0x%x)\n",
      base.eax, base.ebx, base.ecx, base.edx);
    return -1;
  }

  const struct poly_cpuid_regs features = read_cpuid(POLY_CPUID_BASE + 1, 0);
  if (features.eax != POLY_CPUID_ABI_VERSION ||
      (features.ebx & POLY_CPUID_REQUIRED_MODES) !=
        POLY_CPUID_REQUIRED_MODES ||
      (features.ecx & POLY_CPUID_REQUIRED_FEATURES) !=
        POLY_CPUID_REQUIRED_FEATURES) {
    fprintf(stderr,
      "POLYCALL_FAIL: poly CPUID feature mismatch features=(%u,0x%x,0x%x,0x%x)\n",
      features.eax, features.ebx, features.ecx, features.edx);
    return -1;
  }

  return 0;
}

static int read_poly_abi_bridge_contract(struct poly_import_contract *contract) {
  const struct poly_cpuid_regs abi_bridge =
    read_cpuid(POLY_CPUID_BASE + 9, 0);
  const uint32_t abi_gpr_arg_count = abi_bridge.ecx & 0xffU;
  const uint32_t abi_fp_arg_count = (abi_bridge.ecx >> 8) & 0xffU;
  const uint32_t abi_stack_align = (abi_bridge.ecx >> 16) & 0xffffU;
  const uint32_t abi_descriptor_size = abi_bridge.edx & 0xffffU;
  const uint32_t abi_call_stride = (abi_bridge.edx >> 16) & 0xffffU;

  if (abi_bridge.eax != POLY_ABI_BRIDGE_ABI_VERSION ||
      (abi_bridge.ebx & POLY_ABI_BRIDGE_REQUIRED_FLAGS) !=
        POLY_ABI_BRIDGE_REQUIRED_FLAGS ||
      abi_gpr_arg_count != POLY_ABI_BRIDGE_GPR_ARG_COUNT ||
      abi_fp_arg_count != POLY_ABI_BRIDGE_FP_ARG_COUNT ||
      abi_stack_align != POLY_ABI_BRIDGE_STACK_ALIGN ||
      abi_descriptor_size != POLY_X86_IMPORT_DESCRIPTOR_SIZE ||
      abi_call_stride != POLY_IMPORT_CALL_STRIDE) {
    fprintf(stderr,
      "POLYCALL_FAIL: CPU ABI bridge mismatch abi=(%u,0x%x,0x%x,0x%x)\n",
      abi_bridge.eax, abi_bridge.ebx, abi_bridge.ecx, abi_bridge.edx);
    return -1;
  }

  contract->abi_flags = abi_bridge.ebx;
  contract->gpr_arg_count = abi_gpr_arg_count;
  contract->fp_arg_count = abi_fp_arg_count;
  contract->stack_align = abi_stack_align;
  return 0;
}

static int read_poly_import_contract(struct poly_import_contract *contract) {
  const struct poly_cpuid_regs descriptor =
    read_cpuid(POLY_CPUID_BASE + 2, 2);
  const struct poly_cpuid_regs manifest =
    read_cpuid(POLY_CPUID_BASE + 2, 5);
  const uint64_t call_base =
    ((uint64_t) manifest.ecx << 32) | manifest.ebx;

  if (descriptor.eax != POLY_IMPORT_FUNC_X86_SLOT0 ||
      descriptor.ebx !=
        POLY_IMPORT_FUNC_X86_SLOT7 - POLY_IMPORT_FUNC_X86_SLOT0 + 1 ||
      descriptor.ecx != POLY_X86_IMPORT_DESCRIPTOR_SIZE ||
      descriptor.edx != POLY_IMPORT_CALL_STRIDE ||
      manifest.eax != POLY_IMPORT_FUNC_COUNT ||
      call_base != POLY_IMPORT_CALL_BASE ||
      manifest.edx != POLY_IMPORT_CALL_STRIDE) {
    fprintf(stderr,
      "POLYCALL_FAIL: CPU import ABI mismatch desc=(%u,%u,%u,%u) manifest=(%u,0x%016llx,%u)\n",
      descriptor.eax, descriptor.ebx, descriptor.ecx, descriptor.edx,
      manifest.eax, (unsigned long long) call_base, manifest.edx);
      return -1;
  }

  contract->call_base = call_base;
  contract->call_stride = manifest.edx;
  contract->import_count = manifest.eax;
  contract->x86_slot0 = descriptor.eax;
  contract->x86_slot_count = descriptor.ebx;
  contract->x86_descriptor_size = descriptor.ecx;
  return 0;
}

extern uint64_t poly_host_x86_add(uint64_t a, uint64_t b);
extern uint64_t poly_host_x86_mul(uint64_t a, uint64_t b);
extern uint64_t poly_host_import_add(uint64_t a, uint64_t b);
extern uint64_t poly_host_import_mul(uint64_t a, uint64_t b);
extern double poly_host_import_fp64_add(double a, double b);
extern float poly_host_import_fp32_add(float a, float b);
extern uint64_t poly_host_x86_aarch64_tlsdesc(uint64_t descriptor);
extern uint64_t poly_host_x86_riscv_tls_get_addr(uint64_t descriptor);
extern uint64_t poly_host_x86_sum6(uint64_t a, uint64_t b, uint64_t c,
    uint64_t d, uint64_t e, uint64_t f);
extern uint64_t poly_host_x86_sum8(uint64_t a, uint64_t b, uint64_t c,
    uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h);
extern uint64_t poly_host_x86_sum10(uint64_t a, uint64_t b, uint64_t c,
    uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h, uint64_t i,
    uint64_t j);
extern uint64_t poly_host_x86_sum14(uint64_t a, uint64_t b, uint64_t c,
    uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h, uint64_t i,
    uint64_t j, uint64_t k, uint64_t l, uint64_t m, uint64_t n);
extern uint64_t poly_host_x86_align14(uint64_t a, uint64_t b, uint64_t c,
    uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h, uint64_t i,
    uint64_t j, uint64_t k, uint64_t l, uint64_t m, uint64_t n);
extern unsigned __int128 poly_host_x86_i128(uint64_t a, uint64_t b);
extern double poly_host_x86_fp64_add(double a, double b);
extern double poly_host_x86_fp64_sum8(double a, double b, double c,
    double d, double e, double f, double g, double h);
extern double poly_host_x86_fp64_sum10(double a, double b, double c,
    double d, double e, double f, double g, double h, double i, double j);
struct poly_host_pair_fp64 {
  double lo;
  double hi;
};
extern struct poly_host_pair_fp64 poly_host_x86_fpair64(double a, double b,
    double c);
struct poly_host_pair_fp32 {
  float lo;
  float hi;
};
extern struct poly_host_pair_fp32 poly_host_x86_fpair32(float a, float b,
    float c);
typedef uint32_t poly_host_vec128_u32 __attribute__((vector_size(16)));
extern poly_host_vec128_u32 poly_host_x86_vec128_u32(
    poly_host_vec128_u32 a, poly_host_vec128_u32 b);
struct poly_host_sret_u64 {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t d;
};
extern struct poly_host_sret_u64 poly_host_x86_sret_u64(uint64_t a,
    uint64_t b, uint64_t c);
extern struct poly_host_sret_u64 poly_host_x86_sret_u64_stack(uint64_t a,
    uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
extern struct poly_host_sret_u64 poly_host_x86_sret_u64_stack10(uint64_t a,
    uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g,
    uint64_t h, uint64_t i, uint64_t j);
extern double poly_host_x86_mixed_u64_fp64(uint64_t a, double b, uint64_t c,
    double d, uint64_t e, double f);
extern float poly_host_x86_fp32_add(float a, float b);
extern uint64_t poly_host_x86_strlen(const char *text);
extern uint64_t poly_host_x86_strcmp(const unsigned char *left_text,
    const unsigned char *right_text);
extern uint64_t poly_host_x86_strncmp(const unsigned char *left_text,
    const unsigned char *right_text, uint64_t max_len);
extern uint64_t poly_host_x86_strcasecmp(const unsigned char *left_text,
    const unsigned char *right_text);
extern uint64_t poly_host_x86_strncasecmp(const unsigned char *left_text,
    const unsigned char *right_text, uint64_t max_len);
extern uint64_t poly_host_x86_strcasestr(const uint8_t *haystack,
    const uint8_t *needle_text);
extern uint64_t poly_host_x86_memcpy(uint8_t *dest, const uint8_t *src,
    uint64_t size);
extern uint64_t poly_host_x86_memmove(uint8_t *dest, const uint8_t *src,
    uint64_t size);
extern uint64_t poly_host_x86_memset(uint8_t *dest, uint64_t value,
    uint64_t size);
extern uint64_t poly_host_x86_memcmp(const unsigned char *left_text,
    const unsigned char *right_text, uint64_t size);
extern uint64_t poly_host_x86_memchr(const uint8_t *text, uint64_t needle,
    uint64_t size);
extern uint64_t poly_host_x86_memrchr(const uint8_t *text, uint64_t needle,
    uint64_t size);
extern uint64_t poly_host_x86_memmem(const uint8_t *haystack,
    uint64_t haystack_size, const uint8_t *needle, uint64_t needle_size);
extern uint64_t poly_host_x86_strchr(const uint8_t *text, uint64_t needle);
extern uint64_t poly_host_x86_strrchr(const uint8_t *text, uint64_t needle);
extern uint64_t poly_host_x86_strstr(const uint8_t *haystack,
    const uint8_t *needle_text);
extern uint64_t poly_host_x86_strcpy(uint8_t *dest, const uint8_t *src);
extern uint64_t poly_host_x86_strncpy(uint8_t *dest, const uint8_t *src,
    uint64_t max_len);
extern uint64_t poly_host_x86_strnlen(const char *text, uint64_t max_len);
extern uint64_t poly_host_x86_strcat(uint8_t *dest, const uint8_t *src);
extern uint64_t poly_host_x86_strncat(uint8_t *dest, const uint8_t *src,
    uint64_t max_len);
extern uint64_t poly_host_x86_strspn(const uint8_t *text,
    const uint8_t *accept_text);
extern uint64_t poly_host_x86_strcspn(const uint8_t *text,
    const uint8_t *reject_text);
extern uint64_t poly_host_x86_strpbrk(const uint8_t *text,
    const uint8_t *accept_text);
extern uint64_t poly_host_x86_stpcpy(uint8_t *dest, const uint8_t *src);
extern uint64_t poly_host_x86_stpncpy(uint8_t *dest, const uint8_t *src,
    uint64_t max_len);
extern uint64_t poly_host_x86_mempcpy(uint8_t *dest, const uint8_t *src,
    uint64_t size);
extern uint64_t poly_host_x86_rawmemchr(const uint8_t *text, uint64_t needle);
extern uint64_t poly_host_x86_strchrnul(const uint8_t *text, uint64_t needle);
extern uint64_t poly_host_x86_bcopy(const uint8_t *src, uint8_t *dest,
    uint64_t size);
extern uint64_t poly_host_x86_bzero(uint8_t *dest, uint64_t size);
extern uint64_t poly_host_x86_stack_chk_fail(void);
extern uint64_t poly_host_x86_errno_location(void);
extern uint64_t poly_host_x86_getauxval(uint64_t type);
extern uint64_t poly_host_x86_getpagesize(void);
extern uint64_t poly_host_x86_sysconf(uint64_t name);
extern uint64_t poly_host_x86_getenv(const char *name);
extern uint64_t poly_host_x86_malloc(uint64_t size);
extern uint64_t poly_host_x86_calloc(uint64_t count, uint64_t size);
extern uint64_t poly_host_x86_realloc(uint8_t *old_ptr, uint64_t size);
extern uint64_t poly_host_x86_free(void *ptr);
extern uint64_t poly_host_x86_strdup(const uint8_t *src);
extern uint64_t poly_host_x86_strndup(const uint8_t *src, uint64_t max_len);
extern uint64_t poly_host_x86_posix_memalign(uint64_t *out,
    uint64_t alignment, uint64_t size);
extern uint64_t poly_host_x86_aligned_alloc(uint64_t alignment, uint64_t size);
extern uint64_t poly_host_x86_memalign(uint64_t alignment, uint64_t size);
extern uint64_t poly_host_x86_qsort(void *base, uint64_t nmemb,
    uint64_t size, void *compar);
extern uint64_t poly_host_x86_bsearch(const void *key, const void *base,
    uint64_t nmemb, uint64_t size, void *compar);
extern uint64_t poly_host_x86_qsort_r(void *base, uint64_t nmemb,
    uint64_t size, void *compar, void *arg);
extern uint64_t poly_host_x86_pthread_once(uint32_t *once_control,
    void *init_routine);
extern uint64_t poly_host_x86_pthread_key_create(uint32_t *key,
    void *destructor);
extern uint64_t poly_host_x86_pthread_key_delete(uint32_t key);
extern uint64_t poly_host_x86_pthread_getspecific(uint32_t key);
extern uint64_t poly_host_x86_pthread_setspecific(uint32_t key,
    const void *value);
extern uint64_t poly_host_x86_pthread_mutex_init(uint32_t *mutex,
    const void *attr);
extern uint64_t poly_host_x86_pthread_mutex_destroy(uint32_t *mutex);
extern uint64_t poly_host_x86_pthread_mutex_lock(uint32_t *mutex);
extern uint64_t poly_host_x86_pthread_mutex_trylock(uint32_t *mutex);
extern uint64_t poly_host_x86_pthread_mutex_unlock(uint32_t *mutex);
extern uint64_t poly_host_x86_pthread_self(void);
extern uint64_t poly_host_x86_pthread_equal(uint64_t left, uint64_t right);
extern uint64_t poly_host_x86_pthread_rwlock_init(uint32_t *lock,
    const void *attr);
extern uint64_t poly_host_x86_pthread_rwlock_destroy(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_rwlock_rdlock(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_rwlock_tryrdlock(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_rwlock_wrlock(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_rwlock_trywrlock(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_rwlock_unlock(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_mutexattr_init(uint32_t *attr);
extern uint64_t poly_host_x86_pthread_mutexattr_destroy(uint32_t *attr);
extern uint64_t poly_host_x86_pthread_mutexattr_settype(uint32_t *attr,
    uint32_t type);
extern uint64_t poly_host_x86_pthread_mutexattr_gettype(const uint32_t *attr,
    uint32_t *type);
extern uint64_t poly_host_x86_pthread_spin_init(uint32_t *lock,
    uint32_t pshared);
extern uint64_t poly_host_x86_pthread_spin_destroy(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_spin_lock(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_spin_trylock(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_spin_unlock(uint32_t *lock);
extern uint64_t poly_host_x86_pthread_cond_init(uint32_t *cond,
    const void *attr);
extern uint64_t poly_host_x86_pthread_cond_destroy(uint32_t *cond);
extern uint64_t poly_host_x86_pthread_cond_signal(uint32_t *cond);
extern uint64_t poly_host_x86_pthread_cond_broadcast(uint32_t *cond);
extern uint64_t poly_host_x86_clock_gettime(uint64_t clock_id, int64_t *tp);
extern uint64_t poly_host_x86_clock_getres(uint64_t clock_id, int64_t *tp);
extern uint64_t poly_host_x86_time(int64_t *out);
extern uint64_t poly_host_x86_gettimeofday(int64_t *tv, void *tz);
extern uint64_t poly_host_x86_clock(void);
extern uint64_t poly_host_x86_atexit(void *callback);
extern uint64_t poly_host_x86_cxa_atexit(void *callback, void *arg,
    void *dso_handle);
extern uint64_t poly_host_x86_cxa_finalize(void *dso_handle);
extern uint64_t poly_host_x86_cxa_guard_acquire(uint64_t *guard);
extern uint64_t poly_host_x86_cxa_guard_release(uint64_t *guard);
extern uint64_t poly_host_x86_cxa_guard_abort(uint64_t *guard);
extern uint64_t poly_host_x86_getpid(void);
extern uint64_t poly_host_x86_getppid(void);
extern uint64_t poly_host_x86_getuid(void);
extern uint64_t poly_host_x86_gettid(void);
extern uint64_t poly_host_x86_puts(const uint8_t *text);
extern unsigned __int128 poly_host_x86_udivti3(
    unsigned __int128 dividend, unsigned __int128 divisor);
extern unsigned __int128 poly_host_x86_umodti3(
    unsigned __int128 dividend, unsigned __int128 divisor);
extern __int128 poly_host_x86_divti3(__int128 dividend, __int128 divisor);
extern __int128 poly_host_x86_modti3(__int128 dividend, __int128 divisor);
extern __int128 poly_host_x86_fixdfti(double source);
extern unsigned __int128 poly_host_x86_fixunsdfti(double source);
extern double poly_host_x86_floattidf(__int128 source);
extern double poly_host_x86_floatuntidf(unsigned __int128 source);
extern __int128 poly_host_x86_fixsfti(float source);
extern unsigned __int128 poly_host_x86_fixunssfti(float source);
extern float poly_host_x86_floattisf(__int128 source);
extern float poly_host_x86_floatuntisf(unsigned __int128 source);
extern uint64_t poly_host_x86_atomic_compare_exchange_16(uint64_t *ptr,
    uint64_t *expected, uint64_t desired_lo, uint64_t desired_hi,
    uint64_t weak, uint64_t success_order, uint64_t failure_order);
extern unsigned __int128 poly_host_x86_atomic_load_16(uint64_t *ptr,
    uint64_t order);
extern uint64_t poly_host_x86_atomic_store_16(uint64_t *ptr,
    uint64_t value_lo, uint64_t value_hi, uint64_t order);
extern uint64_t poly_host_x86_aarch64_atomic_store_16(uint64_t *ptr,
    uint64_t unused_aapcs64_x1, uint64_t value_lo, uint64_t value_hi,
    uint64_t order);
extern uint64_t poly_host_x86_aarch64_ldadd1(uint64_t source, uint8_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldadd2(uint64_t source, uint16_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldadd4(uint64_t source, uint32_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldadd8(uint64_t source, uint64_t *ptr);
extern uint64_t poly_host_x86_aarch64_swp1(uint64_t source, uint8_t *ptr);
extern uint64_t poly_host_x86_aarch64_swp2(uint64_t source, uint16_t *ptr);
extern uint64_t poly_host_x86_aarch64_swp4(uint64_t source, uint32_t *ptr);
extern uint64_t poly_host_x86_aarch64_swp8(uint64_t source, uint64_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldclr1(uint64_t source, uint8_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldclr2(uint64_t source, uint16_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldclr4(uint64_t source, uint32_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldclr8(uint64_t source, uint64_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldeor1(uint64_t source, uint8_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldeor2(uint64_t source, uint16_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldeor4(uint64_t source, uint32_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldeor8(uint64_t source, uint64_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldset1(uint64_t source, uint8_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldset2(uint64_t source, uint16_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldset4(uint64_t source, uint32_t *ptr);
extern uint64_t poly_host_x86_aarch64_ldset8(uint64_t source, uint64_t *ptr);
extern uint64_t poly_host_x86_aarch64_cas1(uint64_t expected_value,
    uint64_t desired, uint8_t *ptr);
extern uint64_t poly_host_x86_aarch64_cas2(uint64_t expected_value,
    uint64_t desired, uint16_t *ptr);
extern uint64_t poly_host_x86_aarch64_cas4(uint64_t expected_value,
    uint64_t desired, uint32_t *ptr);
extern uint64_t poly_host_x86_aarch64_cas8(uint64_t expected_value,
    uint64_t desired, uint64_t *ptr);
extern uint64_t poly_host_x86_clzdi2(uint64_t value);
extern uint64_t poly_host_x86_ctzdi2(uint64_t value);
extern uint64_t poly_host_x86_paritydi2(uint64_t value);
extern uint64_t poly_host_x86_popcountdi2(uint64_t value);
extern __float128 poly_host_x86_addtf3(__float128 left, __float128 right);
extern __float128 poly_host_x86_riscv_addtf3(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern __float128 poly_host_x86_subtf3(__float128 left, __float128 right);
extern __float128 poly_host_x86_riscv_subtf3(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern __float128 poly_host_x86_multf3(__float128 left, __float128 right);
extern __float128 poly_host_x86_riscv_multf3(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern __float128 poly_host_x86_divtf3(__float128 left, __float128 right);
extern __float128 poly_host_x86_riscv_divtf3(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern __float128 poly_host_x86_floatunditf(uint64_t source);
extern uint64_t poly_host_x86_fixunstfdi(__float128 source);
extern uint64_t poly_host_x86_riscv_fixunstfdi(uint64_t source_lo,
    uint64_t source_hi);
extern __float128 poly_host_x86_floatditf(int64_t source);
extern __float128 poly_host_x86_floatsitf(int64_t source);
extern __float128 poly_host_x86_floatunsitf(uint64_t source);
extern uint64_t poly_host_x86_fixtfdi(__float128 source);
extern uint64_t poly_host_x86_riscv_fixtfdi(uint64_t source_lo,
    uint64_t source_hi);
extern uint64_t poly_host_x86_fixtfsi(__float128 source);
extern uint64_t poly_host_x86_riscv_fixtfsi(uint64_t source_lo,
    uint64_t source_hi);
extern uint64_t poly_host_x86_fixunstfsi(__float128 source);
extern uint64_t poly_host_x86_riscv_fixunstfsi(uint64_t source_lo,
    uint64_t source_hi);
extern uint64_t poly_host_x86_eqtf2(__float128 left, __float128 right);
extern uint64_t poly_host_x86_riscv_eqtf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern uint64_t poly_host_x86_lttf2(__float128 left, __float128 right);
extern uint64_t poly_host_x86_riscv_lttf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern uint64_t poly_host_x86_letf2(__float128 left, __float128 right);
extern uint64_t poly_host_x86_riscv_letf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern uint64_t poly_host_x86_gttf2(__float128 left, __float128 right);
extern uint64_t poly_host_x86_riscv_gttf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern uint64_t poly_host_x86_getf2(__float128 left, __float128 right);
extern uint64_t poly_host_x86_riscv_getf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern __float128 poly_host_x86_extendsftf2(float source);
extern __float128 poly_host_x86_extenddftf2(double source);
extern float poly_host_x86_trunctfsf2(__float128 source);
extern float poly_host_x86_riscv_trunctfsf2(uint64_t source_lo,
    uint64_t source_hi);
extern double poly_host_x86_trunctfdf2(__float128 source);
extern double poly_host_x86_riscv_trunctfdf2(uint64_t source_lo,
    uint64_t source_hi);
extern uint64_t poly_host_x86_netf2(__float128 left, __float128 right);
extern uint64_t poly_host_x86_riscv_netf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);
extern uint64_t poly_host_x86_unordtf2(__float128 left, __float128 right);
extern uint64_t poly_host_x86_riscv_unordtf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi);

static int resolve_aarch64_outline_atomic_import(const char *symbol_name,
    uint64_t *symbol_value);

static int import_symbol_uses_x86_descriptor(const char *symbol_name) {
  static const char *const names[] = {
    "poly_import_add", "poly_import_mul",
    "poly_import_fp64_add", "poly_import_fp32_add",
    "strlen", "strcmp", "strncmp", "strcasecmp", "strncasecmp",
    "strcasestr", "memcpy", "memmove", "memset", "memcmp", "memchr",
    "memrchr", "memmem", "strchr", "index", "strrchr", "rindex",
    "strstr", "strcpy", "strncpy", "strnlen", "strcat", "strncat",
    "strspn", "strcspn", "strpbrk", "stpcpy", "stpncpy", "mempcpy",
    "rawmemchr", "strchrnul", "bcmp", "bcopy", "bzero",
    "__tls_get_addr",
    "__stack_chk_fail", "__errno_location", "getauxval", "getpagesize",
    "sysconf", "getenv", "secure_getenv", "malloc", "calloc", "realloc",
    "free", "strdup", "strndup", "posix_memalign", "aligned_alloc",
    "memalign", "qsort", "bsearch", "qsort_r", "pthread_once",
    "pthread_key_create", "pthread_key_delete", "pthread_getspecific",
    "pthread_setspecific", "pthread_mutex_init", "pthread_mutex_destroy",
    "pthread_mutex_lock", "pthread_mutex_trylock", "pthread_mutex_unlock",
    "pthread_self", "pthread_equal", "atexit", "__cxa_atexit",
    "__cxa_finalize", "pthread_rwlock_init", "pthread_rwlock_destroy",
    "pthread_rwlock_rdlock", "pthread_rwlock_tryrdlock",
    "pthread_rwlock_wrlock", "pthread_rwlock_trywrlock",
    "pthread_rwlock_unlock", "pthread_mutexattr_init",
    "pthread_mutexattr_destroy", "pthread_mutexattr_settype",
    "pthread_mutexattr_gettype", "pthread_spin_init",
    "pthread_spin_destroy", "pthread_spin_lock", "pthread_spin_trylock",
    "pthread_spin_unlock", "pthread_cond_init", "pthread_cond_destroy",
    "pthread_cond_signal", "pthread_cond_broadcast",
    "clock_gettime", "__clock_gettime64", "clock_getres",
    "__clock_getres_time64", "time", "__time64", "gettimeofday",
    "__gettimeofday", "clock",
    "getpid", "getppid", "getuid", "geteuid",
    "getgid", "getegid", "gettid",
    "puts", "__cxa_guard_acquire", "__cxa_guard_release",
    "__cxa_guard_abort",
    "__udivti3", "__umodti3", "__divti3", "__modti3",
    "__fixdfti", "__fixunsdfti", "__floattidf", "__floatuntidf",
    "__fixsfti", "__fixunssfti", "__floattisf", "__floatuntisf",
    "__atomic_compare_exchange_16", "__atomic_load_16",
    "__atomic_store_16",
    "__clzdi2", "__ctzdi2", "__paritydi2", "__popcountdi2",
    "__addtf3", "__subtf3", "__multf3", "__divtf3",
    "__floatunditf", "__fixunstfdi", "__floatditf", "__floatsitf",
    "__fixtfdi", "__eqtf2", "__lttf2", "__letf2", "__gttf2",
    "__getf2", "__extendsftf2", "__extenddftf2", "__trunctfsf2",
    "__trunctfdf2", "__netf2", "__unordtf2", "__floatunsitf",
    "__fixtfsi", "__fixunstfsi"
  };

  for (size_t n = 0; n < sizeof(names) / sizeof(names[0]); n++) {
    if (strcmp(symbol_name, names[n]) == 0)
      return 1;
  }

  uint64_t ignored = 0;
  if (resolve_aarch64_outline_atomic_import(symbol_name, &ignored) == 0)
    return 1;

  return 0;
}

static uint64_t x86_descriptor_target_for_import_id(int arch,
    uint64_t import_id) {
  switch (import_id) {
    case POLY_IMPORT_FUNC_ADD:
      return (uint64_t) (uintptr_t) poly_host_import_add;
    case POLY_IMPORT_FUNC_MUL:
      return (uint64_t) (uintptr_t) poly_host_import_mul;
    case POLY_IMPORT_FUNC_FP64_ADD:
      return (uint64_t) (uintptr_t) poly_host_import_fp64_add;
    case POLY_IMPORT_FUNC_FP32_ADD:
      return (uint64_t) (uintptr_t) poly_host_import_fp32_add;
    case POLY_IMPORT_FUNC_AARCH64_TLSDESC:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_tlsdesc;
    case POLY_IMPORT_FUNC_RISCV_TLS_GET_ADDR:
      return (uint64_t) (uintptr_t) poly_host_x86_riscv_tls_get_addr;
    case POLY_IMPORT_FUNC_X86_SLOT0:
      return (uint64_t) (uintptr_t) poly_host_x86_add;
    case POLY_IMPORT_FUNC_X86_SLOT1:
      return (uint64_t) (uintptr_t) poly_host_x86_mul;
    case POLY_IMPORT_FUNC_X86_SLOT2:
      return (uint64_t) (uintptr_t) poly_host_x86_sum6;
    case POLY_IMPORT_FUNC_X86_SLOT3:
      return (uint64_t) (uintptr_t) poly_host_x86_fp64_add;
    case POLY_IMPORT_FUNC_X86_SLOT4:
      return (uint64_t) (uintptr_t) poly_host_x86_fp32_add;
    case POLY_IMPORT_FUNC_X86_SLOT5:
      return (uint64_t) (uintptr_t) poly_host_x86_sum8;
    case POLY_IMPORT_FUNC_X86_SUM10:
      return (uint64_t) (uintptr_t) poly_host_x86_sum10;
    case POLY_IMPORT_FUNC_X86_SUM14:
      return (uint64_t) (uintptr_t) poly_host_x86_sum14;
    case POLY_IMPORT_FUNC_X86_ALIGN14:
      return (uint64_t) (uintptr_t) poly_host_x86_align14;
    case POLY_IMPORT_FUNC_X86_I128:
      return (uint64_t) (uintptr_t) poly_host_x86_i128;
    case POLY_IMPORT_FUNC_X86_SLOT6:
      return (uint64_t) (uintptr_t) poly_host_x86_fp64_sum8;
    case POLY_IMPORT_FUNC_X86_FP64_SUM10:
      return (uint64_t) (uintptr_t) poly_host_x86_fp64_sum10;
    case POLY_IMPORT_FUNC_X86_FPAIR64:
      return (uint64_t) (uintptr_t) poly_host_x86_fpair64;
    case POLY_IMPORT_FUNC_X86_FPAIR32:
      return (uint64_t) (uintptr_t) poly_host_x86_fpair32;
    case POLY_IMPORT_FUNC_X86_VEC128_U32:
      return (uint64_t) (uintptr_t) poly_host_x86_vec128_u32;
    case POLY_IMPORT_FUNC_X86_SRET_U64:
      return (uint64_t) (uintptr_t) poly_host_x86_sret_u64;
    case POLY_IMPORT_FUNC_X86_SRET_U64_STACK:
      return (uint64_t) (uintptr_t) poly_host_x86_sret_u64_stack;
    case POLY_IMPORT_FUNC_X86_SRET_U64_STACK10:
      return (uint64_t) (uintptr_t) poly_host_x86_sret_u64_stack10;
    case POLY_IMPORT_FUNC_X86_SLOT7:
      return (uint64_t) (uintptr_t) poly_host_x86_mixed_u64_fp64;
    case POLY_IMPORT_FUNC_STRLEN:
      return (uint64_t) (uintptr_t) poly_host_x86_strlen;
    case POLY_IMPORT_FUNC_STRCMP:
      return (uint64_t) (uintptr_t) poly_host_x86_strcmp;
    case POLY_IMPORT_FUNC_STRNCMP:
      return (uint64_t) (uintptr_t) poly_host_x86_strncmp;
    case POLY_IMPORT_FUNC_STRCASECMP:
      return (uint64_t) (uintptr_t) poly_host_x86_strcasecmp;
    case POLY_IMPORT_FUNC_STRNCASECMP:
      return (uint64_t) (uintptr_t) poly_host_x86_strncasecmp;
    case POLY_IMPORT_FUNC_STRCASESTR:
      return (uint64_t) (uintptr_t) poly_host_x86_strcasestr;
    case POLY_IMPORT_FUNC_MEMCPY:
      return (uint64_t) (uintptr_t) poly_host_x86_memcpy;
    case POLY_IMPORT_FUNC_MEMMOVE:
      return (uint64_t) (uintptr_t) poly_host_x86_memmove;
    case POLY_IMPORT_FUNC_MEMSET:
      return (uint64_t) (uintptr_t) poly_host_x86_memset;
    case POLY_IMPORT_FUNC_MEMCMP:
      return (uint64_t) (uintptr_t) poly_host_x86_memcmp;
    case POLY_IMPORT_FUNC_MEMCHR:
      return (uint64_t) (uintptr_t) poly_host_x86_memchr;
    case POLY_IMPORT_FUNC_MEMRCHR:
      return (uint64_t) (uintptr_t) poly_host_x86_memrchr;
    case POLY_IMPORT_FUNC_MEMMEM:
      return (uint64_t) (uintptr_t) poly_host_x86_memmem;
    case POLY_IMPORT_FUNC_STRCHR:
      return (uint64_t) (uintptr_t) poly_host_x86_strchr;
    case POLY_IMPORT_FUNC_STRRCHR:
      return (uint64_t) (uintptr_t) poly_host_x86_strrchr;
    case POLY_IMPORT_FUNC_STRSTR:
      return (uint64_t) (uintptr_t) poly_host_x86_strstr;
    case POLY_IMPORT_FUNC_STRCPY:
      return (uint64_t) (uintptr_t) poly_host_x86_strcpy;
    case POLY_IMPORT_FUNC_STRNCPY:
      return (uint64_t) (uintptr_t) poly_host_x86_strncpy;
    case POLY_IMPORT_FUNC_STRNLEN:
      return (uint64_t) (uintptr_t) poly_host_x86_strnlen;
    case POLY_IMPORT_FUNC_STRCAT:
      return (uint64_t) (uintptr_t) poly_host_x86_strcat;
    case POLY_IMPORT_FUNC_STRNCAT:
      return (uint64_t) (uintptr_t) poly_host_x86_strncat;
    case POLY_IMPORT_FUNC_STRSPN:
      return (uint64_t) (uintptr_t) poly_host_x86_strspn;
    case POLY_IMPORT_FUNC_STRCSPN:
      return (uint64_t) (uintptr_t) poly_host_x86_strcspn;
    case POLY_IMPORT_FUNC_STRPBRK:
      return (uint64_t) (uintptr_t) poly_host_x86_strpbrk;
    case POLY_IMPORT_FUNC_STPCPY:
      return (uint64_t) (uintptr_t) poly_host_x86_stpcpy;
    case POLY_IMPORT_FUNC_STPNCPY:
      return (uint64_t) (uintptr_t) poly_host_x86_stpncpy;
    case POLY_IMPORT_FUNC_MEMPCPY:
      return (uint64_t) (uintptr_t) poly_host_x86_mempcpy;
    case POLY_IMPORT_FUNC_RAWMEMCHR:
      return (uint64_t) (uintptr_t) poly_host_x86_rawmemchr;
    case POLY_IMPORT_FUNC_STRCHRNUL:
      return (uint64_t) (uintptr_t) poly_host_x86_strchrnul;
    case POLY_IMPORT_FUNC_BCMP:
      return (uint64_t) (uintptr_t) poly_host_x86_memcmp;
    case POLY_IMPORT_FUNC_BCOPY:
      return (uint64_t) (uintptr_t) poly_host_x86_bcopy;
    case POLY_IMPORT_FUNC_BZERO:
      return (uint64_t) (uintptr_t) poly_host_x86_bzero;
    case POLY_IMPORT_FUNC_STACK_CHK_FAIL:
      return (uint64_t) (uintptr_t) poly_host_x86_stack_chk_fail;
    case POLY_IMPORT_FUNC_ERRNO_LOCATION:
      return (uint64_t) (uintptr_t) poly_host_x86_errno_location;
    case POLY_IMPORT_FUNC_GETAUXVAL:
      return (uint64_t) (uintptr_t) poly_host_x86_getauxval;
    case POLY_IMPORT_FUNC_GETPAGESIZE:
      return (uint64_t) (uintptr_t) poly_host_x86_getpagesize;
    case POLY_IMPORT_FUNC_SYSCONF:
      return (uint64_t) (uintptr_t) poly_host_x86_sysconf;
    case POLY_IMPORT_FUNC_GETENV:
    case POLY_IMPORT_FUNC_SECURE_GETENV:
      return (uint64_t) (uintptr_t) poly_host_x86_getenv;
    case POLY_IMPORT_FUNC_MALLOC:
      return (uint64_t) (uintptr_t) poly_host_x86_malloc;
    case POLY_IMPORT_FUNC_CALLOC:
      return (uint64_t) (uintptr_t) poly_host_x86_calloc;
    case POLY_IMPORT_FUNC_REALLOC:
      return (uint64_t) (uintptr_t) poly_host_x86_realloc;
    case POLY_IMPORT_FUNC_FREE:
      return (uint64_t) (uintptr_t) poly_host_x86_free;
    case POLY_IMPORT_FUNC_STRDUP:
      return (uint64_t) (uintptr_t) poly_host_x86_strdup;
    case POLY_IMPORT_FUNC_STRNDUP:
      return (uint64_t) (uintptr_t) poly_host_x86_strndup;
    case POLY_IMPORT_FUNC_POSIX_MEMALIGN:
      return (uint64_t) (uintptr_t) poly_host_x86_posix_memalign;
    case POLY_IMPORT_FUNC_ALIGNED_ALLOC:
      return (uint64_t) (uintptr_t) poly_host_x86_aligned_alloc;
    case POLY_IMPORT_FUNC_MEMALIGN:
      return (uint64_t) (uintptr_t) poly_host_x86_memalign;
    case POLY_IMPORT_FUNC_QSORT:
      return (uint64_t) (uintptr_t) poly_host_x86_qsort;
    case POLY_IMPORT_FUNC_BSEARCH:
      return (uint64_t) (uintptr_t) poly_host_x86_bsearch;
    case POLY_IMPORT_FUNC_QSORT_R:
      return (uint64_t) (uintptr_t) poly_host_x86_qsort_r;
    case POLY_IMPORT_FUNC_PTHREAD_ONCE:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_once;
    case POLY_IMPORT_FUNC_PTHREAD_KEY_CREATE:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_key_create;
    case POLY_IMPORT_FUNC_PTHREAD_KEY_DELETE:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_key_delete;
    case POLY_IMPORT_FUNC_PTHREAD_GETSPECIFIC:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_getspecific;
    case POLY_IMPORT_FUNC_PTHREAD_SETSPECIFIC:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_setspecific;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEX_INIT:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutex_init;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEX_DESTROY:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutex_destroy;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEX_LOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutex_lock;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEX_TRYLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutex_trylock;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEX_UNLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutex_unlock;
    case POLY_IMPORT_FUNC_PTHREAD_SELF:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_self;
    case POLY_IMPORT_FUNC_PTHREAD_EQUAL:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_equal;
    case POLY_IMPORT_FUNC_PTHREAD_RWLOCK_INIT:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_rwlock_init;
    case POLY_IMPORT_FUNC_PTHREAD_RWLOCK_DESTROY:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_rwlock_destroy;
    case POLY_IMPORT_FUNC_PTHREAD_RWLOCK_RDLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_rwlock_rdlock;
    case POLY_IMPORT_FUNC_PTHREAD_RWLOCK_TRYRDLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_rwlock_tryrdlock;
    case POLY_IMPORT_FUNC_PTHREAD_RWLOCK_WRLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_rwlock_wrlock;
    case POLY_IMPORT_FUNC_PTHREAD_RWLOCK_TRYWRLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_rwlock_trywrlock;
    case POLY_IMPORT_FUNC_PTHREAD_RWLOCK_UNLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_rwlock_unlock;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_INIT:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutexattr_init;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_DESTROY:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutexattr_destroy;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_SETTYPE:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutexattr_settype;
    case POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_GETTYPE:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_mutexattr_gettype;
    case POLY_IMPORT_FUNC_PTHREAD_SPIN_INIT:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_spin_init;
    case POLY_IMPORT_FUNC_PTHREAD_SPIN_DESTROY:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_spin_destroy;
    case POLY_IMPORT_FUNC_PTHREAD_SPIN_LOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_spin_lock;
    case POLY_IMPORT_FUNC_PTHREAD_SPIN_TRYLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_spin_trylock;
    case POLY_IMPORT_FUNC_PTHREAD_SPIN_UNLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_spin_unlock;
    case POLY_IMPORT_FUNC_PTHREAD_COND_INIT:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_cond_init;
    case POLY_IMPORT_FUNC_PTHREAD_COND_DESTROY:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_cond_destroy;
    case POLY_IMPORT_FUNC_PTHREAD_COND_SIGNAL:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_cond_signal;
    case POLY_IMPORT_FUNC_PTHREAD_COND_BROADCAST:
      return (uint64_t) (uintptr_t) poly_host_x86_pthread_cond_broadcast;
    case POLY_IMPORT_FUNC_CLOCK_GETTIME:
      return (uint64_t) (uintptr_t) poly_host_x86_clock_gettime;
    case POLY_IMPORT_FUNC_CLOCK_GETRES:
      return (uint64_t) (uintptr_t) poly_host_x86_clock_getres;
    case POLY_IMPORT_FUNC_TIME:
      return (uint64_t) (uintptr_t) poly_host_x86_time;
    case POLY_IMPORT_FUNC_GETTIMEOFDAY:
      return (uint64_t) (uintptr_t) poly_host_x86_gettimeofday;
    case POLY_IMPORT_FUNC_CLOCK:
      return (uint64_t) (uintptr_t) poly_host_x86_clock;
    case POLY_IMPORT_FUNC_ATEXIT:
      return (uint64_t) (uintptr_t) poly_host_x86_atexit;
    case POLY_IMPORT_FUNC_CXA_ATEXIT:
      return (uint64_t) (uintptr_t) poly_host_x86_cxa_atexit;
    case POLY_IMPORT_FUNC_CXA_FINALIZE:
      return (uint64_t) (uintptr_t) poly_host_x86_cxa_finalize;
    case POLY_IMPORT_FUNC_CXA_GUARD_ACQUIRE:
      return (uint64_t) (uintptr_t) poly_host_x86_cxa_guard_acquire;
    case POLY_IMPORT_FUNC_CXA_GUARD_RELEASE:
      return (uint64_t) (uintptr_t) poly_host_x86_cxa_guard_release;
    case POLY_IMPORT_FUNC_CXA_GUARD_ABORT:
      return (uint64_t) (uintptr_t) poly_host_x86_cxa_guard_abort;
    case POLY_IMPORT_FUNC_GETPID:
      return (uint64_t) (uintptr_t) poly_host_x86_getpid;
    case POLY_IMPORT_FUNC_GETPPID:
      return (uint64_t) (uintptr_t) poly_host_x86_getppid;
    case POLY_IMPORT_FUNC_GETUID:
    case POLY_IMPORT_FUNC_GETEUID:
    case POLY_IMPORT_FUNC_GETGID:
    case POLY_IMPORT_FUNC_GETEGID:
      return (uint64_t) (uintptr_t) poly_host_x86_getuid;
    case POLY_IMPORT_FUNC_GETTID:
      return (uint64_t) (uintptr_t) poly_host_x86_gettid;
    case POLY_IMPORT_FUNC_PUTS:
      return (uint64_t) (uintptr_t) poly_host_x86_puts;
    case POLY_IMPORT_FUNC_UDIVTI3:
      return (uint64_t) (uintptr_t) poly_host_x86_udivti3;
    case POLY_IMPORT_FUNC_UMODTI3:
      return (uint64_t) (uintptr_t) poly_host_x86_umodti3;
    case POLY_IMPORT_FUNC_DIVTI3:
      return (uint64_t) (uintptr_t) poly_host_x86_divti3;
    case POLY_IMPORT_FUNC_MODTI3:
      return (uint64_t) (uintptr_t) poly_host_x86_modti3;
    case POLY_IMPORT_FUNC_FIXDFTI:
      return (uint64_t) (uintptr_t) poly_host_x86_fixdfti;
    case POLY_IMPORT_FUNC_FIXUNSDFTI:
      return (uint64_t) (uintptr_t) poly_host_x86_fixunsdfti;
    case POLY_IMPORT_FUNC_FLOATTIDF:
      return (uint64_t) (uintptr_t) poly_host_x86_floattidf;
    case POLY_IMPORT_FUNC_FLOATUNTIDF:
      return (uint64_t) (uintptr_t) poly_host_x86_floatuntidf;
    case POLY_IMPORT_FUNC_FIXSFTI:
      return (uint64_t) (uintptr_t) poly_host_x86_fixsfti;
    case POLY_IMPORT_FUNC_FIXUNSSFTI:
      return (uint64_t) (uintptr_t) poly_host_x86_fixunssfti;
    case POLY_IMPORT_FUNC_FLOATTISF:
      return (uint64_t) (uintptr_t) poly_host_x86_floattisf;
    case POLY_IMPORT_FUNC_FLOATUNTISF:
      return (uint64_t) (uintptr_t) poly_host_x86_floatuntisf;
    case POLY_IMPORT_FUNC_ATOMIC_COMPARE_EXCHANGE_16:
      return (uint64_t) (uintptr_t) poly_host_x86_atomic_compare_exchange_16;
    case POLY_IMPORT_FUNC_ATOMIC_LOAD_16:
      return (uint64_t) (uintptr_t) poly_host_x86_atomic_load_16;
    case POLY_IMPORT_FUNC_ATOMIC_STORE_16:
      if (arch == POLY_ARCH_AARCH64)
        return (uint64_t) (uintptr_t)
          poly_host_x86_aarch64_atomic_store_16;
      return (uint64_t) (uintptr_t) poly_host_x86_atomic_store_16;
    case POLY_IMPORT_FUNC_AARCH64_LDADD1_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldadd1;
    case POLY_IMPORT_FUNC_AARCH64_LDADD2_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldadd2;
    case POLY_IMPORT_FUNC_AARCH64_LDADD4_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldadd4;
    case POLY_IMPORT_FUNC_AARCH64_LDADD8_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldadd8;
    case POLY_IMPORT_FUNC_AARCH64_SWP1_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_swp1;
    case POLY_IMPORT_FUNC_AARCH64_SWP2_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_swp2;
    case POLY_IMPORT_FUNC_AARCH64_SWP4_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_swp4;
    case POLY_IMPORT_FUNC_AARCH64_SWP8_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_swp8;
    case POLY_IMPORT_FUNC_AARCH64_LDCLR1_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldclr1;
    case POLY_IMPORT_FUNC_AARCH64_LDCLR2_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldclr2;
    case POLY_IMPORT_FUNC_AARCH64_LDCLR4_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldclr4;
    case POLY_IMPORT_FUNC_AARCH64_LDCLR8_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldclr8;
    case POLY_IMPORT_FUNC_AARCH64_LDEOR1_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldeor1;
    case POLY_IMPORT_FUNC_AARCH64_LDEOR2_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldeor2;
    case POLY_IMPORT_FUNC_AARCH64_LDEOR4_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldeor4;
    case POLY_IMPORT_FUNC_AARCH64_LDEOR8_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldeor8;
    case POLY_IMPORT_FUNC_AARCH64_LDSET1_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldset1;
    case POLY_IMPORT_FUNC_AARCH64_LDSET2_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldset2;
    case POLY_IMPORT_FUNC_AARCH64_LDSET4_RELAX:
    case POLY_IMPORT_FUNC_AARCH64_LDSET4_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldset4;
    case POLY_IMPORT_FUNC_AARCH64_LDSET8_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_ldset8;
    case POLY_IMPORT_FUNC_AARCH64_CAS1_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_cas1;
    case POLY_IMPORT_FUNC_AARCH64_CAS2_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_cas2;
    case POLY_IMPORT_FUNC_AARCH64_CAS4_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_cas4;
    case POLY_IMPORT_FUNC_AARCH64_CAS8_ACQ_REL:
      return (uint64_t) (uintptr_t) poly_host_x86_aarch64_cas8;
    case POLY_IMPORT_FUNC_CLZDI2:
      return (uint64_t) (uintptr_t) poly_host_x86_clzdi2;
    case POLY_IMPORT_FUNC_CTZDI2:
      return (uint64_t) (uintptr_t) poly_host_x86_ctzdi2;
    case POLY_IMPORT_FUNC_PARITYDI2:
      return (uint64_t) (uintptr_t) poly_host_x86_paritydi2;
    case POLY_IMPORT_FUNC_POPCOUNTDI2:
      return (uint64_t) (uintptr_t) poly_host_x86_popcountdi2;
    case POLY_IMPORT_FUNC_ADDTF3:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_addtf3;
      return (uint64_t) (uintptr_t) poly_host_x86_addtf3;
    case POLY_IMPORT_FUNC_SUBTF3:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_subtf3;
      return (uint64_t) (uintptr_t) poly_host_x86_subtf3;
    case POLY_IMPORT_FUNC_MULTF3:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_multf3;
      return (uint64_t) (uintptr_t) poly_host_x86_multf3;
    case POLY_IMPORT_FUNC_DIVTF3:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_divtf3;
      return (uint64_t) (uintptr_t) poly_host_x86_divtf3;
    case POLY_IMPORT_FUNC_FLOATUNDITF:
      return (uint64_t) (uintptr_t) poly_host_x86_floatunditf;
    case POLY_IMPORT_FUNC_FIXUNSTFDI:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_fixunstfdi;
      return (uint64_t) (uintptr_t) poly_host_x86_fixunstfdi;
    case POLY_IMPORT_FUNC_FLOATDITF:
      return (uint64_t) (uintptr_t) poly_host_x86_floatditf;
    case POLY_IMPORT_FUNC_FLOATSITF:
      return (uint64_t) (uintptr_t) poly_host_x86_floatsitf;
    case POLY_IMPORT_FUNC_FIXTFDI:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_fixtfdi;
      return (uint64_t) (uintptr_t) poly_host_x86_fixtfdi;
    case POLY_IMPORT_FUNC_EQTF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_eqtf2;
      return (uint64_t) (uintptr_t) poly_host_x86_eqtf2;
    case POLY_IMPORT_FUNC_LTTF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_lttf2;
      return (uint64_t) (uintptr_t) poly_host_x86_lttf2;
    case POLY_IMPORT_FUNC_LETF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_letf2;
      return (uint64_t) (uintptr_t) poly_host_x86_letf2;
    case POLY_IMPORT_FUNC_GTTF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_gttf2;
      return (uint64_t) (uintptr_t) poly_host_x86_gttf2;
    case POLY_IMPORT_FUNC_GETF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_getf2;
      return (uint64_t) (uintptr_t) poly_host_x86_getf2;
    case POLY_IMPORT_FUNC_EXTENDSFTF2:
      return (uint64_t) (uintptr_t) poly_host_x86_extendsftf2;
    case POLY_IMPORT_FUNC_EXTENDDFTF2:
      return (uint64_t) (uintptr_t) poly_host_x86_extenddftf2;
    case POLY_IMPORT_FUNC_TRUNCTFSF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_trunctfsf2;
      return (uint64_t) (uintptr_t) poly_host_x86_trunctfsf2;
    case POLY_IMPORT_FUNC_TRUNCTFDF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_trunctfdf2;
      return (uint64_t) (uintptr_t) poly_host_x86_trunctfdf2;
    case POLY_IMPORT_FUNC_NETF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_netf2;
      return (uint64_t) (uintptr_t) poly_host_x86_netf2;
    case POLY_IMPORT_FUNC_UNORDTF2:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_unordtf2;
      return (uint64_t) (uintptr_t) poly_host_x86_unordtf2;
    case POLY_IMPORT_FUNC_FLOATUNSITF:
      return (uint64_t) (uintptr_t) poly_host_x86_floatunsitf;
    case POLY_IMPORT_FUNC_FIXTFSI:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_fixtfsi;
      return (uint64_t) (uintptr_t) poly_host_x86_fixtfsi;
    case POLY_IMPORT_FUNC_FIXUNSTFSI:
      if (arch == POLY_ARCH_RISCV)
        return (uint64_t) (uintptr_t) poly_host_x86_riscv_fixunstfsi;
      return (uint64_t) (uintptr_t) poly_host_x86_fixunstfsi;
    default:
      return 0;
  }
}

static uint64_t x86_descriptor_flags_for_import_id(int arch,
    uint64_t import_id) {
  switch (import_id) {
    case POLY_IMPORT_FUNC_X86_SLOT5:
    case POLY_IMPORT_FUNC_ATOMIC_COMPARE_EXCHANGE_16:
    case POLY_IMPORT_FUNC_X86_SUM10:
    case POLY_IMPORT_FUNC_X86_SUM14:
    case POLY_IMPORT_FUNC_X86_ALIGN14:
      return POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS;
    case POLY_IMPORT_FUNC_X86_FP64_SUM10:
      if (arch == POLY_ARCH_RISCV)
        return POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS |
          POLY_IMPORT_X86_DESCRIPTOR_STACK_FROM_GPR0;
      return POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS |
        POLY_IMPORT_X86_DESCRIPTOR_STACK_FROM_MEMORY;
    case POLY_IMPORT_FUNC_UDIVTI3:
    case POLY_IMPORT_FUNC_UMODTI3:
    case POLY_IMPORT_FUNC_DIVTI3:
    case POLY_IMPORT_FUNC_MODTI3:
    case POLY_IMPORT_FUNC_FIXDFTI:
    case POLY_IMPORT_FUNC_FIXUNSDFTI:
    case POLY_IMPORT_FUNC_FIXSFTI:
    case POLY_IMPORT_FUNC_FIXUNSSFTI:
    case POLY_IMPORT_FUNC_ATOMIC_LOAD_16:
    case POLY_IMPORT_FUNC_X86_I128:
      return POLY_IMPORT_X86_DESCRIPTOR_RETURN_I128;
    case POLY_IMPORT_FUNC_ADDTF3:
    case POLY_IMPORT_FUNC_SUBTF3:
    case POLY_IMPORT_FUNC_MULTF3:
    case POLY_IMPORT_FUNC_DIVTF3:
    case POLY_IMPORT_FUNC_FLOATUNDITF:
    case POLY_IMPORT_FUNC_FLOATDITF:
    case POLY_IMPORT_FUNC_FLOATSITF:
    case POLY_IMPORT_FUNC_EXTENDSFTF2:
    case POLY_IMPORT_FUNC_EXTENDDFTF2:
    case POLY_IMPORT_FUNC_FLOATUNSITF:
      return POLY_IMPORT_X86_DESCRIPTOR_RETURN_FP128;
    case POLY_IMPORT_FUNC_X86_FPAIR64:
      return POLY_IMPORT_X86_DESCRIPTOR_RETURN_FPAIR64;
    case POLY_IMPORT_FUNC_X86_FPAIR32:
      return POLY_IMPORT_X86_DESCRIPTOR_RETURN_FPAIR32;
    case POLY_IMPORT_FUNC_X86_VEC128_U32:
      if (arch == POLY_ARCH_RISCV)
        return POLY_IMPORT_X86_DESCRIPTOR_RETURN_VEC128 |
          POLY_IMPORT_X86_DESCRIPTOR_VEC128_FROM_GPR_PAIRS;
      return POLY_IMPORT_X86_DESCRIPTOR_RETURN_VEC128;
    case POLY_IMPORT_FUNC_X86_SRET_U64:
      if (arch == POLY_ARCH_AARCH64)
        return POLY_IMPORT_X86_DESCRIPTOR_AARCH64_SRET_X8;
      return 0;
    case POLY_IMPORT_FUNC_X86_SRET_U64_STACK:
      if (arch == POLY_ARCH_AARCH64)
        return POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS |
          POLY_IMPORT_X86_DESCRIPTOR_AARCH64_SRET_X8;
      return POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS;
    case POLY_IMPORT_FUNC_X86_SRET_U64_STACK10:
      if (arch == POLY_ARCH_AARCH64)
        return POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS |
          POLY_IMPORT_X86_DESCRIPTOR_AARCH64_SRET_X8;
      return POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS;
    default:
      return 0;
  }
}

static uint64_t x86_descriptor_stack_arg_count_for_import_id(
    uint64_t import_id) {
  switch (import_id) {
    case POLY_IMPORT_FUNC_X86_SUM10:
      return 4;
    case POLY_IMPORT_FUNC_X86_SUM14:
    case POLY_IMPORT_FUNC_X86_ALIGN14:
      return 8;
    case POLY_IMPORT_FUNC_X86_FP64_SUM10:
      return 2;
    case POLY_IMPORT_FUNC_X86_SRET_U64_STACK:
      return 1;
    case POLY_IMPORT_FUNC_X86_SRET_U64_STACK10:
      return 5;
    case POLY_IMPORT_FUNC_X86_SLOT5:
    case POLY_IMPORT_FUNC_ATOMIC_COMPARE_EXCHANGE_16:
      return 2;
    default:
      return 0;
  }
}

static int parse_u64(const char *text, uint64_t *value) {
  char *end = NULL;
  errno = 0;
  unsigned long long parsed = strtoull(text, &end, 0);
  if (errno || end == text || *end != '\0')
    return -1;
  *value = (uint64_t) parsed;
  return 0;
}

static int parse_request(const char *arg, struct poly_request *request) {
  memset(request, 0, sizeof(*request));
  request->call_kind = POLY_CALL_U64;
  request->repeat_count = 1;
  if (strncmp(arg, "repeat:", 7) == 0) {
    request->repeat_count = 2;
    arg += 7;
  }
  if (strncmp(arg, "fp64:", 5) == 0) {
    request->call_kind = POLY_CALL_FP64;
    arg += 5;
  }
  else if (strncmp(arg, "fp32:", 5) == 0) {
    request->call_kind = POLY_CALL_FP32;
    arg += 5;
  }
  else if (strncmp(arg, "fp64stack:", 10) == 0) {
    request->call_kind = POLY_CALL_FP64_STACK;
    arg += 10;
  }
  else if (strncmp(arg, "vec128u32:", 10) == 0) {
    request->call_kind = POLY_CALL_VEC128_U32;
    arg += 10;
  }
  else if (strncmp(arg, "hfa3f64:", 8) == 0) {
    request->call_kind = POLY_CALL_AARCH64_HFA3_F64;
    arg += 8;
  }
  else if (strncmp(arg, "hfa4f64:", 8) == 0) {
    request->call_kind = POLY_CALL_AARCH64_HFA4_F64;
    arg += 8;
  }
  else if (strncmp(arg, "hfa3f32:", 8) == 0) {
    request->call_kind = POLY_CALL_AARCH64_HFA3_F32;
    arg += 8;
  }
  else if (strncmp(arg, "hfa4f32:", 8) == 0) {
    request->call_kind = POLY_CALL_AARCH64_HFA4_F32;
    arg += 8;
  }
  else if (strncmp(arg, "hfa3f64arg:", 11) == 0) {
    request->call_kind = POLY_CALL_AARCH64_HFA3_F64_ARG;
    arg += 11;
  }
  else if (strncmp(arg, "hfa4f64arg:", 11) == 0) {
    request->call_kind = POLY_CALL_AARCH64_HFA4_F64_ARG;
    arg += 11;
  }
  else if (strncmp(arg, "hfa3f32arg:", 11) == 0) {
    request->call_kind = POLY_CALL_AARCH64_HFA3_F32_ARG;
    arg += 11;
  }
  else if (strncmp(arg, "hfa4f32arg:", 11) == 0) {
    request->call_kind = POLY_CALL_AARCH64_HFA4_F32_ARG;
    arg += 11;
  }
  else if (strncmp(arg, "pair:", 5) == 0) {
    request->call_kind = POLY_CALL_PAIR_U64;
    arg += 5;
  }
  else if (strncmp(arg, "sret:", 5) == 0) {
    request->call_kind = POLY_CALL_SRET_U64;
    arg += 5;
  }
  else if (strncmp(arg, "sret10:", 7) == 0) {
    request->call_kind = POLY_CALL_U64;
    arg += 7;
  }
  else if (strncmp(arg, "fpair:", 6) == 0) {
    request->call_kind = POLY_CALL_FPAIR64;
    arg += 6;
  }
  else if (strncmp(arg, "fpair32:", 8) == 0) {
    request->call_kind = POLY_CALL_FPAIR32;
    arg += 8;
  }
  else if (strncmp(arg, "fpair32arg:", 11) == 0) {
    request->call_kind = POLY_CALL_FPAIR32_ARG;
    arg += 11;
  }
  else if (strncmp(arg, "fpairarg:", 9) == 0) {
    request->call_kind = POLY_CALL_FPAIR64_ARG;
    arg += 9;
  }
  else if (strncmp(arg, "mixedargs:", 10) == 0) {
    request->call_kind = POLY_CALL_MIXED_ARGS;
    arg += 10;
  }
  else if (strncmp(arg, "hetero:", 7) == 0) {
    request->call_kind = POLY_CALL_HETERO_U64_F64;
    arg += 7;
  }
  else if (strncmp(arg, "heterorev:", 10) == 0) {
    request->call_kind = POLY_CALL_HETERO_F64_U64;
    arg += 10;
  }
  else if (strncmp(arg, "heterof32:", 10) == 0) {
    request->call_kind = POLY_CALL_HETERO_U64_F32;
    arg += 10;
  }
  else if (strncmp(arg, "heterof32rev:", 13) == 0) {
    request->call_kind = POLY_CALL_HETERO_F32_U64;
    arg += 13;
  }
  else if (strncmp(arg, "heterou32:", 10) == 0) {
    request->call_kind = POLY_CALL_HETERO_U32_F64;
    arg += 10;
  }
  else if (strncmp(arg, "heterou32rev:", 13) == 0) {
    request->call_kind = POLY_CALL_HETERO_F64_U32;
    arg += 13;
  }
  else if (strncmp(arg, "heterou32f32:", 13) == 0) {
    request->call_kind = POLY_CALL_COMPACT_U32_F32;
    arg += 13;
  }
  else if (strncmp(arg, "heterof32u32:", 13) == 0) {
    request->call_kind = POLY_CALL_COMPACT_F32_U32;
    arg += 13;
  }
  else if (strncmp(arg, "fini:", 5) == 0) {
    request->call_kind = POLY_CALL_FINI_RESULT;
    arg += 5;
  }
  else if (strncmp(arg, "depfini:", 8) == 0) {
    request->call_kind = POLY_CALL_DEP_FINI_RESULT;
    arg += 8;
  }
  const char *expected = strchr(arg, '=');
  size_t path_len = expected ? (size_t) (expected - arg) : strlen(arg);
  const char *symbol = memchr(arg, '#', path_len);
  size_t symbol_len = 0;
  if (symbol) {
    symbol_len = path_len - (size_t) (symbol - arg) - 1;
    path_len = (size_t) (symbol - arg);
  }
  if (path_len == 0 || path_len >= sizeof(request->path)) {
    fprintf(stderr, "POLYCALL_FAIL: bad argument: %s\n", arg);
    return -1;
  }
  if (symbol && (symbol_len == 0 || symbol_len >= sizeof(request->symbol))) {
    fprintf(stderr, "POLYCALL_FAIL: bad symbol argument: %s\n", arg);
    return -1;
  }

  memcpy(request->path, arg, path_len);
  request->path[path_len] = '\0';
  if (symbol) {
    memcpy(request->symbol, symbol + 1, symbol_len);
    request->symbol[symbol_len] = '\0';
  }
  if (expected) {
    if (parse_u64(expected + 1, &request->expected) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: bad expected value: %s\n", arg);
      return -1;
    }
    request->check_expected = 1;
  }
  return 0;
}

static int read_file(const char *path, unsigned char **data, size_t *size) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "POLYCALL_FAIL: unable to open %s: %s\n", path, strerror(errno));
    return -1;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "POLYCALL_FAIL: unable to seek %s\n", path);
    fclose(file);
    return -1;
  }
  long file_size = ftell(file);
  if (file_size <= 0) {
    fprintf(stderr, "POLYCALL_FAIL: unable to size %s\n", path);
    fclose(file);
    return -1;
  }
  rewind(file);

  unsigned char *buffer = malloc((size_t) file_size);
  if (!buffer) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory reading %s\n", path);
    fclose(file);
    return -1;
  }
  if (fread(buffer, 1, (size_t) file_size, file) != (size_t) file_size) {
    fprintf(stderr, "POLYCALL_FAIL: unable to read %s\n", path);
    free(buffer);
    fclose(file);
    return -1;
  }
  fclose(file);
  *data = buffer;
  *size = (size_t) file_size;
  return 0;
}

static void emit_u32(uint8_t *code, size_t *offset, uint32_t value) {
  code[(*offset)++] = (uint8_t) (value & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 8) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 16) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 24) & 0xff);
}

static void store_u32(uint8_t *code, size_t offset, uint32_t value) {
  code[offset] = (uint8_t) (value & 0xff);
  code[offset + 1] = (uint8_t) ((value >> 8) & 0xff);
  code[offset + 2] = (uint8_t) ((value >> 16) & 0xff);
  code[offset + 3] = (uint8_t) ((value >> 24) & 0xff);
}

static uint64_t align_down_u64(uint64_t value, uint64_t alignment) {
  return value & ~(alignment - 1);
}

static uint64_t align_up_u64(uint64_t value, uint64_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

static int align_up_size(size_t value, size_t alignment, size_t *result) {
  if (alignment <= 1) {
    *result = value;
    return 0;
  }
  if ((alignment & (alignment - 1)) != 0 ||
      value > SIZE_MAX - (alignment - 1))
    return -1;
  *result = (value + alignment - 1) & ~(alignment - 1);
  return 0;
}

static int reserve_tls_range(size_t *total_size, uint64_t memsz,
    uint64_t alignment, size_t *offset) {
  if (memsz == 0) {
    *offset = 0;
    return 0;
  }
  if (memsz > MAX_TLS_BYTES || alignment > MAX_TOTAL_TLS_BYTES)
    return -1;

  size_t aligned = 0;
  const size_t tls_alignment = alignment ? (size_t) alignment : 1;
  if (align_up_size(*total_size, tls_alignment, &aligned) < 0 ||
      aligned > MAX_TOTAL_TLS_BYTES ||
      memsz > MAX_TOTAL_TLS_BYTES - aligned)
    return -1;
  *offset = aligned;
  *total_size = aligned + (size_t) memsz;
  return 0;
}

static int record_load_segment(struct poly_load_segment *segments,
    size_t *count, const Elf64_Phdr *phdr, const char *path) {
  if (*count >= MAX_LOAD_SEGMENTS) {
    fprintf(stderr, "POLYCALL_FAIL: too many PT_LOAD segments: %s\n", path);
    return -1;
  }
  segments[*count].vaddr = phdr->p_vaddr;
  segments[*count].memsz = phdr->p_memsz;
  segments[*count].flags = phdr->p_flags;
  (*count)++;
  return 0;
}

static void emit_u64(uint8_t *code, size_t *offset, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    code[(*offset)++] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static void write_le64(uint8_t *bytes, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    bytes[n] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static uint64_t read_le64(const uint8_t *bytes) {
  uint64_t value = 0;
  for (unsigned n = 0; n < 8; n++)
    value |= (uint64_t) bytes[n] << (n * 8);
  return value;
}

static void emit_movabs_r10(uint8_t *code, size_t *offset, uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xba;
  emit_u64(code, offset, value);
}

static void emit_movabs_r11(uint8_t *code, size_t *offset, uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbb;
  emit_u64(code, offset, value);
}

static void emit_movabs_r12(uint8_t *code, size_t *offset, uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbc;
  emit_u64(code, offset, value);
}

static void emit_movabs_r13(uint8_t *code, size_t *offset, uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbd;
  emit_u64(code, offset, value);
}

static void emit_movabs_r14(uint8_t *code, size_t *offset, uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbe;
  emit_u64(code, offset, value);
}

static void emit_aarch64_movabs(uint8_t *code, size_t *offset, uint32_t rd,
    uint64_t value) {
  emit_u32(code, offset, 0xd2800000U |
    (((uint32_t) value & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2a00000U |
    ((((uint32_t) (value >> 16)) & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2c00000U |
    ((((uint32_t) (value >> 32)) & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2e00000U |
    ((((uint32_t) (value >> 48)) & 0xffffU) << 5) | rd);
}

static uint32_t riscv_ld(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (0x3U << 12) | (rd << 7) | 0x03U;
}

static uint32_t riscv_sd(uint32_t rs2, uint32_t rs1, int32_t imm) {
  const uint32_t imm12 = (uint32_t) imm & 0xfffU;
  return ((imm12 >> 5) << 25) | (rs2 << 20) | (rs1 << 15) |
    (0x3U << 12) | ((imm12 & 0x1fU) << 7) | 0x23U;
}

static uint32_t riscv_addi(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (0x0U << 12) | (rd << 7) | 0x13U;
}

static void emit_save_callee_regs(uint8_t *code, size_t *offset,
    uint64_t save_area) {
  emit_movabs_r10(code, offset, save_area);
  const uint8_t save[] = {
    0x49, 0x89, 0x1a,       // mov [r10],rbx
    0x49, 0x89, 0x6a, 0x08, // mov [r10+8],rbp
    0x4d, 0x89, 0x62, 0x10, // mov [r10+16],r12
    0x4d, 0x89, 0x6a, 0x18, // mov [r10+24],r13
    0x4d, 0x89, 0x72, 0x20, // mov [r10+32],r14
    0x4d, 0x89, 0x7a, 0x28  // mov [r10+40],r15
  };
  memcpy(code + *offset, save, sizeof(save));
  *offset += sizeof(save);
}

static void emit_restore_callee_regs(uint8_t *code, size_t *offset,
    uint64_t save_area) {
  emit_movabs_r10(code, offset, save_area);
  const uint8_t restore[] = {
    0x49, 0x8b, 0x1a,       // mov rbx,[r10]
    0x49, 0x8b, 0x6a, 0x08, // mov rbp,[r10+8]
    0x4d, 0x8b, 0x62, 0x10, // mov r12,[r10+16]
    0x4d, 0x8b, 0x6a, 0x18, // mov r13,[r10+24]
    0x4d, 0x8b, 0x72, 0x20, // mov r14,[r10+32]
    0x4d, 0x8b, 0x7a, 0x28  // mov r15,[r10+40]
  };
  memcpy(code + *offset, restore, sizeof(restore));
  *offset += sizeof(restore);
}

static int detect_arch(uint16_t machine, struct poly_program *program) {
  if (machine == EM_AARCH64) {
    program->arch = POLY_ARCH_AARCH64;
    program->arch_name = "aarch64";
    return 0;
  }
  if (machine == EM_RISCV) {
    program->arch = POLY_ARCH_RISCV;
    program->arch_name = "riscv";
    return 0;
  }
  return -1;
}

static int check_elf_table(uint64_t offset, uint64_t count, uint64_t entsize,
    size_t file_size) {
  if (offset > file_size || entsize == 0 || count > (UINT64_MAX / entsize) ||
      count * entsize > file_size - offset)
    return -1;
  return 0;
}

static int resolve_elf_symbol_from_sections(const unsigned char *data,
    size_t size, const Elf64_Ehdr *ehdr, const char *symbol_name,
    uint64_t *symbol_vaddr) {
  if (ehdr->e_shentsize < sizeof(Elf64_Shdr) ||
      check_elf_table(ehdr->e_shoff, ehdr->e_shnum, ehdr->e_shentsize, size) < 0)
    return -1;

  const Elf64_Shdr *sections = (const Elf64_Shdr *) (data + ehdr->e_shoff);
  for (uint16_t n = 0; n < ehdr->e_shnum; n++) {
    const Elf64_Shdr *symtab = &sections[n];
    if (symtab->sh_type != SHT_DYNSYM && symtab->sh_type != SHT_SYMTAB)
      continue;
    if (symtab->sh_entsize < sizeof(Elf64_Sym) ||
        symtab->sh_link >= ehdr->e_shnum ||
        symtab->sh_offset > size ||
        symtab->sh_size > size - symtab->sh_offset ||
        symtab->sh_size % symtab->sh_entsize != 0)
      continue;

    const Elf64_Shdr *strtab = &sections[symtab->sh_link];
    if (strtab->sh_type != SHT_STRTAB ||
        strtab->sh_offset > size ||
        strtab->sh_size > size - strtab->sh_offset)
      continue;

    const Elf64_Sym *symbols = (const Elf64_Sym *) (data + symtab->sh_offset);
    const char *strings = (const char *) (data + strtab->sh_offset);
    const size_t symbol_count = (size_t) (symtab->sh_size / symtab->sh_entsize);
    for (size_t s = 0; s < symbol_count; s++) {
      const Elf64_Sym *sym = (const Elf64_Sym *) ((const unsigned char *) symbols + s * symtab->sh_entsize);
      const unsigned type = ELF64_ST_TYPE(sym->st_info);
      if (sym->st_name >= strtab->sh_size ||
          sym->st_shndx == SHN_UNDEF ||
          (type != STT_FUNC && type != STT_NOTYPE))
        continue;
      if (strcmp(strings + sym->st_name, symbol_name) == 0) {
        *symbol_vaddr = sym->st_value;
        return 0;
      }
    }
  }

  return -1;
}

static int image_vaddr_to_offset(uint64_t base_vaddr, size_t image_size,
    uint64_t vaddr, uint64_t size, size_t *offset) {
  if (vaddr < base_vaddr || size > image_size)
    return -1;
  const uint64_t image_offset = vaddr - base_vaddr;
  if (image_offset > image_size - size)
    return -1;
  *offset = (size_t) image_offset;
  return 0;
}

static int elf_vaddr_to_image_offset(const struct poly_program *program,
    uint64_t vaddr, uint64_t size, size_t *offset) {
  return image_vaddr_to_offset(program->base_vaddr, program->image_size,
    vaddr, size, offset);
}

static uint32_t relative_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_RELATIVE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_RELATIVE;
  return UINT32_MAX;
}

static int symbolic_64_reloc_type_for_arch(int arch, uint32_t type) {
  if (arch == POLY_ARCH_AARCH64)
    return type == R_AARCH64_ABS64 || type == R_AARCH64_GLOB_DAT ||
      type == R_AARCH64_JUMP_SLOT;
  if (arch == POLY_ARCH_RISCV)
    return type == R_RISCV_64 || type == R_RISCV_JUMP_SLOT;
  return 0;
}

static uint32_t irelative_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_IRELATIVE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_IRELATIVE;
  return UINT32_MAX;
}

static uint32_t copy_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_COPY;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_COPY;
  return UINT32_MAX;
}

static uint32_t none_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_NONE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_NONE;
  return UINT32_MAX;
}

static int reloc_base_is_resolver(int base_kind) {
  return base_kind == RELOC_BASE_IRELATIVE ||
    base_kind == RELOC_BASE_ROOT_IFUNC ||
    base_kind == RELOC_BASE_ROOT_IFUNC_VEC128 ||
    base_kind == RELOC_BASE_ROOT_IFUNC_COMPACT_U32_F32 ||
    base_kind == RELOC_BASE_ROOT_IFUNC_COMPACT_F32_U32 ||
    base_kind == RELOC_BASE_ROOT_IFUNC_FP64_STACK ||
    (base_kind >= RELOC_BASE_DEP_IFUNC &&
     base_kind < RELOC_BASE_DEP_IFUNC + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC &&
     base_kind < RELOC_BASE_DEP_CROSS_IFUNC + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_VEC128 &&
     base_kind < RELOC_BASE_DEP_CROSS_IFUNC_VEC128 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 &&
     base_kind <
       RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 &&
     base_kind <
       RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK &&
     base_kind < RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK + MAX_NEEDED_DEPS);
}

static int reloc_base_is_dep_cross_stub(int base_kind) {
  return (base_kind >= RELOC_BASE_DEP_CROSS_STUB &&
      base_kind < RELOC_BASE_DEP_CROSS_STUB + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_STUB_VEC128 &&
      base_kind < RELOC_BASE_DEP_CROSS_STUB_VEC128 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32 &&
      base_kind <
        RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32 &&
      base_kind <
        RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_STUB_FP64_STACK &&
      base_kind < RELOC_BASE_DEP_CROSS_STUB_FP64_STACK + MAX_NEEDED_DEPS);
}

static int reloc_base_is_dep_cross_ifunc(int base_kind) {
  return (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC &&
      base_kind < RELOC_BASE_DEP_CROSS_IFUNC + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_VEC128 &&
      base_kind < RELOC_BASE_DEP_CROSS_IFUNC_VEC128 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 &&
      base_kind <
        RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 &&
      base_kind <
        RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 + MAX_NEEDED_DEPS) ||
    (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK &&
      base_kind < RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK + MAX_NEEDED_DEPS);
}

static int cross_bridge_kind_from_name(const char *name) {
  if (strcmp(name, "default") == 0)
    return POLY_CROSS_BRIDGE_DEFAULT;
  if (strcmp(name, "compact_u32_f32") == 0)
    return POLY_CROSS_BRIDGE_COMPACT_U32_F32;
  if (strcmp(name, "compact_f32_u32") == 0)
    return POLY_CROSS_BRIDGE_COMPACT_F32_U32;
  if (strcmp(name, "fp64_stack") == 0)
    return POLY_CROSS_BRIDGE_FP64_STACK;
  if (strcmp(name, "vec128_u32") == 0)
    return POLY_CROSS_BRIDGE_VEC128_U32;
  return -1;
}

static int cross_bridge_kind_for_specs(const struct poly_bridge_spec *specs,
    size_t spec_count, const char *symbol_name) {
  if (!symbol_name)
    return POLY_CROSS_BRIDGE_DEFAULT;
  for (size_t n = 0; n < spec_count; n++) {
    if (strcmp(specs[n].symbol, symbol_name) == 0)
      return specs[n].bridge_kind;
  }
  return POLY_CROSS_BRIDGE_DEFAULT;
}

static int append_bridge_spec(struct poly_bridge_spec *specs,
    size_t *spec_count, const char *symbol, const char *bridge,
    const char *source) {
  const int bridge_kind = cross_bridge_kind_from_name(bridge);
  if (bridge_kind < 0 || strlen(symbol) >= MAX_BRIDGE_SYMBOL ||
      *spec_count >= MAX_BRIDGE_SPECS) {
    fprintf(stderr, "POLYCALL_FAIL: bad poly ABI metadata entry: %s\n",
      source);
    return -1;
  }
  for (size_t n = 0; n < *spec_count; n++) {
    if (strcmp(specs[n].symbol, symbol) == 0) {
      specs[n].bridge_kind = bridge_kind;
      return 0;
    }
  }
  strcpy(specs[*spec_count].symbol, symbol);
  specs[*spec_count].bridge_kind = bridge_kind;
  (*spec_count)++;
  return 0;
}

static int parse_bridge_specs_text(const char *source, const char *text,
    size_t text_size, struct poly_bridge_spec *specs, size_t *spec_count) {
  char *buffer = calloc(1, text_size + 1);
  if (!buffer) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory reading %s\n", source);
    return -1;
  }
  memcpy(buffer, text, text_size);

  char *save = NULL;
  for (char *line = strtok_r(buffer, "\n", &save); line;
      line = strtok_r(NULL, "\n", &save)) {
    char *comment = strchr(line, '#');
    if (comment)
      *comment = '\0';
    char *token_save = NULL;
    char *symbol = strtok_r(line, " \t\r", &token_save);
    if (!symbol)
      continue;
    char *bridge = strtok_r(NULL, " \t\r", &token_save);
    if (!bridge || strtok_r(NULL, " \t\r", &token_save)) {
      fprintf(stderr, "POLYCALL_FAIL: bad poly ABI metadata line: %s\n",
        source);
      free(buffer);
      return -1;
    }
    if (append_bridge_spec(specs, spec_count, symbol, bridge, source) < 0) {
      free(buffer);
      return -1;
    }
  }
  free(buffer);
  return 0;
}

static uint32_t read_note_u32(const unsigned char *data) {
  uint32_t value;
  memcpy(&value, data, sizeof(value));
  return value;
}

static int parse_bridge_specs_notes(const char *source,
    const unsigned char *data, size_t size, size_t offset, size_t note_size,
    struct poly_bridge_spec *specs, size_t *spec_count) {
  if (offset > size || note_size > size - offset)
    return -1;
  const size_t end = offset + note_size;
  while (offset < end) {
    if (end - offset < 12)
      return -1;
    const uint32_t namesz = read_note_u32(data + offset);
    const uint32_t descsz = read_note_u32(data + offset + 4);
    offset += 12;
    if (namesz > end - offset)
      return -1;
    const char *note_name = (const char *) (data + offset);
    offset += (namesz + 3U) & ~3U;
    if (offset > end || descsz > end - offset)
      return -1;
    const char *desc = (const char *) (data + offset);
    offset += (descsz + 3U) & ~3U;
    if (offset > end)
      return -1;
    if (namesz == 8 && memcmp(note_name, "POLYABI", 8) == 0 &&
        parse_bridge_specs_text(source, desc, descsz, specs, spec_count) < 0)
      return -1;
  }
  return 0;
}

static int load_bridge_specs_from_program_headers(const char *path,
    const unsigned char *data, size_t size, const Elf64_Ehdr *ehdr,
    struct poly_bridge_spec *specs, size_t *spec_count) {
  if (ehdr->e_phentsize < sizeof(Elf64_Phdr) ||
      check_elf_table(ehdr->e_phoff, ehdr->e_phnum, ehdr->e_phentsize, size) < 0)
    return 0;

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *)
      (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type != PT_NOTE)
      continue;
    if (phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset)
      return -1;
    if (parse_bridge_specs_notes(path, data, size, (size_t) phdr->p_offset,
          (size_t) phdr->p_filesz, specs, spec_count) < 0)
      return -1;
  }
  return 0;
}

static int section_name(const unsigned char *data, size_t size,
    const Elf64_Ehdr *ehdr, const Elf64_Shdr *sections, uint16_t index,
    const char **name) {
  *name = NULL;
  if (ehdr->e_shstrndx == SHN_UNDEF || ehdr->e_shstrndx >= ehdr->e_shnum)
    return 0;
  const Elf64_Shdr *shstr = &sections[ehdr->e_shstrndx];
  if (shstr->sh_type != SHT_STRTAB || shstr->sh_offset > size ||
      shstr->sh_size > size - shstr->sh_offset)
    return -1;
  if (sections[index].sh_name >= shstr->sh_size)
    return -1;
  const char *strings = (const char *) (data + shstr->sh_offset);
  if (memchr(strings + sections[index].sh_name, '\0',
        (size_t) (shstr->sh_size - sections[index].sh_name)) == NULL)
    return -1;
  *name = strings + sections[index].sh_name;
  return 0;
}

static int load_bridge_specs_from_sections(const char *path,
    const unsigned char *data, size_t size, const Elf64_Ehdr *ehdr,
    struct poly_bridge_spec *specs, size_t *spec_count) {
  if (ehdr->e_shentsize < sizeof(Elf64_Shdr) ||
      check_elf_table(ehdr->e_shoff, ehdr->e_shnum, ehdr->e_shentsize, size) < 0)
    return 0;

  const Elf64_Shdr *sections = (const Elf64_Shdr *) (data + ehdr->e_shoff);
  for (uint16_t n = 0; n < ehdr->e_shnum; n++) {
    const Elf64_Shdr *section = &sections[n];
    if (section->sh_offset > size || section->sh_size > size - section->sh_offset)
      return -1;

    const char *name = NULL;
    if (section_name(data, size, ehdr, sections, n, &name) < 0)
      return -1;
    if (name && section->sh_type == SHT_PROGBITS &&
        strcmp(name, ".polyabi") == 0) {
      if (parse_bridge_specs_text(path,
            (const char *) (data + section->sh_offset),
            (size_t) section->sh_size, specs, spec_count) < 0)
        return -1;
      continue;
    }
    if (section->sh_type != SHT_NOTE)
      continue;
    if (parse_bridge_specs_notes(path, data, size, (size_t) section->sh_offset,
          (size_t) section->sh_size, specs, spec_count) < 0)
      return -1;
  }
  return 0;
}

static int load_bridge_specs_sidecar_for_path(const char *path,
    struct poly_bridge_spec *specs, size_t *spec_count) {
  char spec_path[MAX_DEP_PATH + 16];
  const size_t path_len = strlen(path);
  if (path_len + 8 >= sizeof(spec_path))
    return 0;
  memcpy(spec_path, path, path_len);
  memcpy(spec_path + path_len, ".polyabi", 9);

  FILE *file = fopen(spec_path, "rb");
  if (!file) {
    if (errno == ENOENT)
      return 0;
    fprintf(stderr, "POLYCALL_FAIL: unable to open %s: %s\n", spec_path,
      strerror(errno));
    return -1;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "POLYCALL_FAIL: unable to seek %s\n", spec_path);
    fclose(file);
    return -1;
  }
  long file_size = ftell(file);
  if (file_size < 0 || file_size > 64 * 1024) {
    fprintf(stderr, "POLYCALL_FAIL: bad poly ABI sidecar size: %s\n",
      spec_path);
    fclose(file);
    return -1;
  }
  rewind(file);

  char *buffer = calloc(1, (size_t) file_size + 1);
  if (!buffer) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory reading %s\n", spec_path);
    fclose(file);
    return -1;
  }
  if (file_size != 0 &&
      fread(buffer, 1, (size_t) file_size, file) != (size_t) file_size) {
    fprintf(stderr, "POLYCALL_FAIL: unable to read %s\n", spec_path);
    free(buffer);
    fclose(file);
    return -1;
  }
  fclose(file);

  int result = parse_bridge_specs_text(spec_path, buffer, (size_t) file_size,
    specs, spec_count);
  free(buffer);
  return result;
}

static int load_bridge_specs(const char *path, const unsigned char *data,
    size_t size, const Elf64_Ehdr *ehdr, struct poly_bridge_spec *specs,
    size_t *spec_count) {
  *spec_count = 0;
  if (load_bridge_specs_from_program_headers(path, data, size, ehdr, specs,
        spec_count) < 0)
    return -1;
  if (load_bridge_specs_from_sections(path, data, size, ehdr, specs,
        spec_count) < 0)
    return -1;
  return load_bridge_specs_sidecar_for_path(path, specs, spec_count);
}

static int dep_cross_stub_base_kind(size_t dep_index, int bridge_kind) {
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_U32_F32)
    return RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32 + (int) dep_index;
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_F32_U32)
    return RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32 + (int) dep_index;
  if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK)
    return RELOC_BASE_DEP_CROSS_STUB_FP64_STACK + (int) dep_index;
  if (bridge_kind == POLY_CROSS_BRIDGE_VEC128_U32)
    return RELOC_BASE_DEP_CROSS_STUB_VEC128 + (int) dep_index;
  return RELOC_BASE_DEP_CROSS_STUB + (int) dep_index;
}

static int dep_cross_ifunc_base_kind(size_t dep_index, int bridge_kind) {
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_U32_F32)
    return RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 + (int) dep_index;
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_F32_U32)
    return RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 + (int) dep_index;
  if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK)
    return RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK + (int) dep_index;
  if (bridge_kind == POLY_CROSS_BRIDGE_VEC128_U32)
    return RELOC_BASE_DEP_CROSS_IFUNC_VEC128 + (int) dep_index;
  return RELOC_BASE_DEP_CROSS_IFUNC + (int) dep_index;
}

static size_t dep_cross_stub_index(int base_kind) {
  if (base_kind >= RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32 &&
      base_kind <
        RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32 + MAX_NEEDED_DEPS)
    return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32);
  if (base_kind >= RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32 &&
      base_kind <
        RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32 + MAX_NEEDED_DEPS)
    return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32);
  if (base_kind >= RELOC_BASE_DEP_CROSS_STUB_FP64_STACK &&
      base_kind < RELOC_BASE_DEP_CROSS_STUB_FP64_STACK + MAX_NEEDED_DEPS)
    return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_STUB_FP64_STACK);
  if (base_kind >= RELOC_BASE_DEP_CROSS_STUB_VEC128 &&
      base_kind < RELOC_BASE_DEP_CROSS_STUB_VEC128 + MAX_NEEDED_DEPS)
    return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_STUB_VEC128);
  return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_STUB);
}

static size_t dep_cross_ifunc_index(int base_kind) {
  if (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 &&
      base_kind <
        RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 + MAX_NEEDED_DEPS)
    return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32);
  if (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 &&
      base_kind <
        RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 + MAX_NEEDED_DEPS)
    return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32);
  if (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK &&
      base_kind < RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK + MAX_NEEDED_DEPS)
    return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK);
  if (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_VEC128 &&
      base_kind < RELOC_BASE_DEP_CROSS_IFUNC_VEC128 + MAX_NEEDED_DEPS)
    return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_IFUNC_VEC128);
  return (size_t) (base_kind - RELOC_BASE_DEP_CROSS_IFUNC);
}

static int cross_bridge_kind_for_base(int base_kind) {
  if (base_kind == RELOC_BASE_ROOT_CROSS_STUB_COMPACT_U32_F32 ||
      (base_kind >= RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32 &&
        base_kind <
          RELOC_BASE_DEP_CROSS_STUB_COMPACT_U32_F32 + MAX_NEEDED_DEPS))
    return POLY_CROSS_BRIDGE_COMPACT_U32_F32;
  if (base_kind == RELOC_BASE_ROOT_CROSS_STUB_COMPACT_F32_U32 ||
      (base_kind >= RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32 &&
        base_kind <
          RELOC_BASE_DEP_CROSS_STUB_COMPACT_F32_U32 + MAX_NEEDED_DEPS))
    return POLY_CROSS_BRIDGE_COMPACT_F32_U32;
  if (base_kind == RELOC_BASE_ROOT_CROSS_STUB_FP64_STACK ||
      (base_kind >= RELOC_BASE_DEP_CROSS_STUB_FP64_STACK &&
        base_kind < RELOC_BASE_DEP_CROSS_STUB_FP64_STACK + MAX_NEEDED_DEPS))
    return POLY_CROSS_BRIDGE_FP64_STACK;
  if (base_kind == RELOC_BASE_ROOT_CROSS_STUB_VEC128 ||
      (base_kind >= RELOC_BASE_DEP_CROSS_STUB_VEC128 &&
        base_kind < RELOC_BASE_DEP_CROSS_STUB_VEC128 + MAX_NEEDED_DEPS))
    return POLY_CROSS_BRIDGE_VEC128_U32;
  return POLY_CROSS_BRIDGE_DEFAULT;
}

static int cross_bridge_kind_for_ifunc_base(int base_kind) {
  if (base_kind == RELOC_BASE_ROOT_IFUNC_COMPACT_U32_F32 ||
      (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 &&
        base_kind <
          RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_U32_F32 + MAX_NEEDED_DEPS))
    return POLY_CROSS_BRIDGE_COMPACT_U32_F32;
  if (base_kind == RELOC_BASE_ROOT_IFUNC_COMPACT_F32_U32 ||
      (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 &&
        base_kind <
          RELOC_BASE_DEP_CROSS_IFUNC_COMPACT_F32_U32 + MAX_NEEDED_DEPS))
    return POLY_CROSS_BRIDGE_COMPACT_F32_U32;
  if (base_kind == RELOC_BASE_ROOT_IFUNC_FP64_STACK ||
      (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK &&
        base_kind < RELOC_BASE_DEP_CROSS_IFUNC_FP64_STACK + MAX_NEEDED_DEPS))
    return POLY_CROSS_BRIDGE_FP64_STACK;
  if (base_kind == RELOC_BASE_ROOT_IFUNC_VEC128 ||
      (base_kind >= RELOC_BASE_DEP_CROSS_IFUNC_VEC128 &&
        base_kind < RELOC_BASE_DEP_CROSS_IFUNC_VEC128 + MAX_NEEDED_DEPS))
    return POLY_CROSS_BRIDGE_VEC128_U32;
  return POLY_CROSS_BRIDGE_DEFAULT;
}

static int root_cross_stub_base_kind(int bridge_kind) {
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_U32_F32)
    return RELOC_BASE_ROOT_CROSS_STUB_COMPACT_U32_F32;
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_F32_U32)
    return RELOC_BASE_ROOT_CROSS_STUB_COMPACT_F32_U32;
  if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK)
    return RELOC_BASE_ROOT_CROSS_STUB_FP64_STACK;
  if (bridge_kind == POLY_CROSS_BRIDGE_VEC128_U32)
    return RELOC_BASE_ROOT_CROSS_STUB_VEC128;
  return RELOC_BASE_ROOT_CROSS_STUB;
}

static int root_ifunc_base_kind(int bridge_kind) {
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_U32_F32)
    return RELOC_BASE_ROOT_IFUNC_COMPACT_U32_F32;
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_F32_U32)
    return RELOC_BASE_ROOT_IFUNC_COMPACT_F32_U32;
  if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK)
    return RELOC_BASE_ROOT_IFUNC_FP64_STACK;
  if (bridge_kind == POLY_CROSS_BRIDGE_VEC128_U32)
    return RELOC_BASE_ROOT_IFUNC_VEC128;
  return RELOC_BASE_ROOT_IFUNC;
}

static int call_kind_for_bridge_result(int root_arch, int call_kind,
    int bridge_kind) {
  if (root_arch != POLY_ARCH_RISCV || call_kind != POLY_CALL_U64)
    return call_kind;
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_U32_F32)
    return POLY_CALL_COMPACT_U32_F32;
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_F32_U32)
    return POLY_CALL_COMPACT_F32_U32;
  if (bridge_kind == POLY_CROSS_BRIDGE_VEC128_U32)
    return POLY_CALL_VEC128_U32;
  return call_kind;
}

static uint8_t pcall_opcode_for_call_kind(int arch, int call_kind) {
  if (arch == POLY_ARCH_AARCH64) {
    if (call_kind == POLY_CALL_SRET_U64)
      return 0x12;
    if (call_kind == POLY_CALL_FPAIR32)
      return 0x14;
    if (call_kind == POLY_CALL_FPAIR32_ARG)
      return 0x16;
    if (call_kind == POLY_CALL_HETERO_U64_F64 ||
        call_kind == POLY_CALL_HETERO_U32_F64)
      return 0x18;
    if (call_kind == POLY_CALL_HETERO_F64_U64 ||
        call_kind == POLY_CALL_HETERO_F64_U32)
      return 0x19;
    if (call_kind == POLY_CALL_HETERO_U64_F32)
      return 0x1a;
    if (call_kind == POLY_CALL_HETERO_F32_U64)
      return 0x1b;
    if (call_kind == POLY_CALL_FP64_STACK)
      return 0x1e;
    if (call_kind == POLY_CALL_VEC128_U32)
      return 0x21;
    if (call_kind == POLY_CALL_AARCH64_HFA3_F64)
      return 0x23;
    if (call_kind == POLY_CALL_AARCH64_HFA4_F64)
      return 0x24;
    if (call_kind == POLY_CALL_AARCH64_HFA3_F32)
      return 0x25;
    if (call_kind == POLY_CALL_AARCH64_HFA4_F32)
      return 0x26;
    if (call_kind == POLY_CALL_AARCH64_HFA3_F64_ARG)
      return 0x27;
    if (call_kind == POLY_CALL_AARCH64_HFA4_F64_ARG)
      return 0x28;
    if (call_kind == POLY_CALL_AARCH64_HFA3_F32_ARG)
      return 0x29;
    if (call_kind == POLY_CALL_AARCH64_HFA4_F32_ARG)
      return 0x2a;
    return 0x10;
  }
  if (call_kind == POLY_CALL_SRET_U64)
    return 0x13;
  if (call_kind == POLY_CALL_FPAIR32)
    return 0x15;
  if (call_kind == POLY_CALL_FPAIR32_ARG)
    return 0x17;
  if (call_kind == POLY_CALL_COMPACT_U32_F32)
    return 0x1c;
  if (call_kind == POLY_CALL_COMPACT_F32_U32)
    return 0x1d;
  if (call_kind == POLY_CALL_FP64_STACK)
    return 0x1f;
  if (call_kind == POLY_CALL_VEC128_U32)
    return 0x22;
  return 0x11;
}

static int reloc_base_is_root_cross_stub(int base_kind) {
  return base_kind == RELOC_BASE_ROOT_CROSS_STUB ||
    base_kind == RELOC_BASE_ROOT_CROSS_STUB_VEC128 ||
    base_kind == RELOC_BASE_ROOT_CROSS_STUB_COMPACT_U32_F32 ||
    base_kind == RELOC_BASE_ROOT_CROSS_STUB_COMPACT_F32_U32 ||
    base_kind == RELOC_BASE_ROOT_CROSS_STUB_FP64_STACK;
}

static int reloc_base_is_root_ifunc(int base_kind) {
  return base_kind == RELOC_BASE_ROOT_IFUNC ||
    base_kind == RELOC_BASE_ROOT_IFUNC_VEC128 ||
    base_kind == RELOC_BASE_ROOT_IFUNC_COMPACT_U32_F32 ||
    base_kind == RELOC_BASE_ROOT_IFUNC_COMPACT_F32_U32 ||
    base_kind == RELOC_BASE_ROOT_IFUNC_FP64_STACK;
}

static uint32_t aarch64_cross_call_opcode_for_bridge(int bridge_kind) {
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_U32_F32)
    return 0xd42fff80U; // brk #0x7ffc
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_F32_U32)
    return 0xd42fff60U; // brk #0x7ffb
  if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK)
    return 0xd42fff40U; // brk #0x7ffa
  if (bridge_kind == POLY_CROSS_BRIDGE_VEC128_U32)
    return 0xd42fff00U; // brk #0x7ff8
  return 0xd42fffa0U; // brk #0x7ffd
}

static uint32_t riscv_cross_call_opcode_for_bridge(int bridge_kind) {
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_U32_F32)
    return 0x0000107bU;
  if (bridge_kind == POLY_CROSS_BRIDGE_COMPACT_F32_U32)
    return 0x0000207bU;
  if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK)
    return 0x0000307bU;
  if (bridge_kind == POLY_CROSS_BRIDGE_VEC128_U32)
    return 0x0000507bU;
  return 0x0000005bU;
}

static uint32_t fallback_ret_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return 0xd65f03c0U;
  if (arch == POLY_ARCH_RISCV)
    return 0x00008067U;
  return 0;
}

static int align_stub_offset(size_t *offset, size_t alignment,
    size_t limit) {
  size_t aligned = 0;
  if (align_up_size(*offset, alignment, &aligned) < 0 || aligned > limit)
    return -1;
  *offset = aligned;
  return 0;
}

static int emit_cross_isa_call_stub(uint8_t *stubs, size_t stub_limit,
    size_t *stub_offset, int caller_arch, int callee_arch, uint64_t target,
    int bridge_kind, uint64_t *stub_addr) {
  if (caller_arch == callee_arch) {
    *stub_addr = target;
    return 0;
  }
  if (bridge_kind < POLY_CROSS_BRIDGE_DEFAULT ||
      bridge_kind > POLY_CROSS_BRIDGE_VEC128_U32)
    return -1;

  if (align_stub_offset(stub_offset, 8, stub_limit) < 0)
    return -1;
  const size_t start = *stub_offset;
  const uint64_t start_addr = (uint64_t) (uintptr_t) (stubs + start);

  if (caller_arch == POLY_ARCH_AARCH64 && callee_arch == POLY_ARCH_RISCV) {
    if (stub_limit - start < 64)
      return -1;
    if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK) {
      const uint64_t return_addr = start_addr + 36;
      emit_aarch64_movabs(stubs, stub_offset, 16, target);
      emit_aarch64_movabs(stubs, stub_offset, 17, return_addr);
      emit_u32(stubs, stub_offset,
        aarch64_cross_call_opcode_for_bridge(bridge_kind));
      emit_u32(stubs, stub_offset, 0xd65f03c0U); // ret
      *stub_addr = start_addr;
      return 0;
    }
    const uint64_t return_addr = start_addr + 52;
    emit_u32(stubs, stub_offset, 0xd10043ffU); // sub sp, sp, #16
    emit_u32(stubs, stub_offset, 0xf9400bf2U); // ldr x18, [sp, #16]
    emit_u32(stubs, stub_offset, 0xf90003f2U); // str x18, [sp]
    emit_u32(stubs, stub_offset, 0xf90007feU); // str x30, [sp, #8]
    emit_aarch64_movabs(stubs, stub_offset, 16, target);
    emit_aarch64_movabs(stubs, stub_offset, 17, return_addr);
    emit_u32(stubs, stub_offset,
      aarch64_cross_call_opcode_for_bridge(bridge_kind));
    emit_u32(stubs, stub_offset, 0xf94007feU); // ldr x30, [sp, #8]
    emit_u32(stubs, stub_offset, 0x910043ffU); // add sp, sp, #16
    emit_u32(stubs, stub_offset, 0xd65f03c0U); // ret
    *stub_addr = start_addr;
    return 0;
  }

  if (caller_arch == POLY_ARCH_RISCV && callee_arch == POLY_ARCH_AARCH64) {
    if (stub_limit - start < 64)
      return -1;
    const size_t auipc_target_pc = *stub_offset;
    emit_u32(stubs, stub_offset, 0x00000297U); // auipc x5,0
    const size_t ld_target_offset = *stub_offset;
    emit_u32(stubs, stub_offset, 0);
    const size_t auipc_return_pc = *stub_offset;
    emit_u32(stubs, stub_offset, 0x00000317U); // auipc x6,0
    const size_t ld_return_offset = *stub_offset;
    emit_u32(stubs, stub_offset, 0);
    if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK) {
      emit_u32(stubs, stub_offset, riscv_addi(2, 2, -80)); // addi sp,sp,-80
      emit_u32(stubs, stub_offset, riscv_sd(1, 2, 72)); // sd ra,72(sp)
    }
    else {
      emit_u32(stubs, stub_offset, 0xff010113U); // addi sp,sp,-16
      emit_u32(stubs, stub_offset, riscv_ld(7, 2, 16)); // ld t2,16(sp)
      emit_u32(stubs, stub_offset, riscv_sd(7, 2, 0)); // sd t2,0(sp)
      emit_u32(stubs, stub_offset, riscv_sd(1, 2, 8)); // sd ra,8(sp)
    }
    emit_u32(stubs, stub_offset,
      riscv_cross_call_opcode_for_bridge(bridge_kind));
    const size_t return_pc = *stub_offset;
    if (bridge_kind == POLY_CROSS_BRIDGE_FP64_STACK) {
      emit_u32(stubs, stub_offset, riscv_ld(1, 2, 72)); // ld ra,72(sp)
      emit_u32(stubs, stub_offset, riscv_addi(2, 2, 80)); // addi sp,sp,80
    }
    else {
      emit_u32(stubs, stub_offset, 0x00813083U); // ld ra,8(sp)
      emit_u32(stubs, stub_offset, 0x01010113U); // addi sp,sp,16
    }
    emit_u32(stubs, stub_offset, 0x00008067U); // ret
    if (align_stub_offset(stub_offset, 8, stub_limit) < 0)
      return -1;
    const size_t target_data_offset = *stub_offset;
    emit_u64(stubs, stub_offset, target);
    const size_t return_data_offset = *stub_offset;
    emit_u64(stubs, stub_offset,
      (uint64_t) (uintptr_t) (stubs + return_pc));
    store_u32(stubs, ld_target_offset, riscv_ld(5, 5,
      (int32_t) target_data_offset - (int32_t) auipc_target_pc));
    store_u32(stubs, ld_return_offset, riscv_ld(6, 6,
      (int32_t) return_data_offset - (int32_t) auipc_return_pc));
    *stub_addr = start_addr;
    return 0;
  }

  return -1;
}

static const uint64_t poly_import_value = 123;
static const uint64_t poly_stack_chk_guard = 0x706f6c7963616eULL;

static int aarch64_outline_atomic_name_matches(const char *symbol_name,
    const char *op_name, unsigned width) {
  static const char *const order_suffixes[] = {
    "relax", "acq", "rel", "acq_rel"
  };
  char prefix[32];
  int written = snprintf(prefix, sizeof(prefix), "__aarch64_%s%u_",
    op_name, width);

  if (written < 0 || (size_t) written >= sizeof(prefix))
    return 0;
  if (strncmp(symbol_name, prefix, (size_t) written) != 0)
    return 0;

  const char *order = symbol_name + written;
  for (size_t i = 0; i < sizeof(order_suffixes) / sizeof(order_suffixes[0]);
       i++) {
    if (strcmp(order, order_suffixes[i]) == 0)
      return 1;
  }

  return 0;
}

static int resolve_aarch64_outline_atomic_import(const char *symbol_name,
    uint64_t *symbol_value) {
  static const struct {
    const char *op_name;
    unsigned width;
    uint64_t import_id;
  } imports[] = {
    { "ldadd", 8, POLY_IMPORT_FUNC_AARCH64_LDADD8_ACQ_REL },
    { "ldadd", 4, POLY_IMPORT_FUNC_AARCH64_LDADD4_ACQ_REL },
    { "ldadd", 2, POLY_IMPORT_FUNC_AARCH64_LDADD2_ACQ_REL },
    { "ldadd", 1, POLY_IMPORT_FUNC_AARCH64_LDADD1_ACQ_REL },
    { "swp", 8, POLY_IMPORT_FUNC_AARCH64_SWP8_ACQ_REL },
    { "swp", 4, POLY_IMPORT_FUNC_AARCH64_SWP4_ACQ_REL },
    { "swp", 2, POLY_IMPORT_FUNC_AARCH64_SWP2_ACQ_REL },
    { "swp", 1, POLY_IMPORT_FUNC_AARCH64_SWP1_ACQ_REL },
    { "ldclr", 8, POLY_IMPORT_FUNC_AARCH64_LDCLR8_ACQ_REL },
    { "ldclr", 4, POLY_IMPORT_FUNC_AARCH64_LDCLR4_ACQ_REL },
    { "ldclr", 2, POLY_IMPORT_FUNC_AARCH64_LDCLR2_ACQ_REL },
    { "ldclr", 1, POLY_IMPORT_FUNC_AARCH64_LDCLR1_ACQ_REL },
    { "ldeor", 8, POLY_IMPORT_FUNC_AARCH64_LDEOR8_ACQ_REL },
    { "ldeor", 4, POLY_IMPORT_FUNC_AARCH64_LDEOR4_ACQ_REL },
    { "ldeor", 2, POLY_IMPORT_FUNC_AARCH64_LDEOR2_ACQ_REL },
    { "ldeor", 1, POLY_IMPORT_FUNC_AARCH64_LDEOR1_ACQ_REL },
    { "ldset", 8, POLY_IMPORT_FUNC_AARCH64_LDSET8_ACQ_REL },
    { "ldset", 4, POLY_IMPORT_FUNC_AARCH64_LDSET4_ACQ_REL },
    { "ldset", 2, POLY_IMPORT_FUNC_AARCH64_LDSET2_ACQ_REL },
    { "ldset", 1, POLY_IMPORT_FUNC_AARCH64_LDSET1_ACQ_REL },
    { "cas", 8, POLY_IMPORT_FUNC_AARCH64_CAS8_ACQ_REL },
    { "cas", 4, POLY_IMPORT_FUNC_AARCH64_CAS4_ACQ_REL },
    { "cas", 2, POLY_IMPORT_FUNC_AARCH64_CAS2_ACQ_REL },
    { "cas", 1, POLY_IMPORT_FUNC_AARCH64_CAS1_ACQ_REL }
  };

  for (size_t i = 0; i < sizeof(imports) / sizeof(imports[0]); i++) {
    if (aarch64_outline_atomic_name_matches(symbol_name, imports[i].op_name,
          imports[i].width)) {
      *symbol_value = imports[i].import_id * POLY_IMPORT_CALL_STRIDE;
      return 0;
    }
  }

  return -1;
}

static int resolve_import_function(const char *symbol_name,
    uint64_t *symbol_value) {
  if (strcmp(symbol_name, "poly_import_add") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_ADD * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_mul") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MUL * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__stack_chk_fail") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STACK_CHK_FAIL * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__errno_location") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_ERRNO_LOCATION * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "getauxval") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETAUXVAL * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "getpagesize") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETPAGESIZE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "sysconf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_SYSCONF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "getenv") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETENV * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "secure_getenv") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_SECURE_GETENV * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "malloc") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MALLOC * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "calloc") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_CALLOC * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "realloc") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_REALLOC * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "free") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FREE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strdup") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRDUP * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strndup") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRNDUP * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "posix_memalign") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_POSIX_MEMALIGN * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "aligned_alloc") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_ALIGNED_ALLOC * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "memalign") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMALIGN * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "qsort") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_QSORT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "bsearch") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_BSEARCH * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "qsort_r") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_QSORT_R * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_once") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_PTHREAD_ONCE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_key_create") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_KEY_CREATE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_key_delete") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_KEY_DELETE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_getspecific") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_GETSPECIFIC * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_setspecific") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_SETSPECIFIC * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutex_init") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEX_INIT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutex_destroy") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEX_DESTROY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutex_lock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEX_LOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutex_trylock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEX_TRYLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutex_unlock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEX_UNLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_self") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_PTHREAD_SELF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_equal") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_PTHREAD_EQUAL * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_rwlock_init") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_RWLOCK_INIT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_rwlock_destroy") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_RWLOCK_DESTROY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_rwlock_rdlock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_RWLOCK_RDLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_rwlock_tryrdlock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_RWLOCK_TRYRDLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_rwlock_wrlock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_RWLOCK_WRLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_rwlock_trywrlock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_RWLOCK_TRYWRLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_rwlock_unlock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_RWLOCK_UNLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutexattr_init") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_INIT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutexattr_destroy") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_DESTROY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutexattr_settype") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_SETTYPE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_mutexattr_gettype") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_MUTEXATTR_GETTYPE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_spin_init") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_SPIN_INIT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_spin_destroy") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_SPIN_DESTROY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_spin_lock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_SPIN_LOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_spin_trylock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_SPIN_TRYLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_spin_unlock") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_SPIN_UNLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_cond_init") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_COND_INIT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_cond_destroy") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_COND_DESTROY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_cond_signal") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_COND_SIGNAL * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "pthread_cond_broadcast") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_PTHREAD_COND_BROADCAST * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "clock_gettime") == 0 ||
      strcmp(symbol_name, "__clock_gettime64") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_CLOCK_GETTIME * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "clock_getres") == 0 ||
      strcmp(symbol_name, "__clock_getres_time64") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_CLOCK_GETRES * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "time") == 0 ||
      strcmp(symbol_name, "__time64") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_TIME * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "gettimeofday") == 0 ||
      strcmp(symbol_name, "__gettimeofday") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETTIMEOFDAY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "clock") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_CLOCK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "atexit") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_ATEXIT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__cxa_atexit") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_CXA_ATEXIT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__cxa_finalize") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_CXA_FINALIZE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__cxa_guard_acquire") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_CXA_GUARD_ACQUIRE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__cxa_guard_release") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_CXA_GUARD_RELEASE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__cxa_guard_abort") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_CXA_GUARD_ABORT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "getpid") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETPID * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "getppid") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETPPID * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "getuid") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETUID * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "geteuid") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETEUID * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "getgid") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETGID * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "getegid") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETEGID * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "gettid") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETTID * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "puts") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_PUTS * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_add") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT0 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_mul") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT1 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_sum6") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_sum8") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT5 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_sum10") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SUM10 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_sum14") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SUM14 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_align14") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_ALIGN14 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_i128") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_I128 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_fp64_add") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_fp64_sum8") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT6 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_fp64_sum10") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_FP64_SUM10 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_fpair64") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_FPAIR64 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_fpair32") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_FPAIR32 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_vec128_u32") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_VEC128_U32 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_sret_u64") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SRET_U64 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_sret_u64_stack") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_X86_SRET_U64_STACK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_sret_u64_stack10") == 0) {
    *symbol_value =
      POLY_IMPORT_FUNC_X86_SRET_U64_STACK10 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_mixed_u64_fp64") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT7 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_fp32_add") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT4 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_fp64_add") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FP64_ADD * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_fp32_add") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FP32_ADD * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strlen") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRLEN * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strcmp") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRCMP * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strncmp") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRNCMP * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strcasecmp") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRCASECMP * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strncasecmp") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRNCASECMP * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strcasestr") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRCASESTR * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "memcpy") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMCPY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "memmove") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMMOVE * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "memset") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMSET * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "memcmp") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMCMP * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "memchr") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMCHR * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "memrchr") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMRCHR * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "memmem") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMMEM * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strchr") == 0 ||
      strcmp(symbol_name, "index") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRCHR * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strrchr") == 0 ||
      strcmp(symbol_name, "rindex") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRRCHR * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strstr") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRSTR * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strcpy") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRCPY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strncpy") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRNCPY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strnlen") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRNLEN * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strcat") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRCAT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strncat") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRNCAT * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strspn") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRSPN * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strcspn") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRCSPN * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strpbrk") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRPBRK * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "stpcpy") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STPCPY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "stpncpy") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STPNCPY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "mempcpy") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MEMPCPY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "rawmemchr") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_RAWMEMCHR * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "strchrnul") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_STRCHRNUL * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "bcmp") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_BCMP * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "bcopy") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_BCOPY * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "bzero") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_BZERO * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__tls_get_addr") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_RISCV_TLS_GET_ADDR *
      POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__atomic_compare_exchange_16") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_ATOMIC_COMPARE_EXCHANGE_16 *
      POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__atomic_load_16") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_ATOMIC_LOAD_16 *
      POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__atomic_store_16") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_ATOMIC_STORE_16 *
      POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__udivti3") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_UDIVTI3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__umodti3") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_UMODTI3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__divti3") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_DIVTI3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__modti3") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MODTI3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__fixdfti") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FIXDFTI * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__fixunsdfti") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FIXUNSDFTI * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__floattidf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FLOATTIDF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__floatuntidf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FLOATUNTIDF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__fixsfti") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FIXSFTI * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__fixunssfti") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FIXUNSSFTI * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__floattisf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FLOATTISF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__floatuntisf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FLOATUNTISF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__clzdi2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_CLZDI2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__ctzdi2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_CTZDI2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__paritydi2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_PARITYDI2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__popcountdi2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_POPCOUNTDI2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__addtf3") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_ADDTF3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__subtf3") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_SUBTF3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__multf3") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_MULTF3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__divtf3") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_DIVTF3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__floatunditf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FLOATUNDITF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__fixunstfdi") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FIXUNSTFDI * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__floatditf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FLOATDITF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__floatsitf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FLOATSITF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__floatunsitf") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FLOATUNSITF * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__fixtfdi") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FIXTFDI * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__fixtfsi") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FIXTFSI * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__fixunstfsi") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_FIXUNSTFSI * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__eqtf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_EQTF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__lttf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_LTTF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__letf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_LETF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__gttf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GTTF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__getf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_GETF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__extendsftf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_EXTENDSFTF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__extenddftf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_EXTENDDFTF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__trunctfsf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_TRUNCTFSF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__trunctfdf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_TRUNCTFDF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__netf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_NETF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "__unordtf2") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_UNORDTF2 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (resolve_aarch64_outline_atomic_import(symbol_name, symbol_value) == 0)
    return 0;
  return -1;
}

static int append_dynamic_reloc(struct poly_program *program, size_t offset,
    uint64_t value, int base_kind) {
  if (program->reloc_count >= MAX_DYNAMIC_RELOCS) {
    fprintf(stderr, "POLYCALL_FAIL: too many dynamic relocations: %s\n",
      program->path);
    return -1;
  }

  struct poly_dynamic_reloc *relocs = realloc(program->relocs,
    (program->reloc_count + 1) * sizeof(*program->relocs));
  if (!relocs) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory reading relocations: %s\n",
      program->path);
    return -1;
  }
  program->relocs = relocs;
  program->relocs[program->reloc_count].offset = offset;
  program->relocs[program->reloc_count].size = 8;
  program->relocs[program->reloc_count].value = value;
  program->relocs[program->reloc_count].base_kind = base_kind;
  program->reloc_count++;
  return 0;
}

static int append_copy_reloc(struct poly_program *program, size_t offset,
    size_t size, uint64_t source_vaddr, size_t dep_index) {
  if (size == 0 || dep_index >= MAX_NEEDED_DEPS) {
    fprintf(stderr, "POLYCALL_FAIL: bad copy relocation: %s\n",
      program->path);
    return -1;
  }
  if (program->reloc_count >= MAX_DYNAMIC_RELOCS) {
    fprintf(stderr, "POLYCALL_FAIL: too many dynamic relocations: %s\n",
      program->path);
    return -1;
  }

  struct poly_dynamic_reloc *relocs = realloc(program->relocs,
    (program->reloc_count + 1) * sizeof(*program->relocs));
  if (!relocs) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory reading relocations: %s\n",
      program->path);
    return -1;
  }
  program->relocs = relocs;
  program->relocs[program->reloc_count].offset = offset;
  program->relocs[program->reloc_count].size = size;
  program->relocs[program->reloc_count].value = source_vaddr;
  program->relocs[program->reloc_count].base_kind =
    RELOC_BASE_DEP_COPY + (int) dep_index;
  program->reloc_count++;
  return 0;
}

static int load_dynsym_from_sections(const unsigned char *data, size_t size,
    const Elf64_Ehdr *ehdr, struct poly_symbol_table *table) {
  memset(table, 0, sizeof(*table));
  if (ehdr->e_shentsize < sizeof(Elf64_Shdr) ||
      check_elf_table(ehdr->e_shoff, ehdr->e_shnum, ehdr->e_shentsize, size) < 0)
    return -1;

  const Elf64_Shdr *sections = (const Elf64_Shdr *) (data + ehdr->e_shoff);
  for (uint16_t n = 0; n < ehdr->e_shnum; n++) {
    const Elf64_Shdr *symtab = &sections[n];
    if (symtab->sh_type != SHT_DYNSYM)
      continue;
    if (symtab->sh_entsize < sizeof(Elf64_Sym) ||
        symtab->sh_link >= ehdr->e_shnum ||
        symtab->sh_offset > size ||
        symtab->sh_size > size - symtab->sh_offset ||
        symtab->sh_size % symtab->sh_entsize != 0)
      return -1;

    const Elf64_Shdr *strtab = &sections[symtab->sh_link];
    if (strtab->sh_type != SHT_STRTAB ||
        strtab->sh_offset > size ||
        strtab->sh_size > size - strtab->sh_offset)
      return -1;

    table->symbols = (const Elf64_Sym *) (data + symtab->sh_offset);
    table->symbol_count = (size_t) (symtab->sh_size / symtab->sh_entsize);
    table->strings = (const char *) (data + strtab->sh_offset);
    table->strings_size = (size_t) strtab->sh_size;
    return 0;
  }
  return -1;
}

static int load_symbol_count_from_gnu_hash(const struct poly_program *program,
    uint64_t gnu_hash_vaddr, size_t symtab_offset, size_t *symbol_count) {
  size_t hash_offset = 0;
  if (elf_vaddr_to_image_offset(program, gnu_hash_vaddr, 16, &hash_offset) < 0)
    return -1;

  const uint32_t *hash = (const uint32_t *) (program->image + hash_offset);
  const uint32_t nbuckets = hash[0];
  const uint32_t symoffset = hash[1];
  const uint32_t bloom_size = hash[2];
  if (nbuckets == 0 || bloom_size == 0)
    return -1;

  const uint64_t buckets_offset = (uint64_t) hash_offset + 16 +
    (uint64_t) bloom_size * sizeof(uint64_t);
  const uint64_t buckets_size = (uint64_t) nbuckets * sizeof(uint32_t);
  if (buckets_offset > program->image_size ||
      buckets_size > program->image_size - buckets_offset)
    return -1;

  const uint64_t chains_offset = buckets_offset + buckets_size;
  if (chains_offset > program->image_size ||
      symtab_offset > program->image_size ||
      (program->image_size - symtab_offset) / sizeof(Elf64_Sym) < symoffset)
    return -1;

  const size_t max_symbols =
    (program->image_size - symtab_offset) / sizeof(Elf64_Sym);
  const uint32_t *buckets =
    (const uint32_t *) (program->image + buckets_offset);
  size_t count = symoffset;

  for (uint32_t n = 0; n < nbuckets; n++) {
    uint32_t index = buckets[n];
    if (index == 0)
      continue;
    if (index < symoffset || index >= max_symbols)
      return -1;

    while (1) {
      const uint64_t chain_offset = chains_offset +
        (uint64_t) (index - symoffset) * sizeof(uint32_t);
      if (chain_offset > program->image_size ||
          sizeof(uint32_t) > program->image_size - chain_offset)
        return -1;

      const uint32_t chain = *(const uint32_t *)
        (program->image + chain_offset);
      if ((size_t) index + 1 > count)
        count = (size_t) index + 1;
      if (chain & 1)
        break;
      index++;
      if (index >= max_symbols)
        return -1;
    }
  }

  *symbol_count = count;
  return 0;
}

static int load_dynsym_from_dynamic(const struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count, struct poly_symbol_table *table) {
  uint64_t symtab_vaddr = 0;
  uint64_t strtab_vaddr = 0;
  uint64_t hash_vaddr = 0;
  uint64_t gnu_hash_vaddr = 0;
  uint64_t versym_vaddr = 0;
  uint64_t verdef_vaddr = 0;
  uint64_t verneed_vaddr = 0;
  uint64_t strsz = 0;
  uint64_t syment = sizeof(Elf64_Sym);
  uint64_t verdef_count = 0;
  uint64_t verneed_count = 0;

  memset(table, 0, sizeof(*table));
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_NULL:
        n = dyn_count;
        break;
      case DT_SYMTAB:
        symtab_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_STRTAB:
        strtab_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_HASH:
        hash_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_GNU_HASH:
        gnu_hash_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_VERSYM:
        versym_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_VERDEF:
        verdef_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_VERDEFNUM:
        verdef_count = dyn[n].d_un.d_val;
        break;
      case DT_VERNEED:
        verneed_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_VERNEEDNUM:
        verneed_count = dyn[n].d_un.d_val;
        break;
      case DT_STRSZ:
        strsz = dyn[n].d_un.d_val;
        break;
      case DT_SYMENT:
        syment = dyn[n].d_un.d_val;
        break;
      default:
        break;
    }
  }

  if (!symtab_vaddr || !strtab_vaddr || strsz == 0 ||
      syment != sizeof(Elf64_Sym))
    return -1;

  size_t symtab_offset = 0;
  size_t strtab_offset = 0;
  size_t symbol_count = 0;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, sizeof(Elf64_Sym),
        &symtab_offset) < 0 ||
      elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;
  if (hash_vaddr) {
    size_t hash_offset = 0;
    if (elf_vaddr_to_image_offset(program, hash_vaddr, 8, &hash_offset) < 0)
      return -1;
    const uint32_t *hash = (const uint32_t *) (program->image + hash_offset);
    const uint32_t nbucket = hash[0];
    const uint32_t nchain = hash[1];
    const uint64_t hash_size = (uint64_t) (2 + nbucket + nchain) *
      sizeof(uint32_t);
    if (nbucket == 0 || nchain == 0 ||
        elf_vaddr_to_image_offset(program, hash_vaddr, hash_size,
          &hash_offset) < 0)
      return -1;
    symbol_count = nchain;
  }
  else if (gnu_hash_vaddr) {
    if (load_symbol_count_from_gnu_hash(program, gnu_hash_vaddr,
          symtab_offset, &symbol_count) < 0)
      return -1;
  }
  else {
    if (strtab_offset <= symtab_offset ||
        (strtab_offset - symtab_offset) % sizeof(Elf64_Sym) != 0)
      return -1;
    symbol_count = (strtab_offset - symtab_offset) / sizeof(Elf64_Sym);
  }
  if (symbol_count > (SIZE_MAX / sizeof(Elf64_Sym)) ||
      elf_vaddr_to_image_offset(program, symtab_vaddr,
        (uint64_t) symbol_count * sizeof(Elf64_Sym), &symtab_offset) < 0)
    return -1;

  table->symbols = (const Elf64_Sym *) (program->image + symtab_offset);
  table->symbol_count = symbol_count;
  table->strings = (const char *) (program->image + strtab_offset);
  table->strings_size = (size_t) strsz;
  if (versym_vaddr) {
    size_t versym_offset = 0;
    if (elf_vaddr_to_image_offset(program, versym_vaddr,
          (uint64_t) symbol_count * sizeof(Elf64_Half), &versym_offset) < 0)
      return -1;
    table->versym = (const Elf64_Half *) (program->image + versym_offset);
    table->versym_count = symbol_count;
  }
  if (verdef_vaddr && verdef_count != 0) {
    size_t verdef_offset = 0;
    if (elf_vaddr_to_image_offset(program, verdef_vaddr,
          sizeof(Elf64_Verdef), &verdef_offset) < 0)
      return -1;
    table->verdef = program->image + verdef_offset;
    table->verdef_size = program->image_size - verdef_offset;
    table->verdef_count = (size_t) verdef_count;
  }
  if (verneed_vaddr && verneed_count != 0) {
    size_t verneed_offset = 0;
    if (elf_vaddr_to_image_offset(program, verneed_vaddr,
          sizeof(Elf64_Verneed), &verneed_offset) < 0)
      return -1;
    table->verneed = program->image + verneed_offset;
    table->verneed_size = program->image_size - verneed_offset;
    table->verneed_count = (size_t) verneed_count;
  }
  return 0;
}

static int load_dynamic_relocs(struct poly_program *program,
    const unsigned char *data, size_t size, const Elf64_Ehdr *ehdr,
    const Elf64_Dyn *dyn, size_t dyn_count);

static int build_needed_path(const char *owner_path, const char *needed,
    char *out, size_t out_size) {
  if (!needed || needed[0] == '\0')
    return -1;
  if (needed[0] == '/') {
    if (strlen(needed) >= out_size)
      return -1;
    strcpy(out, needed);
    return 0;
  }

  const char *slash = strrchr(owner_path, '/');
  if (!slash) {
    if (strlen(needed) >= out_size)
      return -1;
    strcpy(out, needed);
    return 0;
  }

  const size_t dir_len = (size_t) (slash - owner_path) + 1;
  const size_t needed_len = strlen(needed);
  if (dir_len + needed_len >= out_size)
    return -1;
  memcpy(out, owner_path, dir_len);
  memcpy(out + dir_len, needed, needed_len + 1);
  return 0;
}

static const char *path_basename(const char *path) {
  const char *slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

static int load_soname_from_dynamic(const struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count, char *out, size_t out_size) {
  uint64_t strtab_vaddr = 0;
  uint64_t strsz = 0;
  uint64_t soname_offset = UINT64_MAX;

  if (out_size == 0)
    return -1;
  out[0] = '\0';
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_NULL:
        n = dyn_count;
        break;
      case DT_STRTAB:
        strtab_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_STRSZ:
        strsz = dyn[n].d_un.d_val;
        break;
      case DT_SONAME:
        soname_offset = dyn[n].d_un.d_val;
        break;
      default:
        break;
    }
  }

  if (soname_offset == UINT64_MAX)
    return 0;
  if (!strtab_vaddr || strsz == 0 || soname_offset >= strsz)
    return -1;
  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;
  const char *strings = (const char *) (program->image + strtab_offset);
  const void *end = memchr(strings + soname_offset, '\0',
    (size_t) (strsz - soname_offset));
  if (!end)
    return -1;
  const size_t len = (size_t) ((const char *) end -
    (strings + soname_offset));
  if (len >= out_size)
    return -1;
  memcpy(out, strings + soname_offset, len);
  out[len] = '\0';
  return 0;
}

static int append_path_bytes(char *out, size_t *out_len, size_t out_size,
    const char *bytes, size_t bytes_len) {
  if (*out_len + bytes_len + 1 > out_size)
    return -1;
  memcpy(out + *out_len, bytes, bytes_len);
  *out_len += bytes_len;
  out[*out_len] = '\0';
  return 0;
}

static int append_origin_dir(const char *owner_path, char *out,
    size_t *out_len, size_t out_size) {
  const char *slash = strrchr(owner_path, '/');
  const size_t dir_len = slash ? (size_t) (slash - owner_path) : 0;
  return append_path_bytes(out, out_len, out_size, owner_path, dir_len);
}

static int expand_runpath_entry(const char *owner_path,
    const char *platform_name, const char *entry, size_t entry_len,
    char *out, size_t out_size) {
  size_t out_len = 0;
  if (out_size == 0)
    return -1;
  out[0] = '\0';
  for (size_t n = 0; n < entry_len;) {
    if (entry_len - n >= 9 && memcmp(entry + n, "${ORIGIN}", 9) == 0) {
      if (append_origin_dir(owner_path, out, &out_len, out_size) < 0)
        return -1;
      n += 9;
      continue;
    }
    if (entry_len - n >= 7 && memcmp(entry + n, "$ORIGIN", 7) == 0) {
      if (append_origin_dir(owner_path, out, &out_len, out_size) < 0)
        return -1;
      n += 7;
      continue;
    }
    if (entry_len - n >= 6 && memcmp(entry + n, "${LIB}", 6) == 0) {
      if (append_path_bytes(out, &out_len, out_size, "lib", 3) < 0)
        return -1;
      n += 6;
      continue;
    }
    if (entry_len - n >= 4 && memcmp(entry + n, "$LIB", 4) == 0) {
      if (append_path_bytes(out, &out_len, out_size, "lib", 3) < 0)
        return -1;
      n += 4;
      continue;
    }
    if (entry_len - n >= 11 && memcmp(entry + n, "${PLATFORM}", 11) == 0) {
      if (append_path_bytes(out, &out_len, out_size, platform_name,
            strlen(platform_name)) < 0)
        return -1;
      n += 11;
      continue;
    }
    if (entry_len - n >= 9 && memcmp(entry + n, "$PLATFORM", 9) == 0) {
      if (append_path_bytes(out, &out_len, out_size, platform_name,
            strlen(platform_name)) < 0)
        return -1;
      n += 9;
      continue;
    }
    if (append_path_bytes(out, &out_len, out_size, entry + n, 1) < 0)
      return -1;
    n++;
  }
  return out_len == 0 ? -1 : 0;
}

static int build_runpath_entry_needed_path(const char *entry, size_t entry_len,
    const char *needed, char *out, size_t out_size) {
  const size_t needed_len = strlen(needed);
  const int entry_has_slash = entry_len != 0 && entry[entry_len - 1] == '/';
  const size_t sep_len = entry_has_slash ? 0 : 1;
  if (entry_len + sep_len + needed_len + 1 > out_size)
    return -1;
  memcpy(out, entry, entry_len);
  if (sep_len != 0)
    out[entry_len] = '/';
  memcpy(out + entry_len + sep_len, needed, needed_len + 1);
  return 0;
}

static int build_runpath_needed_path(const char *owner_path,
    const char *platform_name, const char *runpath, size_t runpath_len,
    const char *needed, char *out, size_t out_size) {
  if (!runpath || !platform_name || runpath_len == 0 || needed[0] == '/')
    return -1;

  size_t start = 0;
  while (start < runpath_len) {
    size_t end = start;
    while (end < runpath_len && runpath[end] != ':')
      end++;
    const char *entry = runpath + start;
    const size_t entry_len = end - start;
    start = end + 1;
    if (entry_len == 0)
      continue;

    char expanded[MAX_DEP_PATH];
    if (expand_runpath_entry(owner_path, platform_name, entry, entry_len,
          expanded, sizeof(expanded)) == 0 &&
        build_runpath_entry_needed_path(expanded, strlen(expanded), needed,
          out, out_size) == 0 &&
        access(out, R_OK) == 0)
      return 0;
  }
  return -1;
}

static int load_needed_dependencies_from_dynamic(struct poly_program *owner,
    const char *origin_path, const uint8_t *image, size_t image_size,
    uint64_t base_vaddr, const Elf64_Dyn *dyn, size_t dyn_count,
    size_t needed_depth, const char *inherited_rpath,
    size_t inherited_rpath_len, const char *inherited_rpath_origin);

static const char *poly_library_path(void) {
  const char *library_path = getenv("POLY_LD_LIBRARY_PATH");
  if (library_path && library_path[0] != '\0')
    return library_path;
  return getenv("LD_LIBRARY_PATH");
}

static int dependency_identity_already_loaded(struct poly_program *program,
    const char *path, const char *needed, size_t needed_depth);

static int load_dependency_object(struct poly_program *owner, size_t dep_index,
    const char *path, int expected_arch, size_t needed_depth,
    const char *inherited_rpath, size_t inherited_rpath_len,
    const char *inherited_rpath_origin) {
  struct poly_dependency *dep = &owner->deps[dep_index];
  memset(dep, 0, sizeof(*dep));
  if (strlen(path) >= sizeof(dep->path)) {
    fprintf(stderr, "POLYCALL_FAIL: dependency path too long: %s\n", path);
    return -1;
  }
  strcpy(dep->path, path);
  dep->needed_depth = needed_depth;
  dep->lookup_rank = needed_depth == 0 ? owner->direct_dep_count++ :
    MAX_NEEDED_DEPS + dep_index;

  unsigned char *data = NULL;
  size_t size = 0;
  if (read_file(path, &data, &size) < 0)
    return -1;

  if (size < sizeof(Elf64_Ehdr)) {
    fprintf(stderr, "POLYCALL_FAIL: dependency ELF too small: %s\n", path);
    free(data);
    return -1;
  }

  const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *) data;
  struct poly_program arch_probe;
  memset(&arch_probe, 0, sizeof(arch_probe));
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
      ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
      (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) ||
      detect_arch(ehdr->e_machine, &arch_probe) < 0 ||
      (expected_arch != 0 && arch_probe.arch != expected_arch)) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported dependency ELF header: %s\n",
      path);
    free(data);
    return -1;
  }
  dep->arch = arch_probe.arch;
  dep->arch_name = arch_probe.arch_name;
  if (load_bridge_specs(dep->path, data, size, ehdr, dep->bridge_specs,
        &dep->bridge_spec_count) < 0) {
    free(data);
    return -1;
  }

  if (ehdr->e_phentsize < sizeof(Elf64_Phdr) ||
      ehdr->e_phoff > size ||
      (uint64_t) ehdr->e_phnum * ehdr->e_phentsize > size - ehdr->e_phoff) {
    fprintf(stderr, "POLYCALL_FAIL: bad dependency program header table: %s\n",
      path);
    free(data);
    return -1;
  }

  uint64_t base_vaddr = UINT64_MAX;
  uint64_t limit_vaddr = 0;
  uint64_t dynamic_vaddr = 0;
  uint64_t dynamic_size = 0;
  int found_load = 0;
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *)
      (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type == PT_DYNAMIC) {
      dynamic_vaddr = phdr->p_vaddr;
      dynamic_size = phdr->p_filesz;
      continue;
    }
    if (phdr->p_type == PT_GNU_RELRO) {
      dep->relro_vaddr = phdr->p_vaddr;
      dep->relro_size = phdr->p_memsz;
      continue;
    }
    if (phdr->p_type == PT_TLS) {
      if (phdr->p_filesz > phdr->p_memsz ||
          phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
          phdr->p_memsz > MAX_TLS_BYTES) {
        fprintf(stderr, "POLYCALL_FAIL: bad dependency TLS segment: %s\n",
          path);
        free(data);
        return -1;
      }
      dep->tls_vaddr = phdr->p_vaddr;
      dep->tls_filesz = phdr->p_filesz;
      dep->tls_memsz = phdr->p_memsz;
      dep->tls_align = phdr->p_align;
      continue;
    }
    if (phdr->p_type != PT_LOAD)
      continue;
    if (phdr->p_filesz > phdr->p_memsz ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
        phdr->p_vaddr > UINT64_MAX - phdr->p_memsz) {
      fprintf(stderr, "POLYCALL_FAIL: bad dependency load segment: %s\n",
        path);
      free(data);
      return -1;
    }
    const uint64_t segment_base = align_down_u64(phdr->p_vaddr, 0x1000);
    const uint64_t segment_limit = phdr->p_vaddr + phdr->p_memsz;
    if (record_load_segment(dep->load_segments, &dep->load_segment_count,
          phdr, path) < 0) {
      free(data);
      return -1;
    }
    if (segment_base < base_vaddr)
      base_vaddr = segment_base;
    if (segment_limit > limit_vaddr)
      limit_vaddr = segment_limit;
    found_load = 1;
  }

  if (!found_load || limit_vaddr <= base_vaddr ||
      limit_vaddr - base_vaddr > MAX_PROGRAM_BYTES - 4) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported dependency load image: %s\n",
      path);
    free(data);
    return -1;
  }

  dep->base_vaddr = base_vaddr;
  dep->image_size = (size_t) (limit_vaddr - base_vaddr + 4);
  if (reserve_tls_range(&owner->tls_total_size, dep->tls_memsz,
        dep->tls_align, &dep->tls_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported dependency TLS layout: %s\n",
      path);
    free(data);
    return -1;
  }
  dep->image = calloc(1, dep->image_size);
  if (!dep->image) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory loading dependency %s\n",
      path);
    free(data);
    return -1;
  }

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *)
      (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD)
      continue;
    const uint64_t load_offset = phdr->p_vaddr - base_vaddr;
    if (load_offset > dep->image_size ||
        phdr->p_filesz > dep->image_size - load_offset) {
      fprintf(stderr, "POLYCALL_FAIL: dependency load segment out of range: %s\n",
        path);
      free(dep->image);
      dep->image = NULL;
      free(dep->relocs);
      dep->relocs = NULL;
      free(data);
      return -1;
    }
    memcpy(dep->image + load_offset, data + phdr->p_offset,
      (size_t) phdr->p_filesz);
    dep->loaded_bytes += (size_t) phdr->p_filesz;
  }

  if (dynamic_size == 0 || dynamic_size % sizeof(Elf64_Dyn) != 0) {
    fprintf(stderr, "POLYCALL_FAIL: dependency lacks usable dynamic metadata: %s\n",
      path);
    free(dep->image);
    dep->image = NULL;
    free(data);
    return -1;
  }

  struct poly_program dep_view;
  memset(&dep_view, 0, sizeof(dep_view));
  dep_view.path = dep->path;
  dep_view.arch = dep->arch;
  dep_view.root_arch = owner->arch;
  dep_view.arch_name = dep->arch_name;
  dep_view.image = dep->image;
  dep_view.image_size = dep->image_size;
  dep_view.base_vaddr = dep->base_vaddr;
  dep_view.relocs = dep->relocs;
  dep_view.reloc_count = dep->reloc_count;
  dep_view.init_vaddr = dep->init_vaddr;
  dep_view.init_array_vaddr = dep->init_array_vaddr;
  dep_view.init_array_size = dep->init_array_size;
  dep_view.init_count = dep->init_count;
  dep_view.init_first = dep->init_first;
  dep_view.fini_vaddr = dep->fini_vaddr;
  dep_view.fini_array_vaddr = dep->fini_array_vaddr;
  dep_view.fini_array_size = dep->fini_array_size;
  dep_view.fini_count = dep->fini_count;
  dep_view.relro_vaddr = dep->relro_vaddr;
  dep_view.relro_size = dep->relro_size;
  dep_view.symbolic_binding = dep->symbolic_binding;
  dep_view.dependency_view = 1;
  dep_view.tls_vaddr = dep->tls_vaddr;
  dep_view.tls_filesz = dep->tls_filesz;
  dep_view.tls_memsz = dep->tls_memsz;
  dep_view.tls_align = dep->tls_align;
  dep_view.tls_offset = dep->tls_offset;

  size_t dynamic_offset = 0;
  if (elf_vaddr_to_image_offset(&dep_view, dynamic_vaddr, dynamic_size,
        &dynamic_offset) < 0 ||
      load_dynsym_from_dynamic(&dep_view,
        (const Elf64_Dyn *) (dep->image + dynamic_offset),
        (size_t) (dynamic_size / sizeof(Elf64_Dyn)), &dep->dynsym) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: dependency lacks dynamic symbols: %s\n",
      path);
    free(dep->image);
    dep->image = NULL;
    free(dep_view.relocs);
    dep_view.relocs = NULL;
    free(data);
    return -1;
  }
  const Elf64_Dyn *dynamic =
    (const Elf64_Dyn *) (dep->image + dynamic_offset);
  const size_t dynamic_count = (size_t) (dynamic_size / sizeof(Elf64_Dyn));
  if (load_soname_from_dynamic(&dep_view, dynamic, dynamic_count,
        dep->soname, sizeof(dep->soname)) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: bad dependency SONAME: %s\n", path);
    free(dep->image);
    dep->image = NULL;
    free(dep_view.relocs);
    dep_view.relocs = NULL;
    free(data);
    return -1;
  }
  if (dep->soname[0] == '\0') {
    const char *basename = path_basename(dep->path);
    if (strlen(basename) >= sizeof(dep->soname)) {
      fprintf(stderr, "POLYCALL_FAIL: dependency basename too long: %s\n", path);
      free(dep->image);
      dep->image = NULL;
      free(dep_view.relocs);
      dep_view.relocs = NULL;
      free(data);
      return -1;
    }
    strcpy(dep->soname, basename);
  }
  if (load_needed_dependencies_from_dynamic(owner, dep->path, dep->image,
        dep->image_size, dep->base_vaddr, dynamic, dynamic_count,
        dep->needed_depth + 1, inherited_rpath, inherited_rpath_len,
        inherited_rpath_origin) < 0) {
    free(dep->image);
    dep->image = NULL;
    free(dep_view.relocs);
    dep_view.relocs = NULL;
    free(data);
    return -1;
  }

  memcpy(dep_view.deps, owner->deps, sizeof(dep_view.deps));
  dep_view.dep_count = owner->dep_count;
  dep_view.root_dynsym = owner->root_dynsym;
  memcpy(dep_view.root_bridge_specs, owner->root_bridge_specs,
    sizeof(dep_view.root_bridge_specs));
  dep_view.root_bridge_spec_count = owner->root_bridge_spec_count;
  dep_view.needs_x86_import = owner->needs_x86_import;
  if (load_dynamic_relocs(&dep_view, data, size, ehdr, dynamic,
        dynamic_count) < 0) {
    free(dep->image);
    dep->image = NULL;
    free(dep_view.relocs);
    dep_view.relocs = NULL;
    free(data);
    return -1;
  }
  dep->relocs = dep_view.relocs;
  dep->reloc_count = dep_view.reloc_count;
  dep->init_vaddr = dep_view.init_vaddr;
  dep->init_array_vaddr = dep_view.init_array_vaddr;
  dep->init_array_size = dep_view.init_array_size;
  dep->init_count = dep_view.init_count;
  dep->init_first = dep_view.init_first;
  dep->fini_vaddr = dep_view.fini_vaddr;
  dep->fini_array_vaddr = dep_view.fini_array_vaddr;
  dep->fini_array_size = dep_view.fini_array_size;
  dep->fini_count = dep_view.fini_count;
  dep->relro_vaddr = dep_view.relro_vaddr;
  dep->relro_size = dep_view.relro_size;
  dep->symbolic_binding = dep_view.symbolic_binding;
  owner->needs_x86_import = dep_view.needs_x86_import;

  free(data);
  return 0;
}

static int load_needed_dependencies_from_dynamic(struct poly_program *owner,
    const char *origin_path, const uint8_t *image, size_t image_size,
    uint64_t base_vaddr, const Elf64_Dyn *dyn, size_t dyn_count,
    size_t needed_depth, const char *inherited_rpath,
    size_t inherited_rpath_len, const char *inherited_rpath_origin) {
  uint64_t strtab_vaddr = 0;
  uint64_t strsz = 0;
  uint64_t rpath_offset = 0;
  uint64_t runpath_offset = 0;
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_NULL:
        n = dyn_count;
        break;
      case DT_STRTAB:
        strtab_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_STRSZ:
        strsz = dyn[n].d_un.d_val;
        break;
      case DT_RPATH:
        rpath_offset = dyn[n].d_un.d_val;
        break;
      case DT_RUNPATH:
        runpath_offset = dyn[n].d_un.d_val;
        break;
      default:
        break;
    }
  }
  if (!strtab_vaddr || strsz == 0)
    return 0;

  size_t strtab_offset = 0;
  if (image_vaddr_to_offset(base_vaddr, image_size, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return 0;
  const char *strings = (const char *) (image + strtab_offset);
  const uint64_t search_path_offset = runpath_offset ? runpath_offset :
    rpath_offset;
  const char *search_path = NULL;
  size_t search_path_len = 0;
  const char *library_path = poly_library_path();
  const size_t library_path_len = library_path ? strlen(library_path) : 0;
  if (search_path_offset != 0 && search_path_offset < strsz) {
    const void *end = memchr(strings + search_path_offset, '\0',
      (size_t) (strsz - search_path_offset));
    if (!end) {
      fprintf(stderr, "POLYCALL_FAIL: bad DT_RUNPATH/DT_RPATH string: %s\n",
        origin_path);
      return -1;
    }
    search_path = strings + search_path_offset;
    search_path_len = (size_t) ((const char *) end - search_path);
  }
  const char *child_inherited_rpath = inherited_rpath;
  size_t child_inherited_rpath_len = inherited_rpath_len;
  const char *child_inherited_rpath_origin = inherited_rpath_origin;
  if (runpath_offset != 0) {
    child_inherited_rpath = NULL;
    child_inherited_rpath_len = 0;
    child_inherited_rpath_origin = NULL;
  }
  else if (rpath_offset != 0 && search_path_len != 0) {
    child_inherited_rpath = search_path;
    child_inherited_rpath_len = search_path_len;
    child_inherited_rpath_origin = origin_path;
  }

  for (size_t n = 0; n < dyn_count; n++) {
    if (dyn[n].d_tag == DT_NULL)
      break;
    if (dyn[n].d_tag != DT_NEEDED)
      continue;
    const uint64_t needed_offset = dyn[n].d_un.d_val;
    if (needed_offset >= strsz ||
        memchr(strings + needed_offset, '\0',
          (size_t) (strsz - needed_offset)) == NULL) {
      fprintf(stderr, "POLYCALL_FAIL: bad DT_NEEDED string: %s\n",
        origin_path);
      return -1;
    }

    const char *needed = strings + needed_offset;
    char expanded_needed[MAX_DEP_PATH];
    if (expand_runpath_entry(origin_path, owner->arch_name, needed,
          strlen(needed), expanded_needed, sizeof(expanded_needed)) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: bad DT_NEEDED string: %s\n",
        origin_path);
      return -1;
    }
    needed = expanded_needed;
    char needed_path[MAX_DEP_PATH];
    int found_needed = -1;
    if (needed[0] != '/' && library_path_len != 0)
      found_needed = build_runpath_needed_path(origin_path, owner->arch_name,
        library_path, library_path_len, needed, needed_path,
        sizeof(needed_path));
    if (needed[0] != '/' && search_path_len != 0 && found_needed < 0)
      found_needed = build_runpath_needed_path(origin_path, owner->arch_name,
        search_path, search_path_len, needed, needed_path,
        sizeof(needed_path));
    if (needed[0] != '/' && runpath_offset == 0 && inherited_rpath_len != 0 &&
        inherited_rpath != search_path && found_needed < 0)
      found_needed = build_runpath_needed_path(
        inherited_rpath_origin ? inherited_rpath_origin : origin_path,
        owner->arch_name, inherited_rpath, inherited_rpath_len, needed,
        needed_path, sizeof(needed_path));
    if (found_needed < 0 &&
        (build_needed_path(origin_path, needed, needed_path,
           sizeof(needed_path)) < 0 ||
         (needed[0] != '/' && access(needed_path, R_OK) != 0))) {
      fprintf(stderr, "POLYCALL_FAIL: bad DT_NEEDED path: %s\n",
        origin_path);
      return -1;
    }

    if (dependency_identity_already_loaded(owner, needed_path, needed,
          needed_depth))
      continue;
    if (owner->dep_count >= MAX_NEEDED_DEPS) {
      fprintf(stderr, "POLYCALL_FAIL: too many DT_NEEDED dependencies: %s\n",
        origin_path);
      return -1;
    }
    const size_t dep_index = owner->dep_count++;
    if (load_dependency_object(owner, dep_index, needed_path, 0,
          needed_depth, child_inherited_rpath, child_inherited_rpath_len,
          child_inherited_rpath_origin) < 0)
      return -1;
  }
  return 0;
}

static int load_needed_dependencies(struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count) {
  return load_needed_dependencies_from_dynamic(program, program->path,
    program->image, program->image_size, program->base_vaddr, dyn, dyn_count,
    0, NULL, 0, NULL);
}

static int preload_separator(char c) {
  return c == ':' || c == ' ' || c == '\t' || c == '\n';
}

static int dependency_path_already_loaded(const struct poly_program *program,
    const char *path) {
  for (size_t d = 0; d < program->dep_count; d++) {
    if (strcmp(program->deps[d].path, path) == 0)
      return 1;
  }
  return 0;
}

static int dependency_identity_already_loaded(struct poly_program *program,
    const char *path, const char *needed, size_t needed_depth) {
  const int needed_is_soname = needed && strchr(needed, '/') == NULL;
  for (size_t d = 0; d < program->dep_count; d++) {
    if (strcmp(program->deps[d].path, path) != 0 &&
        (!needed_is_soname ||
         strcmp(program->deps[d].soname, needed) != 0))
      continue;
    if (needed_depth == 0 &&
        program->deps[d].lookup_rank >= MAX_NEEDED_DEPS)
      program->deps[d].lookup_rank = program->direct_dep_count++;
    return 1;
  }
  return 0;
}

static void promote_preload_dependency_scope(struct poly_program *program,
    size_t first_dep) {
  for (size_t d = first_dep; d < program->dep_count; d++) {
    if (program->deps[d].lookup_rank >= MAX_NEEDED_DEPS)
      program->deps[d].lookup_rank = program->direct_dep_count++;
  }
}

static int build_preload_path(const struct poly_program *program,
    const char *token, size_t token_len, char *out, size_t out_size) {
  if (token_len == 0 || token_len >= MAX_DEP_PATH)
    return -1;

  char raw[MAX_DEP_PATH];
  memcpy(raw, token, token_len);
  raw[token_len] = '\0';

  char expanded[MAX_DEP_PATH];
  if (expand_runpath_entry(program->path, program->arch_name, raw, token_len,
        expanded, sizeof(expanded)) < 0)
    return -1;

  if (expanded[0] == '/') {
    if (strlen(expanded) >= out_size || access(expanded, R_OK) != 0)
      return -1;
    strcpy(out, expanded);
    return 0;
  }

  const char *library_path = poly_library_path();
  if (library_path && library_path[0] != '\0' &&
      build_runpath_needed_path(program->path, program->arch_name,
        library_path, strlen(library_path), expanded, out, out_size) == 0)
    return 0;

  if (build_needed_path(program->path, expanded, out, out_size) == 0 &&
      access(out, R_OK) == 0)
    return 0;

  return -1;
}

static int load_preload_dependencies(struct poly_program *program) {
  const char *preload = getenv("POLY_LD_PRELOAD");
  if (!preload || preload[0] == '\0')
    preload = getenv("LD_PRELOAD");
  if (!preload || preload[0] == '\0')
    return 0;

  size_t offset = 0;
  while (preload[offset] != '\0') {
    while (preload[offset] != '\0' && preload_separator(preload[offset]))
      offset++;
    const size_t start = offset;
    while (preload[offset] != '\0' && !preload_separator(preload[offset]))
      offset++;
    const size_t token_len = offset - start;
    if (token_len == 0)
      continue;

    char path[MAX_DEP_PATH];
    if (build_preload_path(program, preload + start, token_len, path,
          sizeof(path)) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: bad preload dependency path: %.*s\n",
        (int) token_len, preload + start);
      return -1;
    }
    if (dependency_path_already_loaded(program, path))
      continue;
    if (program->dep_count >= MAX_NEEDED_DEPS) {
      fprintf(stderr, "POLYCALL_FAIL: too many preload dependencies: %s\n",
        program->path);
      return -1;
    }
    const size_t dep_index = program->dep_count++;
    if (load_dependency_object(program, dep_index, path, program->arch, 0,
          NULL, 0, NULL) < 0)
      return -1;
    promote_preload_dependency_scope(program, dep_index);
  }
  return 0;
}

static uint16_t symbol_version_index(const struct poly_symbol_table *table,
    size_t symbol_index) {
  if (!table->versym || symbol_index >= table->versym_count)
    return VER_NDX_GLOBAL;
  return table->versym[symbol_index] & VERSYM_VERSION;
}

static struct poly_version_requirement symbol_required_version(
    const struct poly_symbol_table *table, size_t symbol_index) {
  const struct poly_version_requirement none = { NULL, NULL };
  const uint16_t version_index = symbol_version_index(table, symbol_index);
  if (version_index <= VER_NDX_GLOBAL || !table->verneed)
    return none;

  size_t need_offset = 0;
  for (size_t need = 0; need < table->verneed_count; need++) {
    if (need_offset > table->verneed_size ||
        sizeof(Elf64_Verneed) > table->verneed_size - need_offset)
      return none;
    const Elf64_Verneed *verneed =
      (const Elf64_Verneed *) (table->verneed + need_offset);
    size_t aux_offset = need_offset + verneed->vn_aux;
    for (size_t aux = 0; aux < verneed->vn_cnt; aux++) {
      if (aux_offset > table->verneed_size ||
          sizeof(Elf64_Vernaux) > table->verneed_size - aux_offset)
        return none;
      const Elf64_Vernaux *vernaux =
        (const Elf64_Vernaux *) (table->verneed + aux_offset);
      if ((vernaux->vna_other & VERSYM_VERSION) == version_index &&
          vernaux->vna_name < table->strings_size &&
          verneed->vn_file < table->strings_size) {
        const struct poly_version_requirement requirement = {
          table->strings + vernaux->vna_name,
          table->strings + verneed->vn_file
        };
        return requirement;
      }
      if (vernaux->vna_next == 0)
        break;
      aux_offset += vernaux->vna_next;
    }
    if (verneed->vn_next == 0)
      break;
    need_offset += verneed->vn_next;
  }
  return none;
}

static const char *symbol_export_version(const struct poly_symbol_table *table,
    size_t symbol_index) {
  const uint16_t version_index = symbol_version_index(table, symbol_index);
  if (version_index <= VER_NDX_GLOBAL || !table->verdef)
    return NULL;

  size_t def_offset = 0;
  for (size_t def = 0; def < table->verdef_count; def++) {
    if (def_offset > table->verdef_size ||
        sizeof(Elf64_Verdef) > table->verdef_size - def_offset)
      return NULL;
    const Elf64_Verdef *verdef =
      (const Elf64_Verdef *) (table->verdef + def_offset);
    if ((verdef->vd_ndx & VERSYM_VERSION) == version_index) {
      const size_t aux_offset = def_offset + verdef->vd_aux;
      if (aux_offset > table->verdef_size ||
          sizeof(Elf64_Verdaux) > table->verdef_size - aux_offset)
        return NULL;
      const Elf64_Verdaux *verdaux =
        (const Elf64_Verdaux *) (table->verdef + aux_offset);
      if (verdaux->vda_name < table->strings_size)
        return table->strings + verdaux->vda_name;
      return NULL;
    }
    if (verdef->vd_next == 0)
      break;
    def_offset += verdef->vd_next;
  }
  return NULL;
}

static int symbol_export_version_matches(
    const struct poly_symbol_table *table, size_t symbol_index,
    const char *required_version) {
  if (!required_version) {
    if (!table->versym || symbol_index >= table->versym_count)
      return 1;
    const Elf64_Half raw_version = table->versym[symbol_index];
    const uint16_t version_index = raw_version & VERSYM_VERSION;
    return version_index <= VER_NDX_GLOBAL ||
      (raw_version & VERSYM_HIDDEN) == 0;
  }

  const char *export_version = symbol_export_version(table, symbol_index);
  return export_version && strcmp(export_version, required_version) == 0;
}

static int dependency_matches_version_file(const struct poly_dependency *dep,
    const char *required_filename) {
  if (!required_filename)
    return 1;
  return strcmp(dep->soname, required_filename) == 0 ||
    strcmp(path_basename(dep->path), required_filename) == 0;
}

static int symbol_is_dependency_export(const Elf64_Sym *sym) {
  const unsigned bind = ELF64_ST_BIND(sym->st_info);
  const unsigned visibility = ELF64_ST_VISIBILITY(sym->st_other);
  return (bind == STB_GLOBAL || bind == STB_WEAK ||
      bind == STB_GNU_UNIQUE) &&
    (visibility == STV_DEFAULT || visibility == STV_PROTECTED);
}

static int symbol_definition_is_preemptible(const Elf64_Sym *sym) {
  const unsigned bind = ELF64_ST_BIND(sym->st_info);
  const unsigned type = ELF64_ST_TYPE(sym->st_info);
  return (bind == STB_GLOBAL || bind == STB_WEAK ||
      bind == STB_GNU_UNIQUE) &&
    ELF64_ST_VISIBILITY(sym->st_other) == STV_DEFAULT &&
    sym->st_shndx != SHN_UNDEF &&
    (type == STT_FUNC || type == STT_NOTYPE ||
     type == STT_OBJECT || type == STT_GNU_IFUNC);
}

static int resolve_symbol_from_table_filtered(const struct poly_symbol_table *table,
    const char *symbol_name, uint64_t *symbol_vaddr, int allow_object) {
  if (!table->symbols || !table->strings || !symbol_name)
    return -1;

  for (size_t s = 0; s < table->symbol_count; s++) {
    const Elf64_Sym *sym = &table->symbols[s];
    const unsigned type = ELF64_ST_TYPE(sym->st_info);
    if (sym->st_name >= table->strings_size ||
        sym->st_shndx == SHN_UNDEF ||
        (type != STT_FUNC && type != STT_NOTYPE &&
         (!allow_object || type != STT_OBJECT)))
      continue;
    if (strcmp(table->strings + sym->st_name, symbol_name) == 0) {
      *symbol_vaddr = sym->st_value;
      return 0;
    }
  }
  return -1;
}

static int resolve_symbol_from_table(const struct poly_symbol_table *table,
    const char *symbol_name, uint64_t *symbol_vaddr) {
  return resolve_symbol_from_table_filtered(table, symbol_name, symbol_vaddr, 0);
}

static int resolve_dynamic_symbol(const struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count, const char *symbol_name,
    uint64_t *symbol_vaddr) {
  struct poly_symbol_table table;
  if (load_dynsym_from_dynamic(program, dyn, dyn_count, &table) < 0)
    return -1;
  return resolve_symbol_from_table(&table, symbol_name, symbol_vaddr);
}

static int resolve_root_symbol(const struct poly_program *program,
    const char *symbol_name,
    const struct poly_version_requirement *required_version,
    uint64_t *symbol_value, int *base_kind) {
  const struct poly_symbol_table *table = &program->root_dynsym;
  if (!table->symbols || !table->strings || !symbol_name ||
      (required_version && required_version->filename))
    return -1;

  for (size_t s = 0; s < table->symbol_count; s++) {
    const Elf64_Sym *sym = &table->symbols[s];
    const unsigned type = ELF64_ST_TYPE(sym->st_info);
    if (sym->st_name >= table->strings_size ||
        sym->st_shndx == SHN_UNDEF ||
        !symbol_is_dependency_export(sym) ||
        (type != STT_FUNC && type != STT_NOTYPE &&
         type != STT_OBJECT && type != STT_GNU_IFUNC) ||
        strcmp(table->strings + sym->st_name, symbol_name) != 0 ||
        !symbol_export_version_matches(table, s,
          required_version ? required_version->name : NULL))
      continue;
    *symbol_value = sym->st_value;
    if (type == STT_GNU_IFUNC) {
      int bridge_kind = POLY_CROSS_BRIDGE_DEFAULT;
      if (program->dependency_view && program->root_arch != 0 &&
          program->root_arch != program->arch)
        bridge_kind = cross_bridge_kind_for_specs(
          program->root_bridge_specs, program->root_bridge_spec_count,
          symbol_name);
      *base_kind = root_ifunc_base_kind(bridge_kind);
    }
    else if (program->dependency_view &&
        (type == STT_FUNC || type == STT_NOTYPE) &&
        program->root_arch != 0 &&
        program->root_arch != program->arch)
      *base_kind = root_cross_stub_base_kind(cross_bridge_kind_for_specs(
        program->root_bridge_specs, program->root_bridge_spec_count,
        symbol_name));
    else
      *base_kind = RELOC_BASE_ROOT_LOAD_BIAS;
    return 0;
  }
  return -1;
}

static int resolve_dependency_symbol(const struct poly_program *program,
    const char *symbol_name,
    const struct poly_version_requirement *required_version,
    uint64_t *symbol_value, int *base_kind) {
  size_t best = program->dep_count;
  size_t best_rank = (size_t) -1;
  uint64_t best_value = 0;
  unsigned best_type = STT_NOTYPE;
  for (size_t n = 0; n < program->dep_count; n++) {
    const struct poly_symbol_table *table = &program->deps[n].dynsym;
    if (required_version &&
        !dependency_matches_version_file(&program->deps[n],
          required_version->filename))
      continue;
    if (!table->symbols || !table->strings || !symbol_name)
      continue;
    for (size_t s = 0; s < table->symbol_count; s++) {
      const Elf64_Sym *sym = &table->symbols[s];
      const unsigned type = ELF64_ST_TYPE(sym->st_info);
      if (sym->st_name >= table->strings_size ||
          sym->st_shndx == SHN_UNDEF ||
          !symbol_is_dependency_export(sym) ||
          (type != STT_FUNC && type != STT_NOTYPE &&
           type != STT_OBJECT && type != STT_GNU_IFUNC) ||
          strcmp(table->strings + sym->st_name, symbol_name) != 0 ||
          !symbol_export_version_matches(table, s,
            required_version ? required_version->name : NULL))
        continue;
      if (program->deps[n].lookup_rank >= best_rank)
        continue;
      best = n;
      best_rank = program->deps[n].lookup_rank;
      best_value = sym->st_value;
      best_type = type;
    }
  }
  if (best < program->dep_count) {
    *symbol_value = best_value;
    if (program->deps[best].arch != program->arch) {
      if (best_type == STT_FUNC || best_type == STT_NOTYPE) {
        *base_kind = dep_cross_stub_base_kind(best,
          cross_bridge_kind_for_specs(program->deps[best].bridge_specs,
            program->deps[best].bridge_spec_count, symbol_name));
        return 0;
      }
      if (best_type == STT_GNU_IFUNC) {
        *base_kind = dep_cross_ifunc_base_kind(best,
          cross_bridge_kind_for_specs(program->deps[best].bridge_specs,
            program->deps[best].bridge_spec_count, symbol_name));
        return 0;
      }
    }
    *base_kind = best_type == STT_GNU_IFUNC ?
      RELOC_BASE_DEP_IFUNC + (int) best :
      RELOC_BASE_DEP_LOAD_BIAS + (int) best;
    return 0;
  }
  return -1;
}

static int resolve_preempting_symbol(const struct poly_program *program,
    const char *symbol_name, uint64_t *symbol_value, int *base_kind) {
  if (resolve_root_symbol(program, symbol_name, NULL, symbol_value,
        base_kind) == 0)
    return 0;
  return resolve_dependency_symbol(program, symbol_name, NULL, symbol_value,
    base_kind);
}

static int resolve_dependency_object_symbol(const struct poly_program *program,
    const char *symbol_name,
    const struct poly_version_requirement *required_version, size_t *dep_index,
    uint64_t *symbol_value, size_t *symbol_size) {
  size_t best = program->dep_count;
  size_t best_rank = (size_t) -1;
  uint64_t best_value = 0;
  size_t best_size = 0;

  for (size_t n = 0; n < program->dep_count; n++) {
    const struct poly_symbol_table *table = &program->deps[n].dynsym;
    if (required_version &&
        !dependency_matches_version_file(&program->deps[n],
          required_version->filename))
      continue;
    if (!table->symbols || !table->strings || !symbol_name)
      continue;
    for (size_t s = 0; s < table->symbol_count; s++) {
      const Elf64_Sym *sym = &table->symbols[s];
      const unsigned type = ELF64_ST_TYPE(sym->st_info);
      if (sym->st_name >= table->strings_size ||
          sym->st_shndx == SHN_UNDEF ||
          !symbol_is_dependency_export(sym) ||
          (type != STT_OBJECT && type != STT_NOTYPE) ||
          strcmp(table->strings + sym->st_name, symbol_name) != 0 ||
          !symbol_export_version_matches(table, s,
            required_version ? required_version->name : NULL))
        continue;
      if (program->deps[n].lookup_rank >= best_rank)
        continue;
      best = n;
      best_rank = program->deps[n].lookup_rank;
      best_value = sym->st_value;
      best_size = (size_t) sym->st_size;
    }
  }

  if (best < program->dep_count) {
    *dep_index = best;
    *symbol_value = best_value;
    *symbol_size = best_size;
    return 0;
  }
  return -1;
}

static int resolve_dependency_tls_symbol(const struct poly_program *program,
    const char *symbol_name,
    const struct poly_version_requirement *required_version, size_t *dep_index,
    uint64_t *symbol_value) {
  size_t best = program->dep_count;
  size_t best_rank = (size_t) -1;
  uint64_t best_value = 0;

  for (size_t n = 0; n < program->dep_count; n++) {
    const struct poly_symbol_table *table = &program->deps[n].dynsym;
    if (required_version &&
        !dependency_matches_version_file(&program->deps[n],
          required_version->filename))
      continue;
    if (!table->symbols || !table->strings || !symbol_name)
      continue;
    for (size_t s = 0; s < table->symbol_count; s++) {
      const Elf64_Sym *sym = &table->symbols[s];
      if (sym->st_name >= table->strings_size ||
          sym->st_shndx == SHN_UNDEF ||
          !symbol_is_dependency_export(sym) ||
          ELF64_ST_TYPE(sym->st_info) != STT_TLS ||
          strcmp(table->strings + sym->st_name, symbol_name) != 0 ||
          !symbol_export_version_matches(table, s,
            required_version ? required_version->name : NULL))
        continue;
      if (program->deps[n].lookup_rank >= best_rank)
        continue;
      best = n;
      best_rank = program->deps[n].lookup_rank;
      best_value = sym->st_value;
    }
  }

  if (best < program->dep_count) {
    *dep_index = best;
    *symbol_value = best_value;
    return 0;
  }
  return -1;
}

static int resolve_root_tls_symbol(const struct poly_program *program,
    const char *symbol_name,
    const struct poly_version_requirement *required_version,
    uint64_t *symbol_value) {
  const struct poly_symbol_table *table = &program->root_dynsym;
  if (!table->symbols || !table->strings || !symbol_name ||
      (required_version && required_version->filename))
    return -1;

  for (size_t s = 0; s < table->symbol_count; s++) {
    const Elf64_Sym *sym = &table->symbols[s];
    if (sym->st_name >= table->strings_size ||
        sym->st_shndx == SHN_UNDEF ||
        !symbol_is_dependency_export(sym) ||
        ELF64_ST_TYPE(sym->st_info) != STT_TLS ||
        strcmp(table->strings + sym->st_name, symbol_name) != 0 ||
        !symbol_export_version_matches(table, s,
          required_version ? required_version->name : NULL))
      continue;
    *symbol_value = sym->st_value;
    return 0;
  }
  return -1;
}

static int resolve_external_reloc_symbol(struct poly_program *program,
    const char *symbol_name,
    const struct poly_version_requirement *required_version,
    uint64_t *symbol_value, int *base_kind) {
  if (resolve_root_symbol(program, symbol_name, required_version,
        symbol_value, base_kind) == 0)
    return 0;

  if (resolve_dependency_symbol(program, symbol_name, required_version,
        symbol_value,
        base_kind) == 0)
    return 0;

  if (strcmp(symbol_name, "poly_import_value") == 0) {
    *symbol_value = 0;
    *base_kind = RELOC_BASE_IMPORT_PAGE;
    return 0;
  }
  if (strcmp(symbol_name, "__stack_chk_guard") == 0) {
    *symbol_value = 8;
    *base_kind = RELOC_BASE_IMPORT_PAGE;
    return 0;
  }
  if (resolve_import_function(symbol_name, symbol_value) == 0) {
    if (strcmp(symbol_name, "__errno_location") == 0)
      program->needs_errno_location = 1;
    if (strcmp(symbol_name, "poly_import_x86_add") == 0 ||
        strcmp(symbol_name, "poly_import_x86_mul") == 0 ||
        strcmp(symbol_name, "poly_import_x86_sum6") == 0 ||
        strcmp(symbol_name, "poly_import_x86_sum8") == 0 ||
        strcmp(symbol_name, "poly_import_x86_sum10") == 0 ||
        strcmp(symbol_name, "poly_import_x86_sum14") == 0 ||
        strcmp(symbol_name, "poly_import_x86_align14") == 0 ||
        strcmp(symbol_name, "poly_import_x86_i128") == 0 ||
        strcmp(symbol_name, "poly_import_x86_fp64_add") == 0 ||
        strcmp(symbol_name, "poly_import_x86_fp64_sum8") == 0 ||
        strcmp(symbol_name, "poly_import_x86_fp64_sum10") == 0 ||
        strcmp(symbol_name, "poly_import_x86_fpair64") == 0 ||
        strcmp(symbol_name, "poly_import_x86_fpair32") == 0 ||
        strcmp(symbol_name, "poly_import_x86_vec128_u32") == 0 ||
        strcmp(symbol_name, "poly_import_x86_sret_u64") == 0 ||
        strcmp(symbol_name, "poly_import_x86_sret_u64_stack") == 0 ||
        strcmp(symbol_name, "poly_import_x86_sret_u64_stack10") == 0 ||
        strcmp(symbol_name, "poly_import_x86_mixed_u64_fp64") == 0 ||
        strcmp(symbol_name, "poly_import_x86_fp32_add") == 0 ||
        import_symbol_uses_x86_descriptor(symbol_name))
      program->needs_x86_import = 1;
    *base_kind = RELOC_BASE_IMPORT_CALL;
    return 0;
  }

  fprintf(stderr, "POLYCALL_FAIL: unresolved external relocation symbol=%s path=%s\n",
    symbol_name, program->path);
  return -1;
}

static int resolve_reloc_symbol(struct poly_program *program,
    const struct poly_symbol_table *table, uint64_t symbol_index,
    uint64_t *symbol_value, int *base_kind) {
  if (!table->symbols || symbol_index >= table->symbol_count) {
    fprintf(stderr, "POLYCALL_FAIL: relocation symbol table missing: %s\n",
      program->path);
    return -1;
  }

  const Elf64_Sym *sym = &table->symbols[symbol_index];
  if (sym->st_name >= table->strings_size) {
    fprintf(stderr, "POLYCALL_FAIL: bad relocation symbol name index=%llu path=%s\n",
      (unsigned long long) symbol_index, program->path);
    return -1;
  }
  const char *symbol_name = table->strings + sym->st_name;
  if (sym->st_shndx == SHN_UNDEF) {
    const struct poly_version_requirement required_version =
      symbol_required_version(table, symbol_index);
    if (ELF64_ST_BIND(sym->st_info) == STB_WEAK) {
      if (resolve_root_symbol(program, symbol_name, &required_version,
            symbol_value, base_kind) == 0)
        return 0;
      if (resolve_dependency_symbol(program, symbol_name, &required_version,
            symbol_value, base_kind) == 0)
        return 0;
      *symbol_value = 0;
      *base_kind = RELOC_BASE_ABSOLUTE;
      return 0;
    }
    *base_kind = RELOC_BASE_ABSOLUTE;
    return resolve_external_reloc_symbol(program, symbol_name,
      &required_version, symbol_value, base_kind);
  }

  if (program->dependency_view &&
      !program->symbolic_binding &&
      symbol_definition_is_preemptible(sym) &&
      resolve_preempting_symbol(program, symbol_name, symbol_value,
        base_kind) == 0)
    return 0;

  size_t symbol_offset = 0;
  if (elf_vaddr_to_image_offset(program, sym->st_value, 1, &symbol_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: relocation symbol escaped image: %s\n",
      program->path);
    return -1;
  }

  *symbol_value = sym->st_value;
  *base_kind = ELF64_ST_TYPE(sym->st_info) == STT_GNU_IFUNC ?
    RELOC_BASE_IRELATIVE : RELOC_BASE_LOAD_BIAS;
  return 0;
}

static int resolve_tls_reloc_symbol(struct poly_program *program,
    const struct poly_symbol_table *table, uint64_t symbol_index,
    uint64_t *tls_offset, int *base_kind) {
  if (!table->symbols || symbol_index >= table->symbol_count) {
    fprintf(stderr, "POLYCALL_FAIL: TLS relocation symbol table missing: %s\n",
      program->path);
    return -1;
  }

  const Elf64_Sym *sym = &table->symbols[symbol_index];
  if (sym->st_name >= table->strings_size) {
    fprintf(stderr, "POLYCALL_FAIL: bad TLS relocation symbol name index=%llu path=%s\n",
      (unsigned long long) symbol_index, program->path);
    return -1;
  }
  const char *symbol_name = table->strings + sym->st_name;
  if (sym->st_shndx == SHN_UNDEF) {
    size_t dep_index = 0;
    uint64_t dep_tls_offset = 0;
    const struct poly_version_requirement required_version =
      symbol_required_version(table, symbol_index);
    if (resolve_root_tls_symbol(program, symbol_name, &required_version,
          tls_offset) == 0) {
      *base_kind = RELOC_BASE_ROOT_TLS_OFFSET;
      return 0;
    }
    if (resolve_dependency_tls_symbol(program, symbol_name, &required_version,
          &dep_index, &dep_tls_offset) == 0) {
      *tls_offset = dep_tls_offset;
      *base_kind = RELOC_BASE_DEP_TLS_OFFSET + (int) dep_index;
      return 0;
    }
    if (ELF64_ST_BIND(sym->st_info) == STB_WEAK) {
      *tls_offset = 0;
      *base_kind = RELOC_BASE_ABSOLUTE;
      return 0;
    }
    fprintf(stderr, "POLYCALL_FAIL: unresolved external TLS relocation symbol=%s path=%s\n",
      symbol_name, program->path);
    return -1;
  }
  if (ELF64_ST_TYPE(sym->st_info) != STT_TLS) {
    fprintf(stderr, "POLYCALL_FAIL: non-TLS symbol used by TLS relocation path=%s\n",
      program->path);
    return -1;
  }

  *tls_offset = sym->st_value;
  *base_kind = RELOC_BASE_TLS_OFFSET;
  return 0;
}

static int process_rela_table(struct poly_program *program,
    const unsigned char *data, size_t size, const Elf64_Ehdr *ehdr,
    const Elf64_Dyn *dyn, size_t dyn_count, uint64_t rela_vaddr,
    uint64_t rela_size, const char *label) {
  size_t rela_offset = 0;
  if (elf_vaddr_to_image_offset(program, rela_vaddr, rela_size, &rela_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: %s table out of loaded image: %s\n",
      label, program->path);
    return -1;
  }

  const Elf64_Rela *rela = (const Elf64_Rela *) (program->image + rela_offset);
  const size_t rela_count = (size_t) (rela_size / sizeof(Elf64_Rela));
  const uint32_t relative_type = relative_reloc_type_for_arch(program->arch);
  const uint32_t irelative_type = irelative_reloc_type_for_arch(program->arch);
  const uint32_t copy_type = copy_reloc_type_for_arch(program->arch);
  const uint32_t none_type = none_reloc_type_for_arch(program->arch);
  struct poly_symbol_table dynsym;
  memset(&dynsym, 0, sizeof(dynsym));
  for (size_t n = 0; n < rela_count; n++) {
    const uint64_t symbol_index = ELF64_R_SYM(rela[n].r_info);
    const uint32_t reloc_type = ELF64_R_TYPE(rela[n].r_info);
    uint64_t reloc_value = 0;
    int base_kind = RELOC_BASE_LOAD_BIAS;
    if (reloc_type == none_type) {
      continue;
    }
    else if (symbol_index != 0 && reloc_type == copy_type) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: copy relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }
      if (symbol_index >= dynsym.symbol_count ||
          dynsym.symbols[symbol_index].st_name >= dynsym.strings_size) {
        fprintf(stderr, "POLYCALL_FAIL: bad copy relocation symbol: %s\n",
          program->path);
        return -1;
      }
      const Elf64_Sym *sym = &dynsym.symbols[symbol_index];
      const char *symbol_name = dynsym.strings + sym->st_name;
      const struct poly_version_requirement required_version =
        symbol_required_version(&dynsym, symbol_index);
      size_t dep_index = 0;
      uint64_t source_vaddr = 0;
      size_t source_size = 0;
      if (resolve_dependency_object_symbol(program, symbol_name,
            &required_version, &dep_index, &source_vaddr, &source_size) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: unresolved copy relocation symbol=%s path=%s\n",
          symbol_name, program->path);
        return -1;
      }
      const size_t copy_size = sym->st_size ? (size_t) sym->st_size :
        source_size;
      if (copy_size == 0 || (source_size != 0 && copy_size > source_size)) {
        fprintf(stderr, "POLYCALL_FAIL: bad copy relocation size symbol=%s path=%s\n",
          symbol_name, program->path);
        return -1;
      }
      size_t relocation_offset = 0;
      if (elf_vaddr_to_image_offset(program, rela[n].r_offset, copy_size,
            &relocation_offset) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: copy relocation target out of image: %s\n",
          program->path);
        return -1;
      }
      if (append_copy_reloc(program, relocation_offset, copy_size,
            source_vaddr, dep_index) < 0)
        return -1;
      continue;
    }
    if (program->arch == POLY_ARCH_AARCH64 && reloc_type == R_AARCH64_TLSDESC) {
      program->needs_x86_import = 1;
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: TLS relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }
      uint64_t tls_offset = 0;
      int tls_base_kind = RELOC_BASE_TLS_OFFSET;
      if (resolve_tls_reloc_symbol(program, &dynsym, symbol_index,
            &tls_offset, &tls_base_kind) < 0)
        return -1;

      size_t relocation_offset = 0;
      if (elf_vaddr_to_image_offset(program, rela[n].r_offset, 16,
            &relocation_offset) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: TLSDESC target out of image: %s\n",
          program->path);
        return -1;
      }
      if (append_dynamic_reloc(program, relocation_offset,
            POLY_IMPORT_FUNC_AARCH64_TLSDESC * POLY_IMPORT_CALL_STRIDE,
            RELOC_BASE_IMPORT_CALL) < 0 ||
          append_dynamic_reloc(program, relocation_offset + 8,
            tls_offset + (uint64_t) rela[n].r_addend,
            tls_base_kind) < 0)
        return -1;
      continue;
    }
    else if (program->arch == POLY_ARCH_AARCH64 &&
        (reloc_type == R_AARCH64_TLS_DTPMOD64 ||
         reloc_type == R_AARCH64_TLS_DTPREL64)) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: TLS relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }

      if (reloc_type == R_AARCH64_TLS_DTPMOD64) {
        reloc_value = 1;
        base_kind = RELOC_BASE_ABSOLUTE;
      }
      else {
        if (resolve_tls_reloc_symbol(program, &dynsym, symbol_index,
              &reloc_value, &base_kind) < 0)
          return -1;
        reloc_value += (uint64_t) rela[n].r_addend;
      }
    }
    else if (program->arch == POLY_ARCH_RISCV &&
        (reloc_type == R_RISCV_TLS_DTPMOD64 ||
         reloc_type == R_RISCV_TLS_DTPREL64 ||
         reloc_type == R_RISCV_TLS_TPREL64)) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: TLS relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }

      if (reloc_type == R_RISCV_TLS_DTPMOD64) {
        reloc_value = 1;
        base_kind = RELOC_BASE_ABSOLUTE;
      }
      else {
        if (resolve_tls_reloc_symbol(program, &dynsym, symbol_index,
              &reloc_value, &base_kind) < 0)
          return -1;
        reloc_value += (uint64_t) rela[n].r_addend;
      }
    }
    else if (program->arch == POLY_ARCH_AARCH64 &&
        reloc_type == R_AARCH64_TLS_TPREL64) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: TLS relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }
      if (resolve_tls_reloc_symbol(program, &dynsym, symbol_index,
            &reloc_value, &base_kind) < 0)
        return -1;
      reloc_value += (uint64_t) rela[n].r_addend;
    }
    else if (symbol_index == 0 && reloc_type == relative_type) {
      reloc_value = (uint64_t) rela[n].r_addend;
    }
    else if (symbol_index == 0 && reloc_type == irelative_type) {
      reloc_value = (uint64_t) rela[n].r_addend;
      base_kind = RELOC_BASE_IRELATIVE;
    }
    else if (symbol_index != 0 &&
        symbolic_64_reloc_type_for_arch(program->arch, reloc_type)) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: symbolic relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }
      uint64_t symbol_value = 0;
      base_kind = RELOC_BASE_LOAD_BIAS;
      if (resolve_reloc_symbol(program, &dynsym, symbol_index,
            &symbol_value, &base_kind) < 0)
        return -1;
      reloc_value = symbol_value + (uint64_t) rela[n].r_addend;
    }
    else {
      fprintf(stderr, "POLYCALL_FAIL: unsupported dynamic relocation type=%llu sym=%llu path=%s\n",
        (unsigned long long) reloc_type,
        (unsigned long long) symbol_index,
        program->path);
      return -1;
    }
    size_t relocation_offset = 0;
    if (elf_vaddr_to_image_offset(program, rela[n].r_offset, 8,
          &relocation_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: relocation target out of image: %s\n",
        program->path);
      return -1;
    }
    if (append_dynamic_reloc(program, relocation_offset, reloc_value,
          base_kind) < 0)
      return -1;
  }
  return 0;
}

static int process_rel_table(struct poly_program *program,
    const unsigned char *data, size_t size, const Elf64_Ehdr *ehdr,
    const Elf64_Dyn *dyn, size_t dyn_count, uint64_t rel_vaddr,
    uint64_t rel_size, const char *label) {
  size_t rel_offset = 0;
  if (elf_vaddr_to_image_offset(program, rel_vaddr, rel_size, &rel_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: %s table out of loaded image: %s\n",
      label, program->path);
    return -1;
  }

  const Elf64_Rel *rel = (const Elf64_Rel *) (program->image + rel_offset);
  const size_t rel_count = (size_t) (rel_size / sizeof(Elf64_Rel));
  const uint32_t relative_type = relative_reloc_type_for_arch(program->arch);
  const uint32_t irelative_type = irelative_reloc_type_for_arch(program->arch);
  const uint32_t copy_type = copy_reloc_type_for_arch(program->arch);
  const uint32_t none_type = none_reloc_type_for_arch(program->arch);
  struct poly_symbol_table dynsym;
  memset(&dynsym, 0, sizeof(dynsym));
  for (size_t n = 0; n < rel_count; n++) {
    const uint64_t symbol_index = ELF64_R_SYM(rel[n].r_info);
    const uint32_t reloc_type = ELF64_R_TYPE(rel[n].r_info);
    if (reloc_type == none_type)
      continue;

    size_t relocation_offset = 0;
    if (elf_vaddr_to_image_offset(program, rel[n].r_offset, 8,
          &relocation_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: relocation target out of image: %s\n",
        program->path);
      return -1;
    }

    uint64_t reloc_value = read_le64(program->image + relocation_offset);
    int base_kind = RELOC_BASE_LOAD_BIAS;
    if (symbol_index != 0 && reloc_type == copy_type) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: copy relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }
      if (symbol_index >= dynsym.symbol_count ||
          dynsym.symbols[symbol_index].st_name >= dynsym.strings_size) {
        fprintf(stderr, "POLYCALL_FAIL: bad copy relocation symbol: %s\n",
          program->path);
        return -1;
      }
      const Elf64_Sym *sym = &dynsym.symbols[symbol_index];
      const char *symbol_name = dynsym.strings + sym->st_name;
      const struct poly_version_requirement required_version =
        symbol_required_version(&dynsym, symbol_index);
      size_t dep_index = 0;
      uint64_t source_vaddr = 0;
      size_t source_size = 0;
      if (resolve_dependency_object_symbol(program, symbol_name,
            &required_version, &dep_index, &source_vaddr, &source_size) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: unresolved copy relocation symbol=%s path=%s\n",
          symbol_name, program->path);
        return -1;
      }
      const size_t copy_size = sym->st_size ? (size_t) sym->st_size :
        source_size;
      if (copy_size == 0 || (source_size != 0 && copy_size > source_size)) {
        fprintf(stderr, "POLYCALL_FAIL: bad copy relocation size symbol=%s path=%s\n",
          symbol_name, program->path);
        return -1;
      }
      if (relocation_offset > program->image_size ||
          copy_size > program->image_size - relocation_offset) {
        fprintf(stderr, "POLYCALL_FAIL: copy relocation target out of image: %s\n",
          program->path);
        return -1;
      }
      if (append_copy_reloc(program, relocation_offset, copy_size,
            source_vaddr, dep_index) < 0)
        return -1;
      continue;
    }
    if (symbol_index == 0 && reloc_type == relative_type) {
      // REL stores the addend in-place at the relocation target.
    }
    else if (symbol_index == 0 && reloc_type == irelative_type) {
      base_kind = RELOC_BASE_IRELATIVE;
    }
    else if (symbol_index != 0 &&
        symbolic_64_reloc_type_for_arch(program->arch, reloc_type)) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: symbolic relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }
      uint64_t symbol_value = 0;
      uint64_t addend = reloc_value;
      base_kind = RELOC_BASE_LOAD_BIAS;
      if (resolve_reloc_symbol(program, &dynsym, symbol_index,
            &symbol_value, &base_kind) < 0)
        return -1;
      reloc_value = symbol_value + addend;
    }
    else {
      fprintf(stderr, "POLYCALL_FAIL: unsupported dynamic relocation type=%llu sym=%llu path=%s\n",
        (unsigned long long) reloc_type,
        (unsigned long long) symbol_index,
        program->path);
      return -1;
    }
    if (append_dynamic_reloc(program, relocation_offset, reloc_value,
          base_kind) < 0)
      return -1;
  }
  return 0;
}

static int append_relr_relocation(struct poly_program *program,
    uint64_t reloc_vaddr) {
  size_t relocation_offset = 0;
  if (elf_vaddr_to_image_offset(program, reloc_vaddr, 8,
        &relocation_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: RELR relocation target out of image: %s\n",
      program->path);
    return -1;
  }
  return append_dynamic_reloc(program, relocation_offset,
    read_le64(program->image + relocation_offset), RELOC_BASE_LOAD_BIAS);
}

static int process_relr_table(struct poly_program *program,
    uint64_t relr_vaddr, uint64_t relr_size, uint64_t relr_ent) {
  if (relr_size == 0)
    return 0;
  if (relr_ent != sizeof(uint64_t) || relr_size % sizeof(uint64_t) != 0) {
    fprintf(stderr, "POLYCALL_FAIL: bad RELR dynamic table: %s\n",
      program->path);
    return -1;
  }

  size_t relr_offset = 0;
  if (elf_vaddr_to_image_offset(program, relr_vaddr, relr_size,
        &relr_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: RELR table out of loaded image: %s\n",
      program->path);
    return -1;
  }

  const uint64_t *relr = (const uint64_t *) (program->image + relr_offset);
  const size_t relr_count = (size_t) (relr_size / sizeof(uint64_t));
  uint64_t where = 0;
  for (size_t n = 0; n < relr_count; n++) {
    const uint64_t entry = relr[n];
    if ((entry & 1) == 0) {
      where = entry;
      if (append_relr_relocation(program, where) < 0)
        return -1;
      where += sizeof(uint64_t);
      continue;
    }

    uint64_t bitmap = entry >> 1;
    for (unsigned bit = 0; bit < 63; bit++) {
      if ((bitmap & (UINT64_C(1) << bit)) != 0 &&
          append_relr_relocation(program,
            where + (uint64_t) bit * sizeof(uint64_t)) < 0)
        return -1;
    }
    where += 63 * sizeof(uint64_t);
  }
  return 0;
}

static int load_dynamic_relocs(struct poly_program *program,
    const unsigned char *data, size_t size, const Elf64_Ehdr *ehdr,
    const Elf64_Dyn *dyn, size_t dyn_count) {
  uint64_t rela_vaddr = 0;
  uint64_t rela_size = 0;
  uint64_t rela_ent = sizeof(Elf64_Rela);
  uint64_t rel_vaddr = 0;
  uint64_t rel_size = 0;
  uint64_t rel_ent = sizeof(Elf64_Rel);
  uint64_t relr_vaddr = 0;
  uint64_t relr_size = 0;
  uint64_t relr_ent = sizeof(uint64_t);
  uint64_t jmprel_vaddr = 0;
  uint64_t pltrel_size = 0;
  uint64_t pltrel_type = 0;
  int saw_rel = 0;
  int saw_rela = 0;

  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_NULL:
        n = dyn_count;
        break;
      case DT_RELA:
        rela_vaddr = dyn[n].d_un.d_ptr;
        saw_rela = 1;
        break;
      case DT_RELASZ:
        rela_size = dyn[n].d_un.d_val;
        break;
      case DT_RELAENT:
        rela_ent = dyn[n].d_un.d_val;
        break;
      case DT_REL:
        rel_vaddr = dyn[n].d_un.d_ptr;
        saw_rel = 1;
        break;
      case DT_RELSZ:
        rel_size = dyn[n].d_un.d_val;
        break;
      case DT_RELENT:
        rel_ent = dyn[n].d_un.d_val;
        break;
      case DT_RELR:
        relr_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_RELRSZ:
        relr_size = dyn[n].d_un.d_val;
        break;
      case DT_RELRENT:
        relr_ent = dyn[n].d_un.d_val;
        break;
      case DT_JMPREL:
        jmprel_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_PLTRELSZ:
        pltrel_size = dyn[n].d_un.d_val;
        break;
      case DT_PLTREL:
        pltrel_type = dyn[n].d_un.d_val;
        break;
      case DT_SYMBOLIC:
        program->symbolic_binding = 1;
        break;
      case DT_FLAGS:
        if ((dyn[n].d_un.d_val & DF_SYMBOLIC) != 0)
          program->symbolic_binding = 1;
        break;
      case DT_FLAGS_1:
        if ((dyn[n].d_un.d_val & DF_1_INITFIRST) != 0)
          program->init_first = 1;
        break;
      case DT_PREINIT_ARRAY:
        program->preinit_array_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_PREINIT_ARRAYSZ:
        program->preinit_array_size = dyn[n].d_un.d_val;
        break;
      case DT_INIT:
        program->init_vaddr = dyn[n].d_un.d_ptr;
        program->init_count = 1;
        break;
      case DT_INIT_ARRAY:
        program->init_array_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_INIT_ARRAYSZ:
        program->init_array_size = dyn[n].d_un.d_val;
        break;
      case DT_FINI:
        program->fini_vaddr = dyn[n].d_un.d_ptr;
        program->fini_count = 1;
        break;
      case DT_FINI_ARRAY:
        program->fini_array_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_FINI_ARRAYSZ:
        program->fini_array_size = dyn[n].d_un.d_val;
        break;
      default:
        break;
    }
  }

  if (rel_ent != sizeof(Elf64_Rel) ||
      (rel_size != 0 && (!saw_rel || rel_size % sizeof(Elf64_Rel) != 0))) {
    fprintf(stderr, "POLYCALL_FAIL: bad REL dynamic table: %s\n",
      program->path);
    return -1;
  }
  if (rela_ent != sizeof(Elf64_Rela) ||
      (rela_size != 0 && (!saw_rela || rela_size % sizeof(Elf64_Rela) != 0))) {
    fprintf(stderr, "POLYCALL_FAIL: bad RELA dynamic table: %s\n",
      program->path);
    return -1;
  }
  if (pltrel_size != 0 &&
      (jmprel_vaddr == 0 ||
       (pltrel_type != DT_RELA && pltrel_type != DT_REL) ||
       (pltrel_type == DT_RELA && pltrel_size % sizeof(Elf64_Rela) != 0) ||
       (pltrel_type == DT_REL && pltrel_size % sizeof(Elf64_Rel) != 0))) {
    fprintf(stderr, "POLYCALL_FAIL: bad JMPREL dynamic table: %s\n",
      program->path);
    return -1;
  }
  if (program->preinit_array_size != 0 &&
      (program->preinit_array_vaddr == 0 ||
       program->preinit_array_size % sizeof(uint64_t) != 0)) {
    fprintf(stderr, "POLYCALL_FAIL: bad PREINIT_ARRAY dynamic table: %s\n",
      program->path);
    return -1;
  }
  program->preinit_count =
    (size_t) (program->preinit_array_size / sizeof(uint64_t));
  if (program->init_array_size != 0 &&
      (program->init_array_vaddr == 0 ||
       program->init_array_size % sizeof(uint64_t) != 0)) {
    fprintf(stderr, "POLYCALL_FAIL: bad INIT_ARRAY dynamic table: %s\n",
      program->path);
    return -1;
  }
  program->init_count += (size_t) (program->init_array_size / sizeof(uint64_t));
  if (program->fini_array_size != 0 &&
      (program->fini_array_vaddr == 0 ||
       program->fini_array_size % sizeof(uint64_t) != 0)) {
    fprintf(stderr, "POLYCALL_FAIL: bad FINI_ARRAY dynamic table: %s\n",
      program->path);
    return -1;
  }
  program->fini_count += (size_t) (program->fini_array_size / sizeof(uint64_t));

  if (rela_size != 0 &&
      process_rela_table(program, data, size, ehdr, dyn, dyn_count,
        rela_vaddr, rela_size, "RELA") < 0)
    return -1;
  if (rel_size != 0 &&
      process_rel_table(program, data, size, ehdr, dyn, dyn_count,
        rel_vaddr, rel_size, "REL") < 0)
    return -1;
  if (relr_size != 0 &&
      process_relr_table(program, relr_vaddr, relr_size, relr_ent) < 0)
    return -1;
  if (pltrel_size != 0 &&
      ((pltrel_type == DT_RELA &&
        process_rela_table(program, data, size, ehdr, dyn, dyn_count,
          jmprel_vaddr, pltrel_size, "JMPREL") < 0) ||
       (pltrel_type == DT_REL &&
        process_rel_table(program, data, size, ehdr, dyn, dyn_count,
          jmprel_vaddr, pltrel_size, "JMPREL") < 0)))
      return -1;
  return 0;
}

static int load_elf_program(const char *path, const char *symbol_name,
    struct poly_program *program) {
  memset(program, 0, sizeof(*program));
  program->path = path;

  unsigned char *data = NULL;
  size_t size = 0;
  if (read_file(path, &data, &size) < 0)
    return -1;

  if (size < sizeof(Elf64_Ehdr)) {
    fprintf(stderr, "POLYCALL_FAIL: ELF too small: %s\n", path);
    free(data);
    return -1;
  }

  const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *) data;
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
      ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
      (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) ||
      detect_arch(ehdr->e_machine, program) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported ELF header: %s\n", path);
    free(data);
    return -1;
  }
  if (load_bridge_specs(program->path, data, size, ehdr,
        program->root_bridge_specs, &program->root_bridge_spec_count) < 0) {
    free(data);
    return -1;
  }

  if (ehdr->e_phentsize < sizeof(Elf64_Phdr) ||
      ehdr->e_phoff > size ||
      (uint64_t) ehdr->e_phnum * ehdr->e_phentsize > size - ehdr->e_phoff) {
    fprintf(stderr, "POLYCALL_FAIL: bad ELF program header table: %s\n", path);
    free(data);
    return -1;
  }
  program->elf_type = ehdr->e_type;

  uint64_t base_vaddr = UINT64_MAX;
  uint64_t limit_vaddr = 0;
  int found_load = 0;
  uint64_t dynamic_vaddr = 0;
  uint64_t dynamic_size = 0;

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type == PT_DYNAMIC) {
      dynamic_vaddr = phdr->p_vaddr;
      dynamic_size = phdr->p_filesz;
      continue;
    }
    if (phdr->p_type == PT_GNU_RELRO) {
      program->relro_vaddr = phdr->p_vaddr;
      program->relro_size = phdr->p_memsz;
      continue;
    }
    if (phdr->p_type == PT_TLS) {
      if (phdr->p_filesz > phdr->p_memsz ||
          phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
          phdr->p_memsz > MAX_TLS_BYTES) {
        fprintf(stderr, "POLYCALL_FAIL: bad ELF TLS segment: %s\n", path);
        free(data);
        return -1;
      }
      program->tls_vaddr = phdr->p_vaddr;
      program->tls_filesz = phdr->p_filesz;
      program->tls_memsz = phdr->p_memsz;
      program->tls_align = phdr->p_align;
      continue;
    }
    if (phdr->p_type != PT_LOAD)
      continue;
    if (phdr->p_filesz > phdr->p_memsz ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
        phdr->p_vaddr > UINT64_MAX - phdr->p_memsz) {
      fprintf(stderr, "POLYCALL_FAIL: bad ELF load segment: %s\n", path);
      free(data);
      return -1;
    }
    uint64_t segment_base = align_down_u64(phdr->p_vaddr, 0x1000);
    uint64_t segment_limit = phdr->p_vaddr + phdr->p_memsz;
    if (record_load_segment(program->load_segments,
          &program->load_segment_count, phdr, path) < 0) {
      free(data);
      return -1;
    }
    if (segment_base < base_vaddr)
      base_vaddr = segment_base;
    if (segment_limit > limit_vaddr)
      limit_vaddr = segment_limit;
    found_load = 1;
  }

  if (!found_load || limit_vaddr <= base_vaddr ||
      limit_vaddr - base_vaddr > MAX_PROGRAM_BYTES - 4) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported ELF load image: %s\n", path);
    free(data);
    return -1;
  }
  program->tls_offset = 0;
  program->tls_total_size = (size_t) program->tls_memsz;

  program->base_vaddr = base_vaddr;
  program->image_size = (size_t) (limit_vaddr - base_vaddr + 4);
  program->image = calloc(1, program->image_size);
  if (!program->image) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory loading %s\n", path);
    free(data);
    return -1;
  }

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD)
      continue;
    uint64_t load_offset = phdr->p_vaddr - base_vaddr;
    if (load_offset > program->image_size ||
        phdr->p_filesz > program->image_size - load_offset) {
      fprintf(stderr, "POLYCALL_FAIL: ELF load segment out of range: %s\n", path);
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
    memcpy(program->image + load_offset, data + phdr->p_offset, (size_t) phdr->p_filesz);
    program->loaded_bytes += (size_t) phdr->p_filesz;
  }

  const Elf64_Dyn *early_dynamic = NULL;
  size_t early_dynamic_count = 0;
  if (dynamic_size != 0 && dynamic_size % sizeof(Elf64_Dyn) == 0) {
    size_t dynamic_offset = 0;
    if (elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size,
          &dynamic_offset) == 0) {
      early_dynamic = (const Elf64_Dyn *) (program->image + dynamic_offset);
      early_dynamic_count = (size_t) (dynamic_size / sizeof(Elf64_Dyn));
      (void) load_dynsym_from_dynamic(program, early_dynamic,
        early_dynamic_count, &program->root_dynsym);
    }
  }

  uint64_t entry_vaddr = ehdr->e_entry;
  if (symbol_name && symbol_name[0] != '\0') {
    int resolved = -1;
    if (early_dynamic)
      resolved = resolve_dynamic_symbol(program, early_dynamic,
        early_dynamic_count, symbol_name, &entry_vaddr);
    if (resolved < 0)
      resolved = resolve_elf_symbol_from_sections(data, size, ehdr,
        symbol_name, &entry_vaddr);
    if (resolved < 0) {
      fprintf(stderr, "POLYCALL_FAIL: symbol not found: %s\n", symbol_name);
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
  }
  uint64_t fini_result_vaddr = 0;
  int fini_result_base_kind = RELOC_BASE_ABSOLUTE;
  int fini_result_resolved = -1;
  if (program->root_dynsym.symbols)
    fini_result_resolved = resolve_root_symbol(program, "poly_fini_result",
      NULL, &fini_result_vaddr, &fini_result_base_kind);
  if (fini_result_resolved < 0) {
    fini_result_resolved = resolve_elf_symbol_from_sections(data, size, ehdr,
      "poly_fini_result", &fini_result_vaddr);
    fini_result_base_kind = RELOC_BASE_ROOT_LOAD_BIAS;
  }
  if (fini_result_resolved == 0) {
    program->fini_result_vaddr = fini_result_vaddr;
    program->fini_result_base_kind = fini_result_base_kind;
    program->fini_result_bridge_kind = cross_bridge_kind_for_specs(
      program->root_bridge_specs, program->root_bridge_spec_count,
      "poly_fini_result");
  }

  int entry_in_exec = 0;
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type == PT_LOAD && (phdr->p_flags & PF_X) &&
        entry_vaddr >= phdr->p_vaddr &&
        entry_vaddr < phdr->p_vaddr + phdr->p_filesz) {
      entry_in_exec = 1;
      break;
    }
  }
  if (!entry_in_exec || entry_vaddr < base_vaddr || entry_vaddr >= limit_vaddr) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported ELF entry image: %s\n", path);
    free(program->image);
    program->image = NULL;
    free(data);
    return -1;
  }
  program->entry_offset = (size_t) (entry_vaddr - base_vaddr);

  if (dynamic_size != 0) {
    if (dynamic_size % sizeof(Elf64_Dyn) != 0) {
      fprintf(stderr, "POLYCALL_FAIL: bad dynamic segment size: %s\n", path);
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
    size_t dynamic_offset = 0;
    if (elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size,
          &dynamic_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: dynamic segment out of loaded image: %s\n",
        path);
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
    const Elf64_Dyn *dynamic =
      (const Elf64_Dyn *) (program->image + dynamic_offset);
    const size_t dynamic_count = (size_t) (dynamic_size / sizeof(Elf64_Dyn));
    (void) load_dynsym_from_dynamic(program, dynamic, dynamic_count,
      &program->root_dynsym);
    if (load_preload_dependencies(program) < 0 ||
        load_needed_dependencies(program, dynamic, dynamic_count) < 0 ||
        load_dynamic_relocs(program, data, size, ehdr, dynamic,
          dynamic_count) < 0) {
      free(program->relocs);
      program->relocs = NULL;
      program->reloc_count = 0;
      for (size_t d = 0; d < program->dep_count; d++) {
        free(program->deps[d].image);
        program->deps[d].image = NULL;
        free(program->deps[d].relocs);
        program->deps[d].relocs = NULL;
      }
      program->dep_count = 0;
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
  }

  free(data);
  return 0;
}

extern uint64_t polycall_guarded_call_u64(const uint8_t *code,
    uint64_t *bad_mask);
extern uint64_t polycall_guarded_call_one_u64(const uint8_t *code,
    uint64_t arg, uint64_t *bad_mask);
extern uint64_t polycall_guarded_call_two_u64(const uint8_t *code,
    uint64_t arg0, uint64_t arg1, uint64_t *bad_mask);
extern uint64_t polycall_guarded_call_three_u64(const uint8_t *code,
    uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t *bad_mask);

__asm__(
  ".text\n"
  ".globl polycall_guarded_call_u64\n"
  ".type polycall_guarded_call_u64,@function\n"
  "polycall_guarded_call_u64:\n"
  "  pushq %rbx\n"
  "  pushq %rbp\n"
  "  pushq %r12\n"
  "  pushq %r13\n"
  "  pushq %r14\n"
  "  pushq %r15\n"
  "  subq $16, %rsp\n"
  "  movq %rsi, 8(%rsp)\n"
  "  movq %rdi, %r10\n"
  "  movabsq $0x1badd00d00000001, %rbx\n"
  "  movabsq $0x1badd00d00000002, %rbp\n"
  "  movabsq $0x1badd00d00000003, %r12\n"
  "  movabsq $0x1badd00d00000004, %r13\n"
  "  movabsq $0x1badd00d00000005, %r14\n"
  "  movabsq $0x1badd00d00000006, %r15\n"
  "  movq $1, %rdi\n"
  "  movq $2, %rsi\n"
  "  movq $3, %rdx\n"
  "  movq $4, %rcx\n"
  "  movq $5, %r8\n"
  "  movq $6, %r9\n"
  "  pushq $9\n"
  "  pushq $8\n"
  "  pushq $7\n"
  "  call *%r10\n"
  "  addq $24, %rsp\n"
  "  movq %rax, (%rsp)\n"
  "  xorl %ecx, %ecx\n"
  "  movabsq $0x1badd00d00000001, %rdx\n"
  "  cmpq %rdx, %rbx\n"
  "  je 1f\n"
  "  orq $1, %rcx\n"
  "1:\n"
  "  movabsq $0x1badd00d00000002, %rdx\n"
  "  cmpq %rdx, %rbp\n"
  "  je 2f\n"
  "  orq $2, %rcx\n"
  "2:\n"
  "  movabsq $0x1badd00d00000003, %rdx\n"
  "  cmpq %rdx, %r12\n"
  "  je 3f\n"
  "  orq $4, %rcx\n"
  "3:\n"
  "  movabsq $0x1badd00d00000004, %rdx\n"
  "  cmpq %rdx, %r13\n"
  "  je 4f\n"
  "  orq $8, %rcx\n"
  "4:\n"
  "  movabsq $0x1badd00d00000005, %rdx\n"
  "  cmpq %rdx, %r14\n"
  "  je 5f\n"
  "  orq $16, %rcx\n"
  "5:\n"
  "  movabsq $0x1badd00d00000006, %rdx\n"
  "  cmpq %rdx, %r15\n"
  "  je 6f\n"
  "  orq $32, %rcx\n"
  "6:\n"
  "  movq 8(%rsp), %rdx\n"
  "  movq %rcx, (%rdx)\n"
  "  movq (%rsp), %rax\n"
  "  addq $16, %rsp\n"
  "  popq %r15\n"
  "  popq %r14\n"
  "  popq %r13\n"
  "  popq %r12\n"
  "  popq %rbp\n"
  "  popq %rbx\n"
  "  ret\n"
  ".size polycall_guarded_call_u64, .-polycall_guarded_call_u64\n"
  ".globl polycall_guarded_call_one_u64\n"
  ".type polycall_guarded_call_one_u64,@function\n"
  "polycall_guarded_call_one_u64:\n"
  "  pushq %rbx\n"
  "  pushq %rbp\n"
  "  pushq %r12\n"
  "  pushq %r13\n"
  "  pushq %r14\n"
  "  pushq %r15\n"
  "  subq $16, %rsp\n"
  "  movq %rdx, 8(%rsp)\n"
  "  movq %rdi, %r10\n"
  "  movabsq $0x1badd00d00000001, %rbx\n"
  "  movabsq $0x1badd00d00000002, %rbp\n"
  "  movabsq $0x1badd00d00000003, %r12\n"
  "  movabsq $0x1badd00d00000004, %r13\n"
  "  movabsq $0x1badd00d00000005, %r14\n"
  "  movabsq $0x1badd00d00000006, %r15\n"
  "  movq %rsi, %rdi\n"
  "  xorq %rsi, %rsi\n"
  "  xorq %rdx, %rdx\n"
  "  xorq %rcx, %rcx\n"
  "  xorq %r8, %r8\n"
  "  xorq %r9, %r9\n"
  "  call *%r10\n"
  "  movq %rax, (%rsp)\n"
  "  xorl %ecx, %ecx\n"
  "  movabsq $0x1badd00d00000001, %rdx\n"
  "  cmpq %rdx, %rbx\n"
  "  je 7f\n"
  "  orq $1, %rcx\n"
  "7:\n"
  "  movabsq $0x1badd00d00000002, %rdx\n"
  "  cmpq %rdx, %rbp\n"
  "  je 8f\n"
  "  orq $2, %rcx\n"
  "8:\n"
  "  movabsq $0x1badd00d00000003, %rdx\n"
  "  cmpq %rdx, %r12\n"
  "  je 9f\n"
  "  orq $4, %rcx\n"
  "9:\n"
  "  movabsq $0x1badd00d00000004, %rdx\n"
  "  cmpq %rdx, %r13\n"
  "  je 10f\n"
  "  orq $8, %rcx\n"
  "10:\n"
  "  movabsq $0x1badd00d00000005, %rdx\n"
  "  cmpq %rdx, %r14\n"
  "  je 11f\n"
  "  orq $16, %rcx\n"
  "11:\n"
  "  movabsq $0x1badd00d00000006, %rdx\n"
  "  cmpq %rdx, %r15\n"
  "  je 12f\n"
  "  orq $32, %rcx\n"
  "12:\n"
  "  movq 8(%rsp), %rdx\n"
  "  movq %rcx, (%rdx)\n"
  "  movq (%rsp), %rax\n"
  "  addq $16, %rsp\n"
  "  popq %r15\n"
  "  popq %r14\n"
  "  popq %r13\n"
  "  popq %r12\n"
  "  popq %rbp\n"
  "  popq %rbx\n"
  "  ret\n"
  ".size polycall_guarded_call_one_u64, .-polycall_guarded_call_one_u64\n"
  ".globl polycall_guarded_call_two_u64\n"
  ".type polycall_guarded_call_two_u64,@function\n"
  "polycall_guarded_call_two_u64:\n"
  "  pushq %rbx\n"
  "  pushq %rbp\n"
  "  pushq %r12\n"
  "  pushq %r13\n"
  "  pushq %r14\n"
  "  pushq %r15\n"
  "  subq $16, %rsp\n"
  "  movq %rcx, 8(%rsp)\n"
  "  movq %rdi, %r10\n"
  "  movabsq $0x1badd00d00000001, %rbx\n"
  "  movabsq $0x1badd00d00000002, %rbp\n"
  "  movabsq $0x1badd00d00000003, %r12\n"
  "  movabsq $0x1badd00d00000004, %r13\n"
  "  movabsq $0x1badd00d00000005, %r14\n"
  "  movabsq $0x1badd00d00000006, %r15\n"
  "  movq %rsi, %rdi\n"
  "  movq %rdx, %rsi\n"
  "  xorq %rdx, %rdx\n"
  "  xorq %rcx, %rcx\n"
  "  xorq %r8, %r8\n"
  "  xorq %r9, %r9\n"
  "  call *%r10\n"
  "  movq %rax, (%rsp)\n"
  "  xorl %ecx, %ecx\n"
  "  movabsq $0x1badd00d00000001, %rdx\n"
  "  cmpq %rdx, %rbx\n"
  "  je 13f\n"
  "  orq $1, %rcx\n"
  "13:\n"
  "  movabsq $0x1badd00d00000002, %rdx\n"
  "  cmpq %rdx, %rbp\n"
  "  je 14f\n"
  "  orq $2, %rcx\n"
  "14:\n"
  "  movabsq $0x1badd00d00000003, %rdx\n"
  "  cmpq %rdx, %r12\n"
  "  je 15f\n"
  "  orq $4, %rcx\n"
  "15:\n"
  "  movabsq $0x1badd00d00000004, %rdx\n"
  "  cmpq %rdx, %r13\n"
  "  je 16f\n"
  "  orq $8, %rcx\n"
  "16:\n"
  "  movabsq $0x1badd00d00000005, %rdx\n"
  "  cmpq %rdx, %r14\n"
  "  je 17f\n"
  "  orq $16, %rcx\n"
  "17:\n"
  "  movabsq $0x1badd00d00000006, %rdx\n"
  "  cmpq %rdx, %r15\n"
  "  je 18f\n"
  "  orq $32, %rcx\n"
  "18:\n"
  "  movq 8(%rsp), %rdx\n"
  "  movq %rcx, (%rdx)\n"
  "  movq (%rsp), %rax\n"
  "  addq $16, %rsp\n"
  "  popq %r15\n"
  "  popq %r14\n"
  "  popq %r13\n"
  "  popq %r12\n"
  "  popq %rbp\n"
  "  popq %rbx\n"
  "  ret\n"
  ".size polycall_guarded_call_two_u64, .-polycall_guarded_call_two_u64\n"
  ".globl polycall_guarded_call_three_u64\n"
  ".type polycall_guarded_call_three_u64,@function\n"
  "polycall_guarded_call_three_u64:\n"
  "  pushq %rbx\n"
  "  pushq %rbp\n"
  "  pushq %r12\n"
  "  pushq %r13\n"
  "  pushq %r14\n"
  "  pushq %r15\n"
  "  subq $16, %rsp\n"
  "  movq %r8, 8(%rsp)\n"
  "  movq %rdi, %r10\n"
  "  movabsq $0x1badd00d00000001, %rbx\n"
  "  movabsq $0x1badd00d00000002, %rbp\n"
  "  movabsq $0x1badd00d00000003, %r12\n"
  "  movabsq $0x1badd00d00000004, %r13\n"
  "  movabsq $0x1badd00d00000005, %r14\n"
  "  movabsq $0x1badd00d00000006, %r15\n"
  "  movq %rsi, %rdi\n"
  "  movq %rdx, %rsi\n"
  "  movq %rcx, %rdx\n"
  "  xorq %rcx, %rcx\n"
  "  xorq %r8, %r8\n"
  "  xorq %r9, %r9\n"
  "  call *%r10\n"
  "  movq %rax, (%rsp)\n"
  "  xorl %ecx, %ecx\n"
  "  movabsq $0x1badd00d00000001, %rdx\n"
  "  cmpq %rdx, %rbx\n"
  "  je 19f\n"
  "  orq $1, %rcx\n"
  "19:\n"
  "  movabsq $0x1badd00d00000002, %rdx\n"
  "  cmpq %rdx, %rbp\n"
  "  je 20f\n"
  "  orq $2, %rcx\n"
  "20:\n"
  "  movabsq $0x1badd00d00000003, %rdx\n"
  "  cmpq %rdx, %r12\n"
  "  je 21f\n"
  "  orq $4, %rcx\n"
  "21:\n"
  "  movabsq $0x1badd00d00000004, %rdx\n"
  "  cmpq %rdx, %r13\n"
  "  je 22f\n"
  "  orq $8, %rcx\n"
  "22:\n"
  "  movabsq $0x1badd00d00000005, %rdx\n"
  "  cmpq %rdx, %r14\n"
  "  je 23f\n"
  "  orq $16, %rcx\n"
  "23:\n"
  "  movabsq $0x1badd00d00000006, %rdx\n"
  "  cmpq %rdx, %r15\n"
  "  je 24f\n"
  "  orq $32, %rcx\n"
  "24:\n"
  "  movq 8(%rsp), %rdx\n"
  "  movq %rcx, (%rdx)\n"
  "  movq (%rsp), %rax\n"
  "  addq $16, %rsp\n"
  "  popq %r15\n"
  "  popq %r14\n"
  "  popq %r13\n"
  "  popq %r12\n"
  "  popq %rbp\n"
  "  popq %rbx\n"
  "  ret\n"
  ".size polycall_guarded_call_three_u64, .-polycall_guarded_call_three_u64\n"
);

static void fail_callee_save_guard(uint64_t mask) {
  fprintf(stderr,
    "POLYCALL_FAIL: x86 callee-saved register clobber mask=0x%llx\n",
    (unsigned long long) mask);
  exit(1);
}

static uint64_t call_poly_stub(uint8_t *code, size_t target_imm_offset,
    uint64_t target, int call_kind) {
  write_le64(code + target_imm_offset, target);
  struct pair_u64 {
    uint64_t lo;
    uint64_t hi;
  };
  struct sret_u64 {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
  };
  struct pair_fp64 {
    double lo;
    double hi;
  };
  struct hfa3_fp64 {
    double a;
    double b;
    double c;
  };
  struct hfa4_fp64 {
    double a;
    double b;
    double c;
    double d;
  };
  struct hfa3_fp32 {
    float a;
    float b;
    float c;
  };
  struct hfa4_fp32 {
    float a;
    float b;
    float c;
    float d;
  };
  struct pair_fp32 {
    float lo;
    float hi;
  };
  struct hetero_u64_f64 {
    uint64_t i;
    double d;
  };
  struct hetero_f64_u64 {
    double d;
    uint64_t i;
  };
  struct hetero_u64_f32 {
    uint64_t i;
    float f;
  };
  struct hetero_f32_u64 {
    float f;
    uint64_t i;
  };
  struct hetero_u32_f64 {
    uint32_t i;
    double d;
  };
  struct hetero_f64_u32 {
    double d;
    uint32_t i;
  };
  struct hetero_u32_f32 {
    uint32_t i;
    float f;
  };
  struct hetero_f32_u32 {
    float f;
    uint32_t i;
  };
  typedef uint32_t vec128_u32 __attribute__((vector_size(16)));
  union vec128_u32_bits {
    vec128_u32 v;
    uint32_t u[4];
  };
  if (call_kind == POLY_CALL_FP64) {
    union {
      double d;
      uint64_t u;
    } fp_result;
    double (*entry)(double, double, double) =
      (double (*)(double, double, double)) code;
    fp_result.d = entry(1.5, 2.25, 3.0);
    return fp_result.u;
  }
  if (call_kind == POLY_CALL_FP32) {
    union {
      float f;
      uint32_t u;
    } fp_result;
    float (*entry)(float, float, float) =
      (float (*)(float, float, float)) code;
    fp_result.f = entry(1.5f, 2.25f, 3.0f);
    return fp_result.u;
  }
  if (call_kind == POLY_CALL_FP64_STACK) {
    union {
      double d;
      uint64_t u;
    } fp_result;
    double (*entry)(double, double, double, double, double, double, double,
        double, double, double, double, double, double, double, double,
        double) =
      (double (*)(double, double, double, double, double, double, double,
        double, double, double, double, double, double, double, double,
        double)) code;
    fp_result.d = entry(1.0, 2.0, 3.0, 4.0, 5.0,
      6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0,
      13.0, 14.0, 15.0, 16.0);
    return fp_result.u;
  }
  if (call_kind == POLY_CALL_VEC128_U32) {
    vec128_u32 (*entry)(vec128_u32, vec128_u32) =
      (vec128_u32 (*)(vec128_u32, vec128_u32)) code;
    union vec128_u32_bits arg0 = { .u = { 1, 2, 3, 4 } };
    union vec128_u32_bits arg1 = { .u = { 10, 20, 30, 40 } };
    union vec128_u32_bits result;
    result.v = entry(arg0.v, arg1.v);
    return ((uint64_t) (result.u[3] & 0xffffU) << 48) |
      ((uint64_t) (result.u[2] & 0xffffU) << 32) |
      ((uint64_t) (result.u[1] & 0xffffU) << 16) |
      (uint64_t) (result.u[0] & 0xffffU);
  }
  if (call_kind == POLY_CALL_ONE_U64) {
    uint64_t bad_mask = 0;
    uint64_t result = polycall_guarded_call_one_u64(code, 0, &bad_mask);
    if (bad_mask != 0)
      fail_callee_save_guard(bad_mask);
    return result;
  }
  if (call_kind == POLY_CALL_PAIR_U64) {
    struct pair_u64 (*entry)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
      uint64_t, uint64_t, uint64_t, uint64_t) =
      (struct pair_u64 (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t)) code;
    struct pair_u64 pair_result = entry(1, 2, 3, 4, 5, 6, 7, 8, 9);
    return ((pair_result.hi & 0xffffffffULL) << 32) |
      (pair_result.lo & 0xffffffffULL);
  }
  if (call_kind == POLY_CALL_SRET_U64) {
    struct sret_u64 (*entry)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
      uint64_t, uint64_t, uint64_t) =
      (struct sret_u64 (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t)) code;
    struct sret_u64 sret_result = entry(1, 2, 3, 4, 5, 6, 7, 8);
    return ((sret_result.a & 0xffffULL) << 48) |
      ((sret_result.b & 0xffffULL) << 32) |
      ((sret_result.c & 0xffffULL) << 16) |
      (sret_result.d & 0xffffULL);
  }
  if (call_kind == POLY_CALL_FPAIR64) {
    union {
      double d;
      uint64_t u;
    } lo_bits, hi_bits;
    struct pair_fp64 (*entry)(double, double, double) =
      (struct pair_fp64 (*)(double, double, double)) code;
    struct pair_fp64 pair_result = entry(1.5, 2.25, 3.0);
    lo_bits.d = pair_result.lo;
    hi_bits.d = pair_result.hi;
    return (lo_bits.u & 0xffffffff00000000ULL) | (hi_bits.u >> 32);
  }
  if (call_kind == POLY_CALL_AARCH64_HFA3_F64) {
    union {
      double d;
      uint64_t u;
    } a_bits, b_bits, c_bits;
    struct hfa3_fp64 (*entry)(double, double, double) =
      (struct hfa3_fp64 (*)(double, double, double)) code;
    struct hfa3_fp64 hfa_result = entry(1.5, 2.25, 3.0);
    a_bits.d = hfa_result.a;
    b_bits.d = hfa_result.b;
    c_bits.d = hfa_result.c;
    return ((a_bits.u >> 48) << 32) |
      (((b_bits.u >> 48) & 0xffffULL) << 16) |
      ((c_bits.u >> 48) & 0xffffULL);
  }
  if (call_kind == POLY_CALL_AARCH64_HFA4_F64) {
    union {
      double d;
      uint64_t u;
    } a_bits, b_bits, c_bits, d_bits;
    struct hfa4_fp64 (*entry)(double, double, double) =
      (struct hfa4_fp64 (*)(double, double, double)) code;
    struct hfa4_fp64 hfa_result = entry(1.5, 2.25, 3.0);
    a_bits.d = hfa_result.a;
    b_bits.d = hfa_result.b;
    c_bits.d = hfa_result.c;
    d_bits.d = hfa_result.d;
    return ((a_bits.u >> 48) << 48) |
      (((b_bits.u >> 48) & 0xffffULL) << 32) |
      (((c_bits.u >> 48) & 0xffffULL) << 16) |
      ((d_bits.u >> 48) & 0xffffULL);
  }
  if (call_kind == POLY_CALL_AARCH64_HFA3_F32) {
    union {
      float f;
      uint32_t u;
    } a_bits, b_bits, c_bits;
    struct hfa3_fp32 (*entry)(float, float, float) =
      (struct hfa3_fp32 (*)(float, float, float)) code;
    struct hfa3_fp32 hfa_result = entry(1.5f, 2.25f, 3.0f);
    a_bits.f = hfa_result.a;
    b_bits.f = hfa_result.b;
    c_bits.f = hfa_result.c;
    return ((uint64_t) (a_bits.u >> 16) << 32) |
      (((uint64_t) (b_bits.u >> 16) & 0xffffULL) << 16) |
      ((uint64_t) (c_bits.u >> 16) & 0xffffULL);
  }
  if (call_kind == POLY_CALL_AARCH64_HFA4_F32) {
    union {
      float f;
      uint32_t u;
    } a_bits, b_bits, c_bits, d_bits;
    struct hfa4_fp32 (*entry)(float, float, float) =
      (struct hfa4_fp32 (*)(float, float, float)) code;
    struct hfa4_fp32 hfa_result = entry(1.5f, 2.25f, 3.0f);
    a_bits.f = hfa_result.a;
    b_bits.f = hfa_result.b;
    c_bits.f = hfa_result.c;
    d_bits.f = hfa_result.d;
    return ((uint64_t) (a_bits.u >> 16) << 48) |
      (((uint64_t) (b_bits.u >> 16) & 0xffffULL) << 32) |
      (((uint64_t) (c_bits.u >> 16) & 0xffffULL) << 16) |
      ((uint64_t) (d_bits.u >> 16) & 0xffffULL);
  }
  if (call_kind == POLY_CALL_AARCH64_HFA3_F64_ARG) {
    struct hfa3_fp64 arg;
    arg.a = 1.5;
    arg.b = 2.25;
    arg.c = 3.0;
    uint64_t (*entry)(struct hfa3_fp64) =
      (uint64_t (*)(struct hfa3_fp64)) code;
    return entry(arg);
  }
  if (call_kind == POLY_CALL_AARCH64_HFA4_F64_ARG) {
    struct hfa4_fp64 arg;
    arg.a = 1.5;
    arg.b = 2.25;
    arg.c = 3.0;
    arg.d = 4.0;
    uint64_t (*entry)(struct hfa4_fp64) =
      (uint64_t (*)(struct hfa4_fp64)) code;
    return entry(arg);
  }
  if (call_kind == POLY_CALL_AARCH64_HFA3_F32_ARG) {
    struct hfa3_fp32 arg;
    arg.a = 1.5f;
    arg.b = 2.25f;
    arg.c = 3.0f;
    uint64_t (*entry)(struct hfa3_fp32) =
      (uint64_t (*)(struct hfa3_fp32)) code;
    return entry(arg);
  }
  if (call_kind == POLY_CALL_AARCH64_HFA4_F32_ARG) {
    struct hfa4_fp32 arg;
    arg.a = 1.5f;
    arg.b = 2.25f;
    arg.c = 3.0f;
    arg.d = 4.0f;
    uint64_t (*entry)(struct hfa4_fp32) =
      (uint64_t (*)(struct hfa4_fp32)) code;
    return entry(arg);
  }
  if (call_kind == POLY_CALL_FPAIR32) {
    union {
      float f;
      uint32_t u;
    } lo_bits, hi_bits;
    struct pair_fp32 (*entry)(float, float, float) =
      (struct pair_fp32 (*)(float, float, float)) code;
    struct pair_fp32 pair_result = entry(1.5f, 2.25f, 3.0f);
    lo_bits.f = pair_result.lo;
    hi_bits.f = pair_result.hi;
    return ((uint64_t) hi_bits.u << 32) | lo_bits.u;
  }
  if (call_kind == POLY_CALL_FPAIR32_ARG) {
    union {
      float f;
      uint32_t u;
    } fp_result;
    struct pair_fp32 pair_arg;
    pair_arg.lo = 1.5f;
    pair_arg.hi = 2.25f;
    float (*entry)(struct pair_fp32, float) =
      (float (*)(struct pair_fp32, float)) code;
    fp_result.f = entry(pair_arg, 3.0f);
    return fp_result.u;
  }
  if (call_kind == POLY_CALL_FPAIR64_ARG) {
    union {
      double d;
      uint64_t u;
    } fp_result;
    struct pair_fp64 pair_arg;
    pair_arg.lo = 1.5;
    pair_arg.hi = 2.25;
    double (*entry)(struct pair_fp64, double) =
      (double (*)(struct pair_fp64, double)) code;
    fp_result.d = entry(pair_arg, 3.0);
    return fp_result.u;
  }
  if (call_kind == POLY_CALL_MIXED_ARGS) {
    union {
      double d;
      uint64_t u;
    } fp_result;
    double (*entry)(uint64_t, double, uint64_t, double, uint64_t, double) =
      (double (*)(uint64_t, double, uint64_t, double, uint64_t, double)) code;
    fp_result.d = entry(1, 1.5, 2, 2.25, 3, 3.0);
    return fp_result.u;
  }
  if (call_kind == POLY_CALL_HETERO_U64_F64) {
    union {
      double d;
      uint64_t u;
    } fp_bits;
    struct hetero_u64_f64 (*entry)(struct hetero_u64_f64, uint64_t) =
      (struct hetero_u64_f64 (*)(struct hetero_u64_f64, uint64_t)) code;
    struct hetero_u64_f64 arg;
    arg.i = 3;
    arg.d = 2.25;
    struct hetero_u64_f64 result = entry(arg, 5);
    fp_bits.d = result.d;
    return ((result.i & 0xffffULL) << 48) |
      ((fp_bits.u >> 16) & 0x0000ffffffffffffULL);
  }
  if (call_kind == POLY_CALL_HETERO_F64_U64) {
    union {
      double d;
      uint64_t u;
    } fp_bits;
    struct hetero_f64_u64 (*entry)(struct hetero_f64_u64, uint64_t) =
      (struct hetero_f64_u64 (*)(struct hetero_f64_u64, uint64_t)) code;
    struct hetero_f64_u64 arg;
    arg.d = 2.25;
    arg.i = 3;
    struct hetero_f64_u64 result = entry(arg, 5);
    fp_bits.d = result.d;
    return ((result.i & 0xffffULL) << 48) |
      ((fp_bits.u >> 16) & 0x0000ffffffffffffULL);
  }
  if (call_kind == POLY_CALL_HETERO_U64_F32) {
    union {
      float f;
      uint32_t u;
    } fp_bits;
    struct hetero_u64_f32 (*entry)(struct hetero_u64_f32, uint64_t) =
      (struct hetero_u64_f32 (*)(struct hetero_u64_f32, uint64_t)) code;
    struct hetero_u64_f32 arg;
    arg.i = 3;
    arg.f = 2.25f;
    struct hetero_u64_f32 result = entry(arg, 5);
    fp_bits.f = result.f;
    return ((result.i & 0xffffffffULL) << 32) | fp_bits.u;
  }
  if (call_kind == POLY_CALL_HETERO_F32_U64) {
    union {
      float f;
      uint32_t u;
    } fp_bits;
    struct hetero_f32_u64 (*entry)(struct hetero_f32_u64, uint64_t) =
      (struct hetero_f32_u64 (*)(struct hetero_f32_u64, uint64_t)) code;
    struct hetero_f32_u64 arg;
    arg.f = 2.25f;
    arg.i = 3;
    struct hetero_f32_u64 result = entry(arg, 5);
    fp_bits.f = result.f;
    return ((result.i & 0xffffffffULL) << 32) | fp_bits.u;
  }
  if (call_kind == POLY_CALL_HETERO_U32_F64) {
    union {
      double d;
      uint64_t u;
    } fp_bits;
    struct hetero_u32_f64 (*entry)(struct hetero_u32_f64, uint32_t) =
      (struct hetero_u32_f64 (*)(struct hetero_u32_f64, uint32_t)) code;
    struct hetero_u32_f64 arg;
    arg.i = 3;
    arg.d = 2.25;
    struct hetero_u32_f64 result = entry(arg, 5);
    fp_bits.d = result.d;
    return ((uint64_t) (result.i & 0xffffU) << 48) |
      ((fp_bits.u >> 16) & 0x0000ffffffffffffULL);
  }
  if (call_kind == POLY_CALL_HETERO_F64_U32) {
    union {
      double d;
      uint64_t u;
    } fp_bits;
    struct hetero_f64_u32 (*entry)(struct hetero_f64_u32, uint32_t) =
      (struct hetero_f64_u32 (*)(struct hetero_f64_u32, uint32_t)) code;
    struct hetero_f64_u32 arg;
    arg.d = 2.25;
    arg.i = 3;
    struct hetero_f64_u32 result = entry(arg, 5);
    fp_bits.d = result.d;
    return ((uint64_t) (result.i & 0xffffU) << 48) |
      ((fp_bits.u >> 16) & 0x0000ffffffffffffULL);
  }
  if (call_kind == POLY_CALL_COMPACT_U32_F32) {
    union {
      float f;
      uint32_t u;
    } fp_bits;
    struct hetero_u32_f32 (*entry)(struct hetero_u32_f32, uint32_t) =
      (struct hetero_u32_f32 (*)(struct hetero_u32_f32, uint32_t)) code;
    struct hetero_u32_f32 arg;
    arg.i = 3;
    arg.f = 2.25f;
    struct hetero_u32_f32 result = entry(arg, 5);
    fp_bits.f = result.f;
    return ((uint64_t) fp_bits.u << 32) | result.i;
  }
  if (call_kind == POLY_CALL_COMPACT_F32_U32) {
    union {
      float f;
      uint32_t u;
    } fp_bits;
    struct hetero_f32_u32 (*entry)(struct hetero_f32_u32, uint32_t) =
      (struct hetero_f32_u32 (*)(struct hetero_f32_u32, uint32_t)) code;
    struct hetero_f32_u32 arg;
    arg.f = 2.25f;
    arg.i = 3;
    struct hetero_f32_u32 result = entry(arg, 5);
    fp_bits.f = result.f;
    return ((uint64_t) result.i << 32) | fp_bits.u;
  }

  uint64_t bad_mask = 0;
  uint64_t result = polycall_guarded_call_u64(code, &bad_mask);
  if (bad_mask != 0)
    fail_callee_save_guard(bad_mask);
  return result;
}

static void call_poly_stub_one_u64(uint8_t *code, size_t target_imm_offset,
    uint64_t target, uint64_t arg)
{
  uint64_t bad_mask = 0;
  write_le64(code + target_imm_offset, target);
  (void) polycall_guarded_call_one_u64(code, arg, &bad_mask);
  if (bad_mask != 0)
    fail_callee_save_guard(bad_mask);
}

int64_t poly_runtime_call_foreign_comparator(void *callback,
    const void *left, const void *right)
{
  if (poly_atexit_call_code == NULL || callback == NULL)
    return 0;

  uint64_t bad_mask = 0;
  write_le64(poly_atexit_call_code + poly_atexit_target_imm_offset,
    (uint64_t) (uintptr_t) callback);
  uint64_t result = polycall_guarded_call_two_u64(poly_atexit_call_code,
    (uint64_t) (uintptr_t) left, (uint64_t) (uintptr_t) right, &bad_mask);
  if (bad_mask != 0)
    fail_callee_save_guard(bad_mask);
  return (int32_t) result;
}

int64_t poly_runtime_call_foreign_comparator_arg(void *callback,
    const void *left, const void *right, void *arg)
{
  if (poly_atexit_call_code == NULL || callback == NULL)
    return 0;

  uint64_t bad_mask = 0;
  write_le64(poly_atexit_call_code + poly_atexit_target_imm_offset,
    (uint64_t) (uintptr_t) callback);
  uint64_t result = polycall_guarded_call_three_u64(poly_atexit_call_code,
    (uint64_t) (uintptr_t) left, (uint64_t) (uintptr_t) right,
    (uint64_t) (uintptr_t) arg, &bad_mask);
  if (bad_mask != 0)
    fail_callee_save_guard(bad_mask);
  return (int32_t) result;
}

uint64_t poly_runtime_call_foreign_void(void *callback)
{
  if (poly_atexit_call_code == NULL || callback == NULL)
    return 1;

  uint64_t bad_mask = 0;
  write_le64(poly_atexit_call_code + poly_atexit_target_imm_offset,
    (uint64_t) (uintptr_t) callback);
  (void) polycall_guarded_call_one_u64(poly_atexit_call_code, 0, &bad_mask);
  if (bad_mask != 0)
    fail_callee_save_guard(bad_mask);
  return 0;
}

uint64_t poly_runtime_finalize_atexit_callbacks(void *dso_handle)
{
  if (poly_atexit_call_code == NULL)
    return 1;

  const uint64_t dso = (uint64_t) (uintptr_t) dso_handle;
  for (size_t n = poly_atexit_callback_count; n > 0; n--) {
    struct poly_atexit_callback *entry = &poly_atexit_callbacks[n - 1];
    if (!entry->active || (dso != 0 && entry->dso_handle != dso))
      continue;
    entry->active = 0;
    call_poly_stub_one_u64(poly_atexit_call_code,
      poly_atexit_target_imm_offset, entry->callback, entry->arg);
  }
  return 0;
}

static void run_poly_atexit_callbacks(uint8_t *code, size_t target_imm_offset)
{
  poly_runtime_set_atexit_call_stub(code, target_imm_offset);
  (void) poly_runtime_finalize_atexit_callbacks(NULL);
}

static void unmap_dependency_images(uint8_t **dep_foreign,
    const size_t *dep_sizes, size_t dep_count) {
  for (size_t n = 0; n < dep_count; n++) {
    if (dep_foreign[n])
      munmap(dep_foreign[n], dep_sizes[n]);
  }
}

static uint8_t *map_foreign_program_image(const struct poly_program *program,
    size_t foreign_size) {
  if (program->elf_type == ET_EXEC) {
    uint8_t *fixed = mmap((void *) (uintptr_t) program->base_vaddr,
      foreign_size, PROT_READ | PROT_WRITE | PROT_EXEC,
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (fixed != MAP_FAILED || errno != EEXIST)
      return fixed;
  }
  return mmap(NULL, foreign_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

static int load_segment_prot(uint32_t flags) {
  int prot = 0;
  if ((flags & PF_R) != 0)
    prot |= PROT_READ;
  if ((flags & PF_W) != 0)
    prot |= PROT_WRITE;
  if ((flags & PF_X) != 0)
    prot |= PROT_EXEC;
  return prot;
}

static int protect_load_segments(const char *path, uint8_t *image,
    size_t image_size, uint64_t base_vaddr,
    const struct poly_load_segment *segments, size_t segment_count) {
  for (size_t n = 0; n < segment_count; n++) {
    if (segments[n].memsz == 0)
      continue;
    if (segments[n].vaddr > UINT64_MAX - segments[n].memsz) {
      fprintf(stderr, "POLYCALL_FAIL: bad PT_LOAD range: %s\n", path);
      return -1;
    }
    size_t segment_offset = 0;
    if (image_vaddr_to_offset(base_vaddr, image_size, segments[n].vaddr,
          segments[n].memsz, &segment_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: PT_LOAD escaped image: %s\n", path);
      return -1;
    }

    const uint64_t segment_start = align_down_u64(segments[n].vaddr, 0x1000);
    const uint64_t segment_end_unaligned = segments[n].vaddr +
      segments[n].memsz;
    if (segment_end_unaligned > UINT64_MAX - 0xfff) {
      fprintf(stderr, "POLYCALL_FAIL: bad PT_LOAD page range: %s\n", path);
      return -1;
    }
    const uint64_t segment_end = align_up_u64(segment_end_unaligned, 0x1000);
    if (segment_start < base_vaddr || segment_end < segment_start ||
        segment_end - segment_start > SIZE_MAX) {
      fprintf(stderr, "POLYCALL_FAIL: bad PT_LOAD page range: %s\n", path);
      return -1;
    }

    void *addr = image + (size_t) (segment_start - base_vaddr);
    const size_t length = (size_t) (segment_end - segment_start);
    if (mprotect(addr, length, load_segment_prot(segments[n].flags)) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: PT_LOAD mprotect failed: %s: %s\n",
        path, strerror(errno));
      return -1;
    }
  }
  return 0;
}

static int protect_relro_region(const char *path, uint8_t *image,
    size_t image_size, uint64_t base_vaddr, uint64_t relro_vaddr,
    uint64_t relro_size) {
  if (relro_size == 0)
    return 0;
  if (relro_vaddr > UINT64_MAX - relro_size) {
    fprintf(stderr, "POLYCALL_FAIL: bad RELRO range: %s\n", path);
    return -1;
  }
  size_t relro_offset = 0;
  if (image_vaddr_to_offset(base_vaddr, image_size, relro_vaddr, relro_size,
        &relro_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: RELRO escaped image: %s\n", path);
    return -1;
  }

  const uint64_t relro_start = align_down_u64(relro_vaddr, 0x1000);
  const uint64_t relro_end_unaligned = relro_vaddr + relro_size;
  if (relro_end_unaligned > UINT64_MAX - 0xfff) {
    fprintf(stderr, "POLYCALL_FAIL: bad RELRO page range: %s\n", path);
    return -1;
  }
  const uint64_t relro_end = align_up_u64(relro_end_unaligned, 0x1000);
  if (relro_start < base_vaddr || relro_end < relro_start ||
      relro_end - relro_start > SIZE_MAX) {
    fprintf(stderr, "POLYCALL_FAIL: bad RELRO page range: %s\n", path);
    return -1;
  }
  void *addr = image + (size_t) (relro_start - base_vaddr);
  const size_t length = (size_t) (relro_end - relro_start);
  if (mprotect(addr, length, PROT_READ) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: RELRO mprotect failed: %s: %s\n",
      path, strerror(errno));
    return -1;
  }
  return 0;
}

static int call_target_from_root_arch(uint8_t *code, size_t target_imm_offset,
    uint8_t *cross_stubs, size_t cross_stub_size, size_t *cross_stub_offset,
    int root_arch, int target_arch, uint64_t target, int call_kind,
    int bridge_kind, const char *path, const char *label, uint64_t *result) {
  uint64_t call_target = target;
  if (root_arch != target_arch &&
      emit_cross_isa_call_stub(cross_stubs, cross_stub_size,
        cross_stub_offset, root_arch, target_arch, target,
        bridge_kind, &call_target) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: %s cross-ISA call stub overflow: %s\n",
      label, path);
    return -1;
  }
  const uint64_t value = call_poly_stub(code, target_imm_offset, call_target,
    call_kind);
  if (result)
    *result = value;
  return 0;
}

static int call_dependency_init_callbacks(const struct poly_dependency *dep,
    uint8_t *dep_image, size_t dep_size, uint64_t dep_load_bias,
    uint8_t *code, size_t target_imm_offset, uint8_t *cross_stubs,
    size_t cross_stub_size, size_t *cross_stub_offset, int root_arch) {
  if (dep->init_vaddr != 0) {
    const uint64_t init_target = dep_load_bias + dep->init_vaddr;
    if (call_target_from_root_arch(code, target_imm_offset, cross_stubs,
          cross_stub_size, cross_stub_offset, root_arch, dep->arch,
          init_target, POLY_CALL_U64, POLY_CROSS_BRIDGE_DEFAULT, dep->path,
          "dependency INIT", NULL) < 0)
      return -1;
  }
  if (dep->init_array_size == 0)
    return 0;

  size_t init_array_offset = 0;
  if (image_vaddr_to_offset(dep->base_vaddr, dep_size, dep->init_array_vaddr,
        dep->init_array_size, &init_array_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: dependency INIT_ARRAY escaped image: %s\n",
      dep->path);
    return -1;
  }
  const size_t init_array_count =
    (size_t) (dep->init_array_size / sizeof(uint64_t));
  for (size_t n = 0; n < init_array_count; n++) {
    uint64_t init_target = read_le64(dep_image + init_array_offset + n * 8);
    if (init_target != 0 &&
        call_target_from_root_arch(code, target_imm_offset, cross_stubs,
          cross_stub_size, cross_stub_offset, root_arch, dep->arch,
          init_target, POLY_CALL_U64, POLY_CROSS_BRIDGE_DEFAULT, dep->path,
          "dependency INIT_ARRAY", NULL) < 0)
      return -1;
  }
  return 0;
}

static int emit_and_call(const struct poly_program *program, int call_kind,
    unsigned repeat_count, uint64_t *result) {
  const uint32_t fallback_ret = fallback_ret_for_arch(program->arch);
  const int needs_x86_import = program->needs_x86_import;
  struct poly_import_contract import_contract = {
    .call_base = POLY_IMPORT_CALL_BASE,
    .call_stride = POLY_IMPORT_CALL_STRIDE,
    .import_count = POLY_IMPORT_FUNC_COUNT,
    .x86_slot0 = POLY_IMPORT_FUNC_X86_SLOT0,
    .x86_slot_count = POLY_IMPORT_FUNC_X86_SLOT7 - POLY_IMPORT_FUNC_X86_SLOT0 + 1,
    .x86_descriptor_size = POLY_X86_IMPORT_DESCRIPTOR_SIZE
  };
  if (read_poly_base_contract() < 0)
    return -1;
  if (read_poly_abi_bridge_contract(&import_contract) < 0)
    return -1;
  if (needs_x86_import && read_poly_import_contract(&import_contract) < 0)
    return -1;
  const size_t callee_save_size = 33;
  const size_t callee_restore_size = 33;
  const size_t callee_save_area_size = 48;
  const size_t import_setup_size = needs_x86_import ? 10 : 0;
  const size_t tls_setup_size = 10;
  const size_t heap_setup_size = 10;
  const size_t pcall_return_offset = callee_save_size + 10 + 10 +
    tls_setup_size + heap_setup_size + import_setup_size + 8;
  const size_t main_stub_size = pcall_return_offset + callee_restore_size + 1;
  const size_t import_return_size = needs_x86_import ? 8 : 0;
  const size_t import_descriptor_count = needs_x86_import ?
    import_contract.import_count : 0;
  const size_t import_descriptor_size = needs_x86_import ?
    import_descriptor_count * import_contract.x86_descriptor_size : 0;
  const size_t stub_size = main_stub_size + import_return_size +
    import_descriptor_size;
  const size_t cross_stub_size = 4096;
  const size_t cross_stub_base_offset =
    (stub_size + callee_save_area_size + 7U) & ~((size_t) 7U);
  const size_t code_size = cross_stub_base_offset + cross_stub_size;
  const size_t foreign_size = program->image_size;
  uint8_t *dep_foreign[MAX_NEEDED_DEPS];
  uint64_t dep_load_bias[MAX_NEEDED_DEPS];
  size_t dep_sizes[MAX_NEEDED_DEPS];
  memset(dep_foreign, 0, sizeof(dep_foreign));
  memset(dep_load_bias, 0, sizeof(dep_load_bias));
  memset(dep_sizes, 0, sizeof(dep_sizes));
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYCALL_FAIL: x86 stub mmap failed: %s\n", strerror(errno));
    return -1;
  }
  uint8_t *foreign = map_foreign_program_image(program, foreign_size);
  if (foreign == MAP_FAILED) {
    fprintf(stderr, "POLYCALL_FAIL: foreign mmap failed: %s\n", strerror(errno));
    munmap(code, code_size);
    return -1;
  }
  uint8_t *import_page = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (import_page == MAP_FAILED) {
    fprintf(stderr, "POLYCALL_FAIL: import mmap failed: %s\n", strerror(errno));
    munmap(foreign, foreign_size);
    munmap(code, code_size);
    return -1;
  }
  uint8_t *heap = mmap(NULL, POLY_IMPORT_HEAP_SIZE, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (heap == MAP_FAILED) {
    fprintf(stderr, "POLYCALL_FAIL: heap mmap failed: %s\n", strerror(errno));
    munmap(import_page, 4096);
    munmap(foreign, foreign_size);
    munmap(code, code_size);
    return -1;
  }
  uint8_t *tls = NULL;
  size_t tls_size = 0;
  if (program->tls_total_size != 0 || program->needs_errno_location) {
    tls_size = program->tls_total_size;
    if (program->needs_errno_location && tls_size < POLY_ERRNO_TLS_SIZE)
      tls_size = POLY_ERRNO_TLS_SIZE;
    tls = mmap(NULL, tls_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (tls == MAP_FAILED) {
      fprintf(stderr, "POLYCALL_FAIL: TLS mmap failed: %s\n", strerror(errno));
      munmap(heap, POLY_IMPORT_HEAP_SIZE);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
  }
  write_le64(import_page + POLY_IMPORT_PAGE_VALUE_OFFSET, poly_import_value);
  write_le64(import_page + POLY_IMPORT_PAGE_STACK_GUARD_OFFSET,
    poly_stack_chk_guard);
  write_le64(import_page + POLY_IMPORT_PAGE_HEAP_BASE_OFFSET,
    (uint64_t) (uintptr_t) heap);
  write_le64(import_page + POLY_IMPORT_PAGE_HEAP_SIZE_OFFSET,
    POLY_IMPORT_HEAP_SIZE);
  write_le64(import_page + POLY_IMPORT_PAGE_HEAP_CURSOR_OFFSET, 0);

  size_t offset = 0;
  const uint64_t return_rip = (uint64_t) (uintptr_t) (code + pcall_return_offset);
  const uint64_t import_x86_return = (uint64_t) (uintptr_t) (code + main_stub_size);
  const uint64_t import_x86_table = import_x86_return + import_return_size;
  const size_t import_x86_table_offset = main_stub_size + import_return_size;
  const uint64_t callee_save_area = (uint64_t) (uintptr_t) (code + stub_size);
  uint8_t *cross_stubs = code + cross_stub_base_offset;
  size_t cross_stub_offset = 0;
  const uint64_t foreign_target = (uint64_t) (uintptr_t) (foreign + program->entry_offset);
  emit_save_callee_regs(code, &offset, callee_save_area);
  const size_t target_imm_offset = offset + 2;
  emit_movabs_r10(code, &offset, 0);
  emit_movabs_r11(code, &offset, return_rip);
  const size_t tls_imm_offset = offset + 2;
  emit_movabs_r13(code, &offset, 0);
  const size_t heap_imm_offset = offset + 2;
  emit_movabs_r14(code, &offset, 0);
  if (needs_x86_import) {
    emit_movabs_r12(code, &offset, import_x86_table);
  }
  const size_t pcall_opcode_offset = offset;
  if (program->arch == POLY_ARCH_AARCH64) {
    uint8_t pcall_op = 0x10;
    if (call_kind == POLY_CALL_FPAIR32)
      pcall_op = 0x14;
    else if (call_kind == POLY_CALL_FPAIR32_ARG)
      pcall_op = 0x16;
    else if (call_kind == POLY_CALL_HETERO_U64_F64 ||
        call_kind == POLY_CALL_HETERO_U32_F64)
      pcall_op = 0x18;
    else if (call_kind == POLY_CALL_HETERO_F64_U64 ||
        call_kind == POLY_CALL_HETERO_F64_U32)
      pcall_op = 0x19;
    else if (call_kind == POLY_CALL_HETERO_U64_F32)
      pcall_op = 0x1a;
    else if (call_kind == POLY_CALL_HETERO_F32_U64)
      pcall_op = 0x1b;
    else if (call_kind == POLY_CALL_FP64_STACK)
      pcall_op = 0x1e;
    else if (call_kind == POLY_CALL_VEC128_U32)
      pcall_op = 0x21;
    else if (call_kind == POLY_CALL_AARCH64_HFA3_F64)
      pcall_op = 0x23;
    else if (call_kind == POLY_CALL_AARCH64_HFA4_F64)
      pcall_op = 0x24;
    else if (call_kind == POLY_CALL_AARCH64_HFA3_F32)
      pcall_op = 0x25;
    else if (call_kind == POLY_CALL_AARCH64_HFA4_F32)
      pcall_op = 0x26;
    else if (call_kind == POLY_CALL_AARCH64_HFA3_F64_ARG)
      pcall_op = 0x27;
    else if (call_kind == POLY_CALL_AARCH64_HFA4_F64_ARG)
      pcall_op = 0x28;
    else if (call_kind == POLY_CALL_AARCH64_HFA3_F32_ARG)
      pcall_op = 0x29;
    else if (call_kind == POLY_CALL_AARCH64_HFA4_F32_ARG)
      pcall_op = 0x2a;
    const uint8_t pcall[] = {
      0x0f, 0x24, pcall_op,
      0x50, 0x4f, 0x4c, 0x59, 0x21
    };
    memcpy(code + offset, pcall, sizeof(pcall));
    offset += sizeof(pcall);
  }
  else {
    uint8_t pcall_op = 0x11;
    if (call_kind == POLY_CALL_FPAIR32)
      pcall_op = 0x15;
    else if (call_kind == POLY_CALL_FPAIR32_ARG)
      pcall_op = 0x17;
    else if (call_kind == POLY_CALL_COMPACT_U32_F32)
      pcall_op = 0x1c;
    else if (call_kind == POLY_CALL_COMPACT_F32_U32)
      pcall_op = 0x1d;
    else if (call_kind == POLY_CALL_FP64_STACK)
      pcall_op = 0x1f;
    else if (call_kind == POLY_CALL_VEC128_U32)
      pcall_op = 0x22;
    const uint8_t pcall[] = {
      0x0f, 0x24, pcall_op,
      0x50, 0x4f, 0x4c, 0x59, 0x21
    };
    memcpy(code + offset, pcall, sizeof(pcall));
    offset += sizeof(pcall);
  }
  emit_restore_callee_regs(code, &offset, callee_save_area);
  code[offset++] = 0xc3;
  if (needs_x86_import) {
    const uint8_t import_return[] = { 0x0f, 0x24, 0x20, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, import_return, sizeof(import_return));
    offset += sizeof(import_return);
    for (size_t n = 0; n < import_descriptor_count; n++) {
      emit_u64(code, &offset, 0);
      emit_u64(code, &offset, 0);
      emit_u64(code, &offset, 0);
      emit_u64(code, &offset, 0);
    }
    for (uint64_t import_id = 0; import_id < import_contract.import_count;
         import_id++) {
      const uint64_t target =
        x86_descriptor_target_for_import_id(program->arch, import_id);
      if (target == 0)
        continue;
      const size_t descriptor_offset = import_x86_table_offset +
        (size_t) import_id * import_contract.x86_descriptor_size;
      write_le64(code + descriptor_offset, target);
      write_le64(code + descriptor_offset + 8, import_x86_return);
      write_le64(code + descriptor_offset + 16,
        x86_descriptor_flags_for_import_id(program->arch, import_id));
      write_le64(code + descriptor_offset + 24,
        x86_descriptor_stack_arg_count_for_import_id(import_id));
    }
  }
  if (offset != stub_size) {
    fprintf(stderr, "POLYCALL_FAIL: internal x86 stub size mismatch\n");
    if (tls)
      munmap(tls, tls_size);
    munmap(import_page, 4096);
    munmap(foreign, foreign_size);
    munmap(code, code_size);
    return -1;
  }
  memcpy(foreign, program->image, program->image_size);
  const uint64_t root_load_bias =
    (uint64_t) (uintptr_t) foreign - program->base_vaddr;
  for (size_t n = 0; n < program->dep_count; n++) {
    dep_sizes[n] = program->deps[n].image_size;
    dep_foreign[n] = mmap(NULL, dep_sizes[n], PROT_READ | PROT_WRITE | PROT_EXEC,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (dep_foreign[n] == MAP_FAILED) {
      dep_foreign[n] = NULL;
      fprintf(stderr, "POLYCALL_FAIL: dependency mmap failed: %s\n",
        strerror(errno));
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    memcpy(dep_foreign[n], program->deps[n].image,
      program->deps[n].image_size);
    size_t dep_offset = program->deps[n].image_size - 4;
    emit_u32(dep_foreign[n], &dep_offset,
      fallback_ret_for_arch(program->deps[n].arch));
    dep_load_bias[n] = (uint64_t) (uintptr_t) dep_foreign[n] -
      program->deps[n].base_vaddr;
  }
  size_t max_dep_depth = 0;
  for (size_t d = 0; d < program->dep_count; d++) {
    if (program->deps[d].needed_depth > max_dep_depth)
      max_dep_depth = program->deps[d].needed_depth;
  }
  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_dependency *dep = &program->deps[d];
    for (size_t r = 0; r < dep->reloc_count; r++) {
      if (reloc_base_is_resolver(dep->relocs[r].base_kind))
        continue;
      if (dep->relocs[r].base_kind >= RELOC_BASE_DEP_COPY &&
          dep->relocs[r].base_kind <
            RELOC_BASE_DEP_COPY + (int) program->dep_count) {
        const size_t source_dep =
          (size_t) (dep->relocs[r].base_kind - RELOC_BASE_DEP_COPY);
        size_t source_offset = 0;
        if (dep->relocs[r].offset > dep_sizes[d] ||
            dep->relocs[r].size > dep_sizes[d] - dep->relocs[r].offset ||
            image_vaddr_to_offset(program->deps[source_dep].base_vaddr,
              dep_sizes[source_dep], dep->relocs[r].value,
              dep->relocs[r].size, &source_offset) < 0) {
          fprintf(stderr, "POLYCALL_FAIL: dependency copy relocation escaped image: %s\n",
            dep->path);
          unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
          if (tls)
            munmap(tls, tls_size);
          munmap(import_page, 4096);
          munmap(foreign, foreign_size);
          munmap(code, code_size);
          return -1;
        }
        memcpy(dep_foreign[d] + dep->relocs[r].offset,
          dep_foreign[source_dep] + source_offset, dep->relocs[r].size);
        continue;
      }
      if (dep->relocs[r].offset > dep_sizes[d] ||
          dep_sizes[d] - dep->relocs[r].offset < 8) {
        fprintf(stderr, "POLYCALL_FAIL: dependency relocation target escaped image: %s\n",
          dep->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      uint64_t reloc_base = 0;
      if (dep->relocs[r].base_kind == RELOC_BASE_ABSOLUTE)
        reloc_base = 0;
      else if (dep->relocs[r].base_kind == RELOC_BASE_LOAD_BIAS)
        reloc_base = dep_load_bias[d];
      else if (dep->relocs[r].base_kind == RELOC_BASE_ROOT_LOAD_BIAS)
        reloc_base = root_load_bias;
      else if (reloc_base_is_root_cross_stub(dep->relocs[r].base_kind)) {
        uint64_t stub_addr = 0;
        if (emit_cross_isa_call_stub(cross_stubs, cross_stub_size,
              &cross_stub_offset, dep->arch, program->arch,
              root_load_bias + dep->relocs[r].value,
              cross_bridge_kind_for_base(dep->relocs[r].base_kind),
              &stub_addr) < 0) {
          fprintf(stderr, "POLYCALL_FAIL: dependency root cross-ISA stub overflow: %s\n",
            dep->path);
          unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
          if (tls)
            munmap(tls, tls_size);
          munmap(import_page, 4096);
          munmap(foreign, foreign_size);
          munmap(code, code_size);
          return -1;
        }
        write_le64(dep_foreign[d] + dep->relocs[r].offset, stub_addr);
        continue;
      }
      else if (dep->relocs[r].base_kind == RELOC_BASE_IMPORT_PAGE)
        reloc_base = (uint64_t) (uintptr_t) import_page;
      else if (dep->relocs[r].base_kind == RELOC_BASE_IMPORT_CALL)
        reloc_base = import_contract.call_base;
      else if (dep->relocs[r].base_kind == RELOC_BASE_TLS_OFFSET)
        reloc_base = program->deps[d].tls_offset;
      else if (dep->relocs[r].base_kind == RELOC_BASE_ROOT_TLS_OFFSET)
        reloc_base = program->tls_offset;
      else if (dep->relocs[r].base_kind >= RELOC_BASE_DEP_TLS_OFFSET &&
          dep->relocs[r].base_kind <
            RELOC_BASE_DEP_TLS_OFFSET + (int) program->dep_count) {
        const size_t dep_index =
          (size_t) (dep->relocs[r].base_kind - RELOC_BASE_DEP_TLS_OFFSET);
        reloc_base = program->deps[dep_index].tls_offset;
      }
      else if (reloc_base_is_dep_cross_stub(dep->relocs[r].base_kind)) {
        const size_t dep_index = dep_cross_stub_index(dep->relocs[r].base_kind);
        uint64_t stub_addr = 0;
        if (emit_cross_isa_call_stub(cross_stubs, cross_stub_size,
              &cross_stub_offset, dep->arch, program->deps[dep_index].arch,
              dep_load_bias[dep_index] + dep->relocs[r].value,
              cross_bridge_kind_for_base(dep->relocs[r].base_kind),
              &stub_addr) < 0) {
          fprintf(stderr, "POLYCALL_FAIL: dependency cross-ISA stub overflow: %s\n",
            dep->path);
          unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
          if (tls)
            munmap(tls, tls_size);
          munmap(import_page, 4096);
          munmap(foreign, foreign_size);
          munmap(code, code_size);
          return -1;
        }
        write_le64(dep_foreign[d] + dep->relocs[r].offset, stub_addr);
        continue;
      }
      else if (dep->relocs[r].base_kind >= RELOC_BASE_DEP_LOAD_BIAS &&
          dep->relocs[r].base_kind <
            RELOC_BASE_DEP_LOAD_BIAS + (int) program->dep_count) {
        const size_t dep_index =
          (size_t) (dep->relocs[r].base_kind - RELOC_BASE_DEP_LOAD_BIAS);
        reloc_base = dep_load_bias[dep_index];
      }
      else {
        fprintf(stderr, "POLYCALL_FAIL: unsupported dependency relocation base: %s\n",
          dep->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      write_le64(dep_foreign[d] + dep->relocs[r].offset,
        dep->relocs[r].value + reloc_base);
    }
  }
  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_dependency *dep = &program->deps[d];
    for (size_t r = 0; r < dep->reloc_count; r++) {
      if (!reloc_base_is_resolver(dep->relocs[r].base_kind))
        continue;
      if (dep->relocs[r].offset > dep_sizes[d] ||
          dep_sizes[d] - dep->relocs[r].offset < 8) {
        fprintf(stderr, "POLYCALL_FAIL: dependency resolver target escaped image: %s\n",
          dep->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      uint64_t resolver = 0;
      int resolver_arch = dep->arch;
      if (dep->relocs[r].base_kind == RELOC_BASE_IRELATIVE)
        resolver = dep_load_bias[d] + dep->relocs[r].value;
      else if (reloc_base_is_root_ifunc(dep->relocs[r].base_kind))
        continue;
      else {
        const size_t resolver_dep = reloc_base_is_dep_cross_ifunc(
            dep->relocs[r].base_kind) ?
          dep_cross_ifunc_index(dep->relocs[r].base_kind) :
          (size_t) (dep->relocs[r].base_kind - RELOC_BASE_DEP_IFUNC);
        resolver_arch = program->deps[resolver_dep].arch;
        resolver = dep_load_bias[resolver_dep] + dep->relocs[r].value;
      }
      uint64_t resolved = 0;
      if (call_target_from_root_arch(code, target_imm_offset, cross_stubs,
            cross_stub_size, &cross_stub_offset, program->arch,
            resolver_arch, resolver, POLY_CALL_U64,
            POLY_CROSS_BRIDGE_DEFAULT, dep->path, "dependency IFUNC resolver",
            &resolved) < 0) {
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      uint64_t reloc_value = resolved;
      if (dep->arch != resolver_arch &&
          emit_cross_isa_call_stub(cross_stubs, cross_stub_size,
            &cross_stub_offset, dep->arch, resolver_arch, resolved,
            cross_bridge_kind_for_ifunc_base(dep->relocs[r].base_kind),
            &reloc_value) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: dependency IFUNC cross-ISA stub overflow: %s\n",
          dep->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      write_le64(dep_foreign[d] + dep->relocs[r].offset, reloc_value);
    }
  }
  if (program->tls_memsz != 0) {
    size_t tls_image_offset = 0;
    if (elf_vaddr_to_image_offset(program, program->tls_vaddr,
          program->tls_filesz, &tls_image_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: TLS image escaped loaded image: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    memcpy(tls + program->tls_offset, foreign + tls_image_offset,
      (size_t) program->tls_filesz);
  }
  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_dependency *dep = &program->deps[d];
    if (dep->tls_memsz == 0)
      continue;
    size_t tls_image_offset = 0;
    if (image_vaddr_to_offset(dep->base_vaddr, dep_sizes[d],
          dep->tls_vaddr, dep->tls_filesz, &tls_image_offset) < 0 ||
        dep->tls_offset > tls_size ||
        dep->tls_filesz > tls_size - dep->tls_offset) {
      fprintf(stderr, "POLYCALL_FAIL: dependency TLS image escaped loaded image: %s\n",
        dep->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    memcpy(tls + dep->tls_offset, dep_foreign[d] + tls_image_offset,
      (size_t) dep->tls_filesz);
  }
  if (tls) {
    write_le64(code + tls_imm_offset, (uint64_t) (uintptr_t) tls);
  }
  else {
    write_le64(code + tls_imm_offset, 0);
  }
  write_le64(code + heap_imm_offset, (uint64_t) (uintptr_t) import_page);
  const uint64_t load_bias = root_load_bias;
  for (size_t n = 0; n < program->reloc_count; n++) {
    if (reloc_base_is_resolver(program->relocs[n].base_kind))
      continue;
    if (program->relocs[n].base_kind >= RELOC_BASE_DEP_COPY &&
        program->relocs[n].base_kind <
          RELOC_BASE_DEP_COPY + (int) program->dep_count) {
      const size_t source_dep =
        (size_t) (program->relocs[n].base_kind - RELOC_BASE_DEP_COPY);
      size_t source_offset = 0;
      if (program->relocs[n].offset > foreign_size ||
          program->relocs[n].size > foreign_size - program->relocs[n].offset ||
          image_vaddr_to_offset(program->deps[source_dep].base_vaddr,
            dep_sizes[source_dep], program->relocs[n].value,
            program->relocs[n].size, &source_offset) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: copy relocation escaped image: %s\n",
          program->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      memcpy(foreign + program->relocs[n].offset,
        dep_foreign[source_dep] + source_offset, program->relocs[n].size);
      continue;
    }
    if (program->relocs[n].offset > foreign_size ||
        foreign_size - program->relocs[n].offset < 8) {
      fprintf(stderr, "POLYCALL_FAIL: relocation target escaped image: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    uint64_t reloc_base = 0;
    if (program->relocs[n].base_kind == RELOC_BASE_ABSOLUTE)
      reloc_base = 0;
    else if (program->relocs[n].base_kind == RELOC_BASE_LOAD_BIAS)
      reloc_base = load_bias;
    else if (program->relocs[n].base_kind == RELOC_BASE_IMPORT_PAGE)
      reloc_base = (uint64_t) (uintptr_t) import_page;
    else if (program->relocs[n].base_kind == RELOC_BASE_IMPORT_CALL)
      reloc_base = import_contract.call_base;
    else if (program->relocs[n].base_kind == RELOC_BASE_TLS_OFFSET)
      reloc_base = program->tls_offset;
    else if (program->relocs[n].base_kind == RELOC_BASE_ROOT_TLS_OFFSET)
      reloc_base = program->tls_offset;
    else if (program->relocs[n].base_kind >= RELOC_BASE_DEP_TLS_OFFSET &&
        program->relocs[n].base_kind <
          RELOC_BASE_DEP_TLS_OFFSET + (int) program->dep_count) {
      const size_t dep_index =
        (size_t) (program->relocs[n].base_kind - RELOC_BASE_DEP_TLS_OFFSET);
      reloc_base = program->deps[dep_index].tls_offset;
    }
    else if (reloc_base_is_dep_cross_stub(program->relocs[n].base_kind)) {
      const size_t dep_index = dep_cross_stub_index(program->relocs[n].base_kind);
      uint64_t stub_addr = 0;
      if (emit_cross_isa_call_stub(cross_stubs, cross_stub_size,
            &cross_stub_offset, program->arch, program->deps[dep_index].arch,
            dep_load_bias[dep_index] + program->relocs[n].value,
            cross_bridge_kind_for_base(program->relocs[n].base_kind),
            &stub_addr) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: cross-ISA stub overflow: %s\n",
          program->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      write_le64(foreign + program->relocs[n].offset, stub_addr);
      continue;
    }
    else if (program->relocs[n].base_kind >= RELOC_BASE_DEP_LOAD_BIAS &&
        program->relocs[n].base_kind <
          RELOC_BASE_DEP_LOAD_BIAS + (int) program->dep_count) {
      const size_t dep_index =
        (size_t) (program->relocs[n].base_kind - RELOC_BASE_DEP_LOAD_BIAS);
      reloc_base = dep_load_bias[dep_index];
    }
    else {
      fprintf(stderr, "POLYCALL_FAIL: unknown relocation base kind: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    const uint64_t reloc_value = program->relocs[n].value + reloc_base;
    write_le64(foreign + program->relocs[n].offset, reloc_value);
  }
  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_dependency *dep = &program->deps[d];
    for (size_t r = 0; r < dep->reloc_count; r++) {
      if (!reloc_base_is_root_ifunc(dep->relocs[r].base_kind))
        continue;
      if (dep->relocs[r].offset > dep_sizes[d] ||
          dep_sizes[d] - dep->relocs[r].offset < 8) {
        fprintf(stderr, "POLYCALL_FAIL: dependency root IFUNC target escaped image: %s\n",
          dep->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      const uint64_t resolver = root_load_bias + dep->relocs[r].value;
      uint64_t resolved = 0;
      if (call_target_from_root_arch(code, target_imm_offset, cross_stubs,
            cross_stub_size, &cross_stub_offset, program->arch,
            program->arch, resolver, POLY_CALL_U64,
            POLY_CROSS_BRIDGE_DEFAULT, dep->path,
            "dependency root IFUNC resolver", &resolved) < 0) {
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      uint64_t reloc_value = resolved;
      if (dep->arch != program->arch &&
          emit_cross_isa_call_stub(cross_stubs, cross_stub_size,
            &cross_stub_offset, dep->arch, program->arch, resolved,
            cross_bridge_kind_for_ifunc_base(dep->relocs[r].base_kind),
            &reloc_value) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: dependency root IFUNC cross-ISA stub overflow: %s\n",
          dep->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      write_le64(dep_foreign[d] + dep->relocs[r].offset, reloc_value);
    }
  }
  offset = program->image_size - 4;
  emit_u32(foreign, &offset, fallback_ret);

  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_dependency *dep = &program->deps[d];
    if (protect_load_segments(dep->path, dep_foreign[d], dep_sizes[d],
          dep->base_vaddr, dep->load_segments, dep->load_segment_count) < 0) {
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
  }
  if (protect_load_segments(program->path, foreign, foreign_size,
        program->base_vaddr, program->load_segments,
        program->load_segment_count) < 0) {
    unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
    if (tls)
      munmap(tls, tls_size);
    munmap(import_page, 4096);
    munmap(foreign, foreign_size);
    munmap(code, code_size);
    return -1;
  }

  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_dependency *dep = &program->deps[d];
    if (protect_relro_region(dep->path, dep_foreign[d], dep_sizes[d],
          dep->base_vaddr, dep->relro_vaddr, dep->relro_size) < 0) {
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
  }
  if (protect_relro_region(program->path, foreign, foreign_size,
        program->base_vaddr, program->relro_vaddr, program->relro_size) < 0) {
    unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
    if (tls)
      munmap(tls, tls_size);
    munmap(import_page, 4096);
    munmap(foreign, foreign_size);
    munmap(code, code_size);
    return -1;
  }

  poly_runtime_reset_atexit_callbacks();
  poly_runtime_set_atexit_call_stub(code, target_imm_offset);

  if (program->preinit_array_size != 0) {
    size_t preinit_array_offset = 0;
    if (elf_vaddr_to_image_offset(program, program->preinit_array_vaddr,
          program->preinit_array_size, &preinit_array_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: PREINIT_ARRAY escaped image: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    for (size_t n = 0; n < program->preinit_count; n++) {
      uint64_t preinit_target = read_le64(foreign + preinit_array_offset +
        n * 8);
      if (preinit_target != 0)
        (void) call_poly_stub(code, target_imm_offset, preinit_target,
          POLY_CALL_U64);
    }
  }

  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_dependency *dep = &program->deps[d];
    if (!dep->init_first)
      continue;
    if (call_dependency_init_callbacks(dep, dep_foreign[d], dep_sizes[d],
          dep_load_bias[d], code, target_imm_offset, cross_stubs,
          cross_stub_size, &cross_stub_offset, program->arch) < 0) {
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
  }

  for (size_t depth = max_dep_depth + 1; depth > 0; depth--) {
    const size_t init_depth = depth - 1;
    for (size_t d = 0; d < program->dep_count; d++) {
      const struct poly_dependency *dep = &program->deps[d];
      if (dep->init_first || dep->needed_depth != init_depth)
        continue;
      if (call_dependency_init_callbacks(dep, dep_foreign[d], dep_sizes[d],
            dep_load_bias[d], code, target_imm_offset, cross_stubs,
            cross_stub_size, &cross_stub_offset, program->arch) < 0) {
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
    }
  }

  for (size_t n = 0; n < program->reloc_count; n++) {
    if (!reloc_base_is_resolver(program->relocs[n].base_kind))
      continue;
    if (program->relocs[n].offset > foreign_size ||
        foreign_size - program->relocs[n].offset < 8) {
      fprintf(stderr, "POLYCALL_FAIL: resolver target escaped image: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    uint64_t resolver = 0;
    int resolver_arch = program->arch;
    if (program->relocs[n].base_kind == RELOC_BASE_IRELATIVE) {
      size_t resolver_offset = 0;
      if (elf_vaddr_to_image_offset(program, program->relocs[n].value, 4,
            &resolver_offset) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: resolver escaped image: %s\n",
          program->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      resolver = load_bias + program->relocs[n].value;
    }
    else if (reloc_base_is_root_ifunc(program->relocs[n].base_kind)) {
      resolver = root_load_bias + program->relocs[n].value;
    }
    else {
      const size_t resolver_dep = reloc_base_is_dep_cross_ifunc(
          program->relocs[n].base_kind) ?
        dep_cross_ifunc_index(program->relocs[n].base_kind) :
        (size_t) (program->relocs[n].base_kind - RELOC_BASE_DEP_IFUNC);
      resolver_arch = program->deps[resolver_dep].arch;
      resolver = dep_load_bias[resolver_dep] + program->relocs[n].value;
    }
    uint64_t resolved = 0;
    if (call_target_from_root_arch(code, target_imm_offset, cross_stubs,
          cross_stub_size, &cross_stub_offset, program->arch, resolver_arch,
          resolver, POLY_CALL_U64, POLY_CROSS_BRIDGE_DEFAULT, program->path,
          "IFUNC resolver", &resolved) < 0) {
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    uint64_t reloc_value = resolved;
    if (program->arch != resolver_arch &&
        emit_cross_isa_call_stub(cross_stubs, cross_stub_size,
          &cross_stub_offset, program->arch, resolver_arch, resolved,
          cross_bridge_kind_for_ifunc_base(program->relocs[n].base_kind),
          &reloc_value) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: IFUNC cross-ISA stub overflow: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    write_le64(foreign + program->relocs[n].offset, reloc_value);
  }

  if (program->init_vaddr != 0) {
    const uint64_t init_target = load_bias + program->init_vaddr;
    (void) call_poly_stub(code, target_imm_offset, init_target, POLY_CALL_U64);
  }
  if (program->init_array_size != 0) {
    size_t init_array_offset = 0;
    if (elf_vaddr_to_image_offset(program, program->init_array_vaddr,
          program->init_array_size, &init_array_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: INIT_ARRAY escaped image: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    const size_t init_array_count =
      (size_t) (program->init_array_size / sizeof(uint64_t));
    for (size_t n = 0; n < init_array_count; n++) {
      uint64_t init_target = read_le64(foreign + init_array_offset + n * 8);
      if (init_target != 0)
        (void) call_poly_stub(code, target_imm_offset, init_target,
          POLY_CALL_U64);
    }
  }
  if (call_kind == POLY_CALL_SRET_U64) {
    if (program->arch == POLY_ARCH_AARCH64) {
      const uint8_t pcall[] = { 0x0f, 0x24, 0x12, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
      memcpy(code + pcall_opcode_offset, pcall, sizeof(pcall));
    }
    else {
      const uint8_t pcall[] = { 0x0f, 0x24, 0x13, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
      memcpy(code + pcall_opcode_offset, pcall, sizeof(pcall));
    }
  }
  const int entry_call_kind = call_kind == POLY_CALL_FINI_RESULT ?
    POLY_CALL_U64 : call_kind;
  if (repeat_count == 0)
    repeat_count = 1;
  for (unsigned repeat = 0; repeat < repeat_count; repeat++) {
    *result = call_poly_stub(code, target_imm_offset, foreign_target,
      entry_call_kind);
  }

  if (program->fini_array_size != 0) {
    size_t fini_array_offset = 0;
    if (elf_vaddr_to_image_offset(program, program->fini_array_vaddr,
          program->fini_array_size, &fini_array_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: FINI_ARRAY escaped image: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    const size_t fini_array_count =
      (size_t) (program->fini_array_size / sizeof(uint64_t));
    for (size_t n = fini_array_count; n > 0; n--) {
      uint64_t fini_target = read_le64(foreign + fini_array_offset +
        (n - 1) * 8);
      if (fini_target != 0)
        (void) call_poly_stub(code, target_imm_offset, fini_target,
          POLY_CALL_U64);
    }
  }
  if (program->fini_vaddr != 0) {
    const uint64_t fini_target = load_bias + program->fini_vaddr;
    (void) call_poly_stub(code, target_imm_offset, fini_target, POLY_CALL_U64);
  }
  run_poly_atexit_callbacks(code, target_imm_offset);
  if (call_kind == POLY_CALL_FINI_RESULT) {
    if (program->fini_result_vaddr == 0) {
      fprintf(stderr, "POLYCALL_FAIL: fini result symbol missing: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    uint64_t fini_result_target = load_bias + program->fini_result_vaddr;
    if (reloc_base_is_root_ifunc(program->fini_result_base_kind))
      fini_result_target = call_poly_stub(code, target_imm_offset,
        fini_result_target, POLY_CALL_U64);
    const int result_call_kind = call_kind_for_bridge_result(program->arch,
      POLY_CALL_U64, program->fini_result_bridge_kind);
    const uint8_t saved_pcall_op = code[pcall_opcode_offset + 2];
    code[pcall_opcode_offset + 2] =
      pcall_opcode_for_call_kind(program->arch, result_call_kind);
    *result = call_poly_stub(code, target_imm_offset, fini_result_target,
      result_call_kind);
    code[pcall_opcode_offset + 2] = saved_pcall_op;
  }
  for (size_t fini_depth = 0; fini_depth <= max_dep_depth; fini_depth++) {
    for (size_t d = program->dep_count; d > 0; d--) {
      const size_t dep_index = d - 1;
      const struct poly_dependency *dep = &program->deps[dep_index];
      if (dep->needed_depth != fini_depth)
        continue;
      if (dep->fini_array_size != 0) {
        if (dep->fini_array_vaddr < dep->base_vaddr ||
            dep->fini_array_size > dep_sizes[dep_index]) {
          fprintf(stderr, "POLYCALL_FAIL: dependency FINI_ARRAY escaped image: %s\n",
            dep->path);
          unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
          if (tls)
            munmap(tls, tls_size);
          munmap(import_page, 4096);
          munmap(foreign, foreign_size);
          munmap(code, code_size);
          return -1;
        }
        const uint64_t fini_array_offset =
          dep->fini_array_vaddr - dep->base_vaddr;
        if (fini_array_offset > dep_sizes[dep_index] ||
            dep->fini_array_size > dep_sizes[dep_index] - fini_array_offset) {
          fprintf(stderr, "POLYCALL_FAIL: dependency FINI_ARRAY escaped image: %s\n",
            dep->path);
          unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
          if (tls)
            munmap(tls, tls_size);
          munmap(import_page, 4096);
          munmap(foreign, foreign_size);
          munmap(code, code_size);
          return -1;
        }
        const size_t fini_array_count =
          (size_t) (dep->fini_array_size / sizeof(uint64_t));
        for (size_t n = fini_array_count; n > 0; n--) {
          uint64_t fini_target = read_le64(dep_foreign[dep_index] +
            fini_array_offset + (n - 1) * 8);
          if (fini_target != 0 &&
              call_target_from_root_arch(code, target_imm_offset, cross_stubs,
                cross_stub_size, &cross_stub_offset, program->arch,
                dep->arch, fini_target, POLY_CALL_U64,
                POLY_CROSS_BRIDGE_DEFAULT, dep->path, "dependency FINI_ARRAY",
                NULL) < 0) {
            unmap_dependency_images(dep_foreign, dep_sizes,
              program->dep_count);
            if (tls)
              munmap(tls, tls_size);
            munmap(import_page, 4096);
            munmap(foreign, foreign_size);
            munmap(code, code_size);
            return -1;
          }
        }
      }
      if (dep->fini_vaddr != 0) {
        const uint64_t fini_target =
          dep_load_bias[dep_index] + dep->fini_vaddr;
        if (call_target_from_root_arch(code, target_imm_offset, cross_stubs,
              cross_stub_size, &cross_stub_offset, program->arch, dep->arch,
              fini_target, POLY_CALL_U64, POLY_CROSS_BRIDGE_DEFAULT,
              dep->path, "dependency FINI", NULL) < 0) {
          unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
          if (tls)
            munmap(tls, tls_size);
          munmap(import_page, 4096);
          munmap(foreign, foreign_size);
          munmap(code, code_size);
          return -1;
        }
      }
    }
  }
  run_poly_atexit_callbacks(code, target_imm_offset);
  if (call_kind == POLY_CALL_DEP_FINI_RESULT) {
    uint64_t fini_result_vaddr = 0;
    int base_kind = RELOC_BASE_ABSOLUTE;
    if (resolve_dependency_symbol(program, "poly_needed_fini_result", NULL,
          &fini_result_vaddr, &base_kind) < 0 ||
        (!reloc_base_is_dep_cross_stub(base_kind) &&
         !reloc_base_is_dep_cross_ifunc(base_kind) &&
         (base_kind < RELOC_BASE_DEP_IFUNC ||
          base_kind >= RELOC_BASE_DEP_IFUNC + (int) program->dep_count) &&
         (base_kind < RELOC_BASE_DEP_LOAD_BIAS ||
          base_kind >= RELOC_BASE_DEP_LOAD_BIAS + (int) program->dep_count))) {
      fprintf(stderr, "POLYCALL_FAIL: dependency fini result symbol missing: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    const int is_cross_stub = reloc_base_is_dep_cross_stub(base_kind);
    const int is_cross_ifunc = reloc_base_is_dep_cross_ifunc(base_kind);
    const int is_dep_ifunc = base_kind >= RELOC_BASE_DEP_IFUNC &&
      base_kind < RELOC_BASE_DEP_IFUNC + (int) program->dep_count;
    const size_t dep_index = is_cross_stub ? dep_cross_stub_index(base_kind) :
      (is_cross_ifunc ? dep_cross_ifunc_index(base_kind) :
       (is_dep_ifunc ? (size_t) (base_kind - RELOC_BASE_DEP_IFUNC) :
        (size_t) (base_kind - RELOC_BASE_DEP_LOAD_BIAS)));
    const int bridge_kind = is_cross_stub ? cross_bridge_kind_for_base(base_kind) :
      (is_cross_ifunc ? cross_bridge_kind_for_ifunc_base(base_kind) :
       POLY_CROSS_BRIDGE_DEFAULT);
    uint64_t fini_result_target = dep_load_bias[dep_index] + fini_result_vaddr;
    if (is_cross_ifunc || is_dep_ifunc) {
      uint64_t resolved = 0;
      if (call_target_from_root_arch(code, target_imm_offset, cross_stubs,
            cross_stub_size, &cross_stub_offset, program->arch,
            program->deps[dep_index].arch, fini_result_target, POLY_CALL_U64,
            POLY_CROSS_BRIDGE_DEFAULT, program->deps[dep_index].path,
            "dependency fini result IFUNC resolver", &resolved) < 0) {
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      fini_result_target = resolved;
    }
    const int result_call_kind = call_kind_for_bridge_result(program->arch,
      POLY_CALL_U64, bridge_kind);
    const uint8_t saved_pcall_op = code[pcall_opcode_offset + 2];
    code[pcall_opcode_offset + 2] =
      pcall_opcode_for_call_kind(program->arch, result_call_kind);
    const int call_status = call_target_from_root_arch(code,
      target_imm_offset, cross_stubs, cross_stub_size, &cross_stub_offset,
      program->arch, program->deps[dep_index].arch, fini_result_target,
      result_call_kind, bridge_kind, program->deps[dep_index].path,
      "dependency fini result", result);
    code[pcall_opcode_offset + 2] = saved_pcall_op;
    if (call_status < 0) {
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
  }
  if (tls)
    munmap(tls, tls_size);
  unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
  munmap(heap, POLY_IMPORT_HEAP_SIZE);
  munmap(import_page, 4096);
  munmap(foreign, foreign_size);
  munmap(code, code_size);
  return 0;
}

static void free_program(struct poly_program *program) {
  free(program->image);
  free(program->relocs);
  for (size_t n = 0; n < program->dep_count; n++)
    free(program->deps[n].image);
  for (size_t n = 0; n < program->dep_count; n++)
    free(program->deps[n].relocs);
  program->image = NULL;
  program->relocs = NULL;
  program->image_size = 0;
  program->entry_offset = 0;
  program->loaded_bytes = 0;
  program->reloc_count = 0;
  program->dep_count = 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s foreign-function.elf[=expected]...\n", argv[0]);
    return 2;
  }

  puts("POLYCALL: start");
  for (int n = 1; n < argc; n++) {
    struct poly_request request;
    if (parse_request(argv[n], &request) < 0)
      return 1;

    struct poly_program program;
    const char *symbol_name = request.symbol[0] ? request.symbol : NULL;
    if (load_elf_program(request.path, symbol_name, &program) < 0)
      return 1;

    size_t dep_init_count = 0;
    size_t dep_fini_count = 0;
    size_t dep_relro_count = 0;
    size_t dep_load_segment_count = 0;
    for (size_t d = 0; d < program.dep_count; d++) {
      dep_init_count += program.deps[d].init_count;
      dep_fini_count += program.deps[d].fini_count;
      if (program.deps[d].relro_size != 0)
        dep_relro_count++;
      dep_load_segment_count += program.deps[d].load_segment_count;
    }
    const size_t relro_count =
      (program.relro_size != 0 ? 1U : 0U) + dep_relro_count;

    printf("POLYCALL_ELF: arch=%s type=%u image_bytes=%zu loaded_bytes=%zu entry_offset=%zu relocs=%zu deps=%zu loads=%zu dep_loads=%zu dep_inits=%zu dep_finis=%zu relro=%zu tls=%llu inits=%zu finis=%zu symbol=%s path=%s\n",
      program.arch_name, (unsigned) program.elf_type, program.image_size,
      program.loaded_bytes, program.entry_offset, program.reloc_count,
      program.dep_count, program.load_segment_count, dep_load_segment_count,
      dep_init_count, dep_fini_count, relro_count,
      (unsigned long long) program.tls_memsz, program.init_count,
      program.fini_count,
      symbol_name ? symbol_name : "-", program.path);

    uint64_t result = 0;
    if (emit_and_call(&program, request.call_kind, request.repeat_count,
          &result) < 0) {
      free_program(&program);
      return 1;
    }

    printf("POLYCALL_RESULT: arch=%s value=%llu path=%s\n",
      program.arch_name, (unsigned long long) result, program.path);
    if (request.repeat_count > 1) {
      printf("POLYCALL_REPEAT: arch=%s count=%u path=%s\n",
        program.arch_name, request.repeat_count, program.path);
    }
    if (request.call_kind == POLY_CALL_FP64) {
      printf("POLYCALL_RESULT_FP64: arch=%s bits=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_FP32) {
      printf("POLYCALL_RESULT_FP32: arch=%s bits=0x%08llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_FPAIR64) {
      printf("POLYCALL_RESULT_FPAIR64: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_FPAIR32) {
      printf("POLYCALL_RESULT_FPAIR32: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_FPAIR32_ARG) {
      printf("POLYCALL_RESULT_FPAIR32_ARG: arch=%s bits=0x%08llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_FPAIR64_ARG) {
      printf("POLYCALL_RESULT_FPAIR64_ARG: arch=%s bits=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_MIXED_ARGS) {
      printf("POLYCALL_RESULT_MIXED_ARGS: arch=%s bits=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_HETERO_U64_F64) {
      printf("POLYCALL_RESULT_HETERO_U64_F64: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_HETERO_F64_U64) {
      printf("POLYCALL_RESULT_HETERO_F64_U64: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_HETERO_U64_F32) {
      printf("POLYCALL_RESULT_HETERO_U64_F32: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_HETERO_F32_U64) {
      printf("POLYCALL_RESULT_HETERO_F32_U64: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_HETERO_U32_F64) {
      printf("POLYCALL_RESULT_HETERO_U32_F64: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_HETERO_F64_U32) {
      printf("POLYCALL_RESULT_HETERO_F64_U32: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_COMPACT_U32_F32) {
      printf("POLYCALL_RESULT_COMPACT_U32_F32: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_COMPACT_F32_U32) {
      printf("POLYCALL_RESULT_COMPACT_F32_U32: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_VEC128_U32) {
      printf("POLYCALL_RESULT_VEC128_U32: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_AARCH64_HFA3_F64) {
      printf("POLYCALL_RESULT_AARCH64_HFA3_F64: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_AARCH64_HFA4_F64) {
      printf("POLYCALL_RESULT_AARCH64_HFA4_F64: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_AARCH64_HFA3_F32) {
      printf("POLYCALL_RESULT_AARCH64_HFA3_F32: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_AARCH64_HFA4_F32) {
      printf("POLYCALL_RESULT_AARCH64_HFA4_F32: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_AARCH64_HFA3_F64_ARG) {
      printf("POLYCALL_RESULT_AARCH64_HFA3_F64_ARG: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_AARCH64_HFA4_F64_ARG) {
      printf("POLYCALL_RESULT_AARCH64_HFA4_F64_ARG: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_AARCH64_HFA3_F32_ARG) {
      printf("POLYCALL_RESULT_AARCH64_HFA3_F32_ARG: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.call_kind == POLY_CALL_AARCH64_HFA4_F32_ARG) {
      printf("POLYCALL_RESULT_AARCH64_HFA4_F32_ARG: arch=%s packed=0x%016llx path=%s\n",
        program.arch_name, (unsigned long long) result, program.path);
    }
    if (request.check_expected) {
      printf("POLYCALL_EXPECT: arch=%s expected=%llu path=%s\n",
        program.arch_name, (unsigned long long) request.expected, program.path);
      if (result != request.expected) {
        fprintf(stderr, "POLYCALL_FAIL: %s expected %llu got %llu\n",
          program.path, (unsigned long long) request.expected, (unsigned long long) result);
        free_program(&program);
        return 1;
      }
    }
    free_program(&program);
  }

  puts("POLYCALL_OK");
  return 0;
}
