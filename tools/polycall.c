#include <errno.h>
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef DT_GNU_HASH
#define DT_GNU_HASH 0x6ffffef5
#endif
#ifndef DT_RELR
#define DT_RELR 36
#endif
#ifndef DT_RELRSZ
#define DT_RELRSZ 35
#endif
#ifndef DT_RELRENT
#define DT_RELRENT 37
#endif
#ifndef R_AARCH64_IRELATIVE
#define R_AARCH64_IRELATIVE 1032
#endif
#ifndef R_AARCH64_TLSDESC
#define R_AARCH64_TLSDESC 1031
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

enum {
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
  MAX_PROGRAM_BYTES = 1024 * 1024,
  MAX_DYNAMIC_RELOCS = 4096,
  MAX_TLS_BYTES = 4096,
  POLY_ERRNO_TLS_OFFSET = 4096,
  POLY_ERRNO_TLS_SIZE = 4104,
  MAX_NEEDED_DEPS = 8,
  MAX_DEP_PATH = 192,
  RELOC_BASE_ABSOLUTE = 0,
  RELOC_BASE_LOAD_BIAS = 1,
  RELOC_BASE_IMPORT_PAGE = 2,
  RELOC_BASE_IMPORT_CALL = 3,
  RELOC_BASE_IRELATIVE = 4,
  RELOC_BASE_TLS_OFFSET = 5,
  RELOC_BASE_DEP_LOAD_BIAS = 100
};

static const uint64_t POLY_IMPORT_CALL_BASE = 0xffffffffffffe000ULL;
static const uint64_t POLY_IMPORT_CALL_STRIDE = 0x10;
static const size_t POLY_X86_IMPORT_DESCRIPTOR_SIZE = 16;

enum {
  POLY_IMPORT_FUNC_ADD = 0,
  POLY_IMPORT_FUNC_MUL = 1,
  POLY_IMPORT_FUNC_X86_ADD = 2,
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
  POLY_IMPORT_FUNC_STRNDUP = 126
};

enum {
  POLY_IMPORT_PAGE_VALUE_OFFSET = 0,
  POLY_IMPORT_PAGE_STACK_GUARD_OFFSET = 8,
  POLY_IMPORT_PAGE_HEAP_BASE_OFFSET = 16,
  POLY_IMPORT_PAGE_HEAP_SIZE_OFFSET = 24,
  POLY_IMPORT_PAGE_HEAP_CURSOR_OFFSET = 32,
  POLY_IMPORT_HEAP_SIZE = 64 * 1024
};

struct poly_dynamic_reloc {
  size_t offset;
  uint64_t value;
  int base_kind;
};

struct poly_symbol_table {
  const Elf64_Sym *symbols;
  size_t symbol_count;
  const char *strings;
  size_t strings_size;
};

struct poly_dependency {
  char path[MAX_DEP_PATH];
  const char *arch_name;
  int arch;
  uint8_t *image;
  size_t image_size;
  uint64_t base_vaddr;
  size_t loaded_bytes;
  uint64_t init_vaddr;
  uint64_t init_array_vaddr;
  uint64_t init_array_size;
  size_t init_count;
  uint64_t fini_vaddr;
  uint64_t fini_array_vaddr;
  uint64_t fini_array_size;
  size_t fini_count;
  size_t needed_depth;
  size_t lookup_rank;
  struct poly_symbol_table dynsym;
  struct poly_dynamic_reloc *relocs;
  size_t reloc_count;
};

struct poly_program {
  const char *path;
  const char *arch_name;
  int arch;
  int elf_type;
  uint8_t *image;
  size_t image_size;
  size_t entry_offset;
  uint64_t base_vaddr;
  size_t loaded_bytes;
  uint64_t tls_vaddr;
  uint64_t tls_filesz;
  uint64_t tls_memsz;
  uint64_t tls_align;
  uint64_t init_vaddr;
  uint64_t init_array_vaddr;
  uint64_t init_array_size;
  size_t init_count;
  uint64_t fini_vaddr;
  uint64_t fini_array_vaddr;
  uint64_t fini_array_size;
  uint64_t fini_result_vaddr;
  size_t fini_count;
  struct poly_dynamic_reloc *relocs;
  size_t reloc_count;
  struct poly_dependency deps[MAX_NEEDED_DEPS];
  size_t dep_count;
  size_t direct_dep_count;
  int needs_x86_import;
  int needs_errno_location;
};

struct poly_request {
  char path[160];
  char symbol[96];
  uint64_t expected;
  int check_expected;
  int call_kind;
};

extern uint64_t poly_host_x86_add(uint64_t a, uint64_t b);
extern uint64_t poly_host_x86_mul(uint64_t a, uint64_t b);
extern uint64_t poly_host_x86_sum6(uint64_t a, uint64_t b, uint64_t c,
    uint64_t d, uint64_t e, uint64_t f);
extern uint64_t poly_host_x86_sum8(uint64_t a, uint64_t b, uint64_t c,
    uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h);
extern double poly_host_x86_fp64_add(double a, double b);
extern double poly_host_x86_fp64_sum8(double a, double b, double c,
    double d, double e, double f, double g, double h);
extern double poly_host_x86_mixed_u64_fp64(uint64_t a, double b, uint64_t c,
    double d, uint64_t e, double f);
extern float poly_host_x86_fp32_add(float a, float b);

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
  else if (strncmp(arg, "pair:", 5) == 0) {
    request->call_kind = POLY_CALL_PAIR_U64;
    arg += 5;
  }
  else if (strncmp(arg, "sret:", 5) == 0) {
    request->call_kind = POLY_CALL_SRET_U64;
    arg += 5;
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

static uint64_t align_down_u64(uint64_t value, uint64_t alignment) {
  return value & ~(alignment - 1);
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

static void emit_save_import_regs(uint8_t *code, size_t *offset) {
  const uint8_t save[] = {
    0x4c, 0x89, 0x64, 0x24, 0xf8 // mov [rsp-8],r12
  };
  memcpy(code + *offset, save, sizeof(save));
  *offset += sizeof(save);
}

static void emit_restore_import_regs(uint8_t *code, size_t *offset) {
  const uint8_t restore[] = {
    0x4c, 0x8b, 0x64, 0x24, 0xf8 // mov r12,[rsp-8]
  };
  memcpy(code + *offset, restore, sizeof(restore));
  *offset += sizeof(restore);
}

static void emit_save_tls_reg(uint8_t *code, size_t *offset) {
  const uint8_t save[] = {
    0x4c, 0x89, 0x6c, 0x24, 0xf0 // mov [rsp-16],r13
  };
  memcpy(code + *offset, save, sizeof(save));
  *offset += sizeof(save);
}

static void emit_restore_tls_reg(uint8_t *code, size_t *offset) {
  const uint8_t restore[] = {
    0x4c, 0x8b, 0x6c, 0x24, 0xf0 // mov r13,[rsp-16]
  };
  memcpy(code + *offset, restore, sizeof(restore));
  *offset += sizeof(restore);
}

static void emit_save_heap_reg(uint8_t *code, size_t *offset) {
  const uint8_t save[] = {
    0x4c, 0x89, 0x74, 0x24, 0xe8 // mov [rsp-24],r14
  };
  memcpy(code + *offset, save, sizeof(save));
  *offset += sizeof(save);
}

static void emit_restore_heap_reg(uint8_t *code, size_t *offset) {
  const uint8_t restore[] = {
    0x4c, 0x8b, 0x74, 0x24, 0xe8 // mov r14,[rsp-24]
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
  if (strcmp(symbol_name, "poly_import_x86_fp64_add") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT3 * POLY_IMPORT_CALL_STRIDE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_x86_fp64_sum8") == 0) {
    *symbol_value = POLY_IMPORT_FUNC_X86_SLOT6 * POLY_IMPORT_CALL_STRIDE;
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
  program->relocs[program->reloc_count].value = value;
  program->relocs[program->reloc_count].base_kind = base_kind;
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
  uint64_t strsz = 0;
  uint64_t syment = sizeof(Elf64_Sym);

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

static int build_origin_path(const char *owner_path, const char *suffix,
    size_t suffix_len, const char *needed, char *out, size_t out_size) {
  const char *slash = strrchr(owner_path, '/');
  const size_t dir_len = slash ? (size_t) (slash - owner_path) : 0;
  const size_t needed_len = strlen(needed);
  const int suffix_has_slash = suffix_len != 0 && suffix[suffix_len - 1] == '/';
  const size_t sep_len = suffix_len == 0 || suffix_has_slash ? 0 : 1;
  if (dir_len + suffix_len + sep_len + needed_len + 1 > out_size)
    return -1;
  if (dir_len != 0)
    memcpy(out, owner_path, dir_len);
  if (suffix_len != 0)
    memcpy(out + dir_len, suffix, suffix_len);
  if (sep_len != 0)
    out[dir_len + suffix_len] = '/';
  memcpy(out + dir_len + suffix_len + sep_len, needed, needed_len + 1);
  return 0;
}

static int build_runpath_needed_path(const char *owner_path,
    const char *runpath, size_t runpath_len, const char *needed,
    char *out, size_t out_size) {
  if (!runpath || runpath_len == 0 || needed[0] == '/')
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

    int built = -1;
    if (entry_len >= 7 && memcmp(entry, "$ORIGIN", 7) == 0) {
      built = build_origin_path(owner_path, entry + 7, entry_len - 7,
        needed, out, out_size);
    }
    else if (entry_len >= 9 && memcmp(entry, "${ORIGIN}", 9) == 0) {
      built = build_origin_path(owner_path, entry + 9, entry_len - 9,
        needed, out, out_size);
    }
    else if (entry[0] == '/') {
      const size_t needed_len = strlen(needed);
      const int entry_has_slash = entry[entry_len - 1] == '/';
      const size_t sep_len = entry_has_slash ? 0 : 1;
      if (entry_len + sep_len + needed_len + 1 <= out_size) {
        memcpy(out, entry, entry_len);
        if (sep_len != 0)
          out[entry_len] = '/';
        memcpy(out + entry_len + sep_len, needed, needed_len + 1);
        built = 0;
      }
    }

    if (built == 0 && access(out, R_OK) == 0)
      return 0;
  }
  return -1;
}

static int load_needed_dependencies_from_dynamic(struct poly_program *owner,
    const char *origin_path, const uint8_t *image, size_t image_size,
    uint64_t base_vaddr, const Elf64_Dyn *dyn, size_t dyn_count,
    size_t needed_depth);

static int load_dependency_object(struct poly_program *owner, size_t dep_index,
    const char *path, int expected_arch, size_t needed_depth) {
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
      arch_probe.arch != expected_arch) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported dependency ELF header: %s\n",
      path);
    free(data);
    return -1;
  }
  dep->arch = arch_probe.arch;
  dep->arch_name = arch_probe.arch_name;

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
  dep_view.fini_vaddr = dep->fini_vaddr;
  dep_view.fini_array_vaddr = dep->fini_array_vaddr;
  dep_view.fini_array_size = dep->fini_array_size;
  dep_view.fini_count = dep->fini_count;

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
  if (load_needed_dependencies_from_dynamic(owner, dep->path, dep->image,
        dep->image_size, dep->base_vaddr, dynamic, dynamic_count,
        dep->needed_depth + 1) < 0) {
    free(dep->image);
    dep->image = NULL;
    free(dep_view.relocs);
    dep_view.relocs = NULL;
    free(data);
    return -1;
  }

  memcpy(dep_view.deps, owner->deps, sizeof(dep_view.deps));
  dep_view.dep_count = owner->dep_count;
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
  dep->fini_vaddr = dep_view.fini_vaddr;
  dep->fini_array_vaddr = dep_view.fini_array_vaddr;
  dep->fini_array_size = dep_view.fini_array_size;
  dep->fini_count = dep_view.fini_count;
  owner->needs_x86_import = dep_view.needs_x86_import;

  free(data);
  return 0;
}

static int load_needed_dependencies_from_dynamic(struct poly_program *owner,
    const char *origin_path, const uint8_t *image, size_t image_size,
    uint64_t base_vaddr, const Elf64_Dyn *dyn, size_t dyn_count,
    size_t needed_depth) {
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

    char needed_path[MAX_DEP_PATH];
    if (build_needed_path(origin_path, strings + needed_offset,
          needed_path, sizeof(needed_path)) < 0 ||
        (access(needed_path, R_OK) != 0 &&
         build_runpath_needed_path(origin_path, search_path, search_path_len,
           strings + needed_offset, needed_path, sizeof(needed_path)) < 0)) {
      fprintf(stderr, "POLYCALL_FAIL: bad DT_NEEDED path: %s\n",
        origin_path);
      return -1;
    }

    int duplicate = 0;
    for (size_t d = 0; d < owner->dep_count; d++) {
      if (strcmp(owner->deps[d].path, needed_path) == 0) {
        if (needed_depth == 0 &&
            owner->deps[d].lookup_rank >= MAX_NEEDED_DEPS)
          owner->deps[d].lookup_rank = owner->direct_dep_count++;
        duplicate = 1;
        break;
      }
    }
    if (duplicate)
      continue;
    if (owner->dep_count >= MAX_NEEDED_DEPS) {
      fprintf(stderr, "POLYCALL_FAIL: too many DT_NEEDED dependencies: %s\n",
        origin_path);
      return -1;
    }
    const size_t dep_index = owner->dep_count++;
    if (load_dependency_object(owner, dep_index, needed_path, owner->arch,
          needed_depth) < 0)
      return -1;
  }
  return 0;
}

static int load_needed_dependencies(struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count) {
  return load_needed_dependencies_from_dynamic(program, program->path,
    program->image, program->image_size, program->base_vaddr, dyn, dyn_count,
    0);
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

static int resolve_dependency_symbol(const struct poly_program *program,
    const char *symbol_name, uint64_t *symbol_value, int *base_kind) {
  size_t best = program->dep_count;
  size_t best_rank = (size_t) -1;
  uint64_t best_value = 0;
  for (size_t n = 0; n < program->dep_count; n++) {
    uint64_t candidate_value = 0;
    if (resolve_symbol_from_table_filtered(&program->deps[n].dynsym,
          symbol_name, &candidate_value, 1) < 0)
      continue;
    if (program->deps[n].lookup_rank >= best_rank)
      continue;
    best = n;
    best_rank = program->deps[n].lookup_rank;
    best_value = candidate_value;
  }
  if (best < program->dep_count) {
    *symbol_value = best_value;
    *base_kind = RELOC_BASE_DEP_LOAD_BIAS + (int) best;
    return 0;
  }
  return -1;
}

static int resolve_external_reloc_symbol(struct poly_program *program,
    const char *symbol_name, uint64_t *symbol_value, int *base_kind) {
  if (resolve_dependency_symbol(program, symbol_name, symbol_value,
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
        strcmp(symbol_name, "poly_import_x86_fp64_add") == 0 ||
        strcmp(symbol_name, "poly_import_x86_fp64_sum8") == 0 ||
        strcmp(symbol_name, "poly_import_x86_mixed_u64_fp64") == 0 ||
        strcmp(symbol_name, "poly_import_x86_fp32_add") == 0)
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
    if (ELF64_ST_BIND(sym->st_info) == STB_WEAK) {
      if (resolve_dependency_symbol(program, symbol_name, symbol_value,
            base_kind) == 0)
        return 0;
      *symbol_value = 0;
      *base_kind = RELOC_BASE_ABSOLUTE;
      return 0;
    }
    *base_kind = RELOC_BASE_ABSOLUTE;
    return resolve_external_reloc_symbol(program, symbol_name, symbol_value,
      base_kind);
  }
  size_t symbol_offset = 0;
  if (elf_vaddr_to_image_offset(program, sym->st_value, 1, &symbol_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: relocation symbol escaped image: %s\n",
      program->path);
    return -1;
  }

  *symbol_value = sym->st_value;
  *base_kind = RELOC_BASE_LOAD_BIAS;
  return 0;
}

static int resolve_tls_reloc_symbol(struct poly_program *program,
    const struct poly_symbol_table *table, uint64_t symbol_index,
    uint64_t *tls_offset) {
  if (!table->symbols || symbol_index >= table->symbol_count) {
    fprintf(stderr, "POLYCALL_FAIL: TLS relocation symbol table missing: %s\n",
      program->path);
    return -1;
  }

  const Elf64_Sym *sym = &table->symbols[symbol_index];
  if (sym->st_shndx == SHN_UNDEF) {
    if (ELF64_ST_BIND(sym->st_info) == STB_WEAK) {
      *tls_offset = 0;
      return 0;
    }
    fprintf(stderr, "POLYCALL_FAIL: unresolved external TLS relocation path=%s\n",
      program->path);
    return -1;
  }
  if (ELF64_ST_TYPE(sym->st_info) != STT_TLS) {
    fprintf(stderr, "POLYCALL_FAIL: non-TLS symbol used by TLS relocation path=%s\n",
      program->path);
    return -1;
  }

  *tls_offset = sym->st_value;
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
  struct poly_symbol_table dynsym;
  memset(&dynsym, 0, sizeof(dynsym));
  for (size_t n = 0; n < rela_count; n++) {
    const uint64_t symbol_index = ELF64_R_SYM(rela[n].r_info);
    const uint32_t reloc_type = ELF64_R_TYPE(rela[n].r_info);
    uint64_t reloc_value = 0;
    int base_kind = RELOC_BASE_LOAD_BIAS;
    if (program->arch == POLY_ARCH_AARCH64 && reloc_type == R_AARCH64_TLSDESC) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: TLS relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }
      uint64_t tls_offset = 0;
      if (resolve_tls_reloc_symbol(program, &dynsym, symbol_index,
            &tls_offset) < 0)
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
            RELOC_BASE_TLS_OFFSET) < 0)
        return -1;
      continue;
    }
    if (program->arch == POLY_ARCH_RISCV &&
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
              &reloc_value) < 0)
          return -1;
        reloc_value += (uint64_t) rela[n].r_addend;
        base_kind = RELOC_BASE_TLS_OFFSET;
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
            &reloc_value) < 0)
        return -1;
      reloc_value += (uint64_t) rela[n].r_addend;
      base_kind = RELOC_BASE_TLS_OFFSET;
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
  struct poly_symbol_table dynsym;
  memset(&dynsym, 0, sizeof(dynsym));
  for (size_t n = 0; n < rel_count; n++) {
    const uint64_t symbol_index = ELF64_R_SYM(rel[n].r_info);
    const uint32_t reloc_type = ELF64_R_TYPE(rel[n].r_info);
    size_t relocation_offset = 0;
    if (elf_vaddr_to_image_offset(program, rel[n].r_offset, 8,
          &relocation_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: relocation target out of image: %s\n",
        program->path);
      return -1;
    }

    uint64_t reloc_value = read_le64(program->image + relocation_offset);
    int base_kind = RELOC_BASE_LOAD_BIAS;
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

  uint64_t entry_vaddr = ehdr->e_entry;
  if (symbol_name && symbol_name[0] != '\0') {
    int resolved = -1;
    if (dynamic_size != 0 && dynamic_size % sizeof(Elf64_Dyn) == 0) {
      size_t dynamic_offset = 0;
      if (elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size,
            &dynamic_offset) == 0) {
        resolved = resolve_dynamic_symbol(program,
          (const Elf64_Dyn *) (program->image + dynamic_offset),
          (size_t) (dynamic_size / sizeof(Elf64_Dyn)), symbol_name,
          &entry_vaddr);
      }
    }
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
  int fini_result_resolved = -1;
  if (dynamic_size != 0 && dynamic_size % sizeof(Elf64_Dyn) == 0) {
    size_t dynamic_offset = 0;
    if (elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size,
          &dynamic_offset) == 0) {
      fini_result_resolved = resolve_dynamic_symbol(program,
        (const Elf64_Dyn *) (program->image + dynamic_offset),
        (size_t) (dynamic_size / sizeof(Elf64_Dyn)), "poly_fini_result",
        &fini_result_vaddr);
    }
  }
  if (fini_result_resolved < 0)
    fini_result_resolved = resolve_elf_symbol_from_sections(data, size, ehdr,
      "poly_fini_result", &fini_result_vaddr);
  if (fini_result_resolved == 0)
    program->fini_result_vaddr = fini_result_vaddr;

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
    if (load_needed_dependencies(program, dynamic, dynamic_count) < 0 ||
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

  uint64_t (*entry)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) =
    (uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)) code;
  return entry(1, 2, 3, 4, 5, 6, 7, 8, 9);
}

static void unmap_dependency_images(uint8_t **dep_foreign,
    const size_t *dep_sizes, size_t dep_count) {
  for (size_t n = 0; n < dep_count; n++) {
    if (dep_foreign[n])
      munmap(dep_foreign[n], dep_sizes[n]);
  }
}

static int emit_and_call(const struct poly_program *program, int call_kind,
    uint64_t *result) {
  const uint32_t fallback_ret = program->arch == POLY_ARCH_AARCH64 ? 0xd65f03c0U : 0x00008067U;
  const int needs_x86_import = program->needs_x86_import;
  const size_t save_regs_size = needs_x86_import ? 5 : 0;
  const size_t restore_regs_size = needs_x86_import ? 5 : 0;
  const size_t import_setup_size = needs_x86_import ? 10 : 0;
  const size_t save_tls_size = 5;
  const size_t restore_tls_size = 5;
  const size_t save_heap_size = 5;
  const size_t restore_heap_size = 5;
  const size_t tls_setup_size = 10;
  const size_t heap_setup_size = 10;
  const size_t pcall_return_offset = save_regs_size + save_tls_size +
    save_heap_size + 10 + 10 + tls_setup_size + heap_setup_size +
    import_setup_size + 8;
  const size_t main_stub_size = pcall_return_offset + restore_regs_size +
    restore_tls_size + restore_heap_size + 1;
  const size_t import_return_size = needs_x86_import ? 8 : 0;
  const size_t import_descriptor_size = needs_x86_import ?
    8 * POLY_X86_IMPORT_DESCRIPTOR_SIZE : 0;
  const size_t stub_size = main_stub_size + import_return_size +
    import_descriptor_size;
  const size_t code_size = stub_size;
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
  uint8_t *foreign = mmap(NULL, foreign_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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
  if (program->tls_memsz != 0 || program->needs_errno_location) {
    tls_size = (size_t) program->tls_memsz;
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
  const uint64_t import_x86_add_target =
    (uint64_t) (uintptr_t) poly_host_x86_add;
  const uint64_t import_x86_mul_target =
    (uint64_t) (uintptr_t) poly_host_x86_mul;
  const uint64_t import_x86_sum6_target =
    (uint64_t) (uintptr_t) poly_host_x86_sum6;
  const uint64_t import_x86_sum8_target =
    (uint64_t) (uintptr_t) poly_host_x86_sum8;
  const uint64_t import_x86_fp64_add_target =
    (uint64_t) (uintptr_t) poly_host_x86_fp64_add;
  const uint64_t import_x86_fp64_sum8_target =
    (uint64_t) (uintptr_t) poly_host_x86_fp64_sum8;
  const uint64_t import_x86_mixed_u64_fp64_target =
    (uint64_t) (uintptr_t) poly_host_x86_mixed_u64_fp64;
  const uint64_t import_x86_fp32_add_target =
    (uint64_t) (uintptr_t) poly_host_x86_fp32_add;
  const uint64_t import_x86_return = (uint64_t) (uintptr_t) (code + main_stub_size);
  const uint64_t import_x86_table = import_x86_return + import_return_size;
  const uint64_t foreign_target = (uint64_t) (uintptr_t) (foreign + program->entry_offset);
  if (needs_x86_import)
    emit_save_import_regs(code, &offset);
  emit_save_tls_reg(code, &offset);
  emit_save_heap_reg(code, &offset);
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
    const uint8_t pcall[] = {
      0x0f, 0x24, pcall_op,
      0x50, 0x4f, 0x4c, 0x59, 0x21
    };
    memcpy(code + offset, pcall, sizeof(pcall));
    offset += sizeof(pcall);
  }
  if (needs_x86_import)
    emit_restore_import_regs(code, &offset);
  emit_restore_tls_reg(code, &offset);
  emit_restore_heap_reg(code, &offset);
  code[offset++] = 0xc3;
  if (needs_x86_import) {
    const uint8_t import_return[] = { 0x0f, 0x24, 0x20, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, import_return, sizeof(import_return));
    offset += sizeof(import_return);
    emit_u64(code, &offset, import_x86_add_target);
    emit_u64(code, &offset, import_x86_return);
    emit_u64(code, &offset, import_x86_mul_target);
    emit_u64(code, &offset, import_x86_return);
    emit_u64(code, &offset, import_x86_sum6_target);
    emit_u64(code, &offset, import_x86_return);
    emit_u64(code, &offset, import_x86_fp64_add_target);
    emit_u64(code, &offset, import_x86_return);
    emit_u64(code, &offset, import_x86_fp32_add_target);
    emit_u64(code, &offset, import_x86_return);
    emit_u64(code, &offset, import_x86_sum8_target);
    emit_u64(code, &offset, import_x86_return);
    emit_u64(code, &offset, import_x86_fp64_sum8_target);
    emit_u64(code, &offset, import_x86_return);
    emit_u64(code, &offset, import_x86_mixed_u64_fp64_target);
    emit_u64(code, &offset, import_x86_return);
  }
  if (offset != code_size) {
    fprintf(stderr, "POLYCALL_FAIL: internal x86 stub size mismatch\n");
    if (tls)
      munmap(tls, tls_size);
    munmap(import_page, 4096);
    munmap(foreign, foreign_size);
    munmap(code, code_size);
    return -1;
  }
  memcpy(foreign, program->image, program->image_size);
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
    emit_u32(dep_foreign[n], &dep_offset, fallback_ret);
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
      if (dep->relocs[r].base_kind == RELOC_BASE_IRELATIVE)
        continue;
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
      else if (dep->relocs[r].base_kind == RELOC_BASE_IMPORT_PAGE)
        reloc_base = (uint64_t) (uintptr_t) import_page;
      else if (dep->relocs[r].base_kind == RELOC_BASE_IMPORT_CALL)
        reloc_base = POLY_IMPORT_CALL_BASE;
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
      if (dep->relocs[r].base_kind != RELOC_BASE_IRELATIVE)
        continue;
      if (dep->relocs[r].offset > dep_sizes[d] ||
          dep_sizes[d] - dep->relocs[r].offset < 8) {
        fprintf(stderr, "POLYCALL_FAIL: dependency IRELATIVE target escaped image: %s\n",
          dep->path);
        unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
        if (tls)
          munmap(tls, tls_size);
        munmap(import_page, 4096);
        munmap(foreign, foreign_size);
        munmap(code, code_size);
        return -1;
      }
      const uint64_t resolver = dep_load_bias[d] + dep->relocs[r].value;
      const uint64_t resolved = call_poly_stub(code, target_imm_offset,
        resolver, POLY_CALL_U64);
      write_le64(dep_foreign[d] + dep->relocs[r].offset, resolved);
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
    memcpy(tls, foreign + tls_image_offset, (size_t) program->tls_filesz);
  }
  if (tls) {
    write_le64(code + tls_imm_offset, (uint64_t) (uintptr_t) tls);
  }
  else {
    write_le64(code + tls_imm_offset, 0);
  }
  write_le64(code + heap_imm_offset, (uint64_t) (uintptr_t) import_page);
  const uint64_t load_bias = (uint64_t) (uintptr_t) foreign - program->base_vaddr;
  for (size_t n = 0; n < program->reloc_count; n++) {
    if (program->relocs[n].base_kind == RELOC_BASE_IRELATIVE)
      continue;
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
      reloc_base = POLY_IMPORT_CALL_BASE;
    else if (program->relocs[n].base_kind == RELOC_BASE_TLS_OFFSET)
      reloc_base = 0;
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
  offset = program->image_size - 4;
  emit_u32(foreign, &offset, fallback_ret);

  for (size_t depth = max_dep_depth + 1; depth > 0; depth--) {
    const size_t init_depth = depth - 1;
    for (size_t d = 0; d < program->dep_count; d++) {
      const struct poly_dependency *dep = &program->deps[d];
      if (dep->needed_depth != init_depth)
        continue;
      if (dep->init_vaddr != 0) {
        const uint64_t init_target = dep_load_bias[d] + dep->init_vaddr;
        (void) call_poly_stub(code, target_imm_offset, init_target,
          POLY_CALL_U64);
      }
      if (dep->init_array_size != 0) {
        if (dep->init_array_vaddr < dep->base_vaddr ||
            dep->init_array_size > dep_sizes[d]) {
          fprintf(stderr, "POLYCALL_FAIL: dependency INIT_ARRAY escaped image: %s\n",
            dep->path);
          unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
          if (tls)
            munmap(tls, tls_size);
          munmap(import_page, 4096);
          munmap(foreign, foreign_size);
          munmap(code, code_size);
          return -1;
        }
        const uint64_t init_array_offset =
          dep->init_array_vaddr - dep->base_vaddr;
        if (init_array_offset > dep_sizes[d] ||
            dep->init_array_size > dep_sizes[d] - init_array_offset) {
          fprintf(stderr, "POLYCALL_FAIL: dependency INIT_ARRAY escaped image: %s\n",
            dep->path);
          unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
          if (tls)
            munmap(tls, tls_size);
          munmap(import_page, 4096);
          munmap(foreign, foreign_size);
          munmap(code, code_size);
          return -1;
        }
        const size_t init_array_count =
          (size_t) (dep->init_array_size / sizeof(uint64_t));
        for (size_t n = 0; n < init_array_count; n++) {
          uint64_t init_target = read_le64(dep_foreign[d] +
            init_array_offset + n * 8);
          if (init_target != 0)
            (void) call_poly_stub(code, target_imm_offset, init_target,
              POLY_CALL_U64);
        }
      }
    }
  }

  for (size_t n = 0; n < program->reloc_count; n++) {
    if (program->relocs[n].base_kind != RELOC_BASE_IRELATIVE)
      continue;
    if (program->relocs[n].offset > foreign_size ||
        foreign_size - program->relocs[n].offset < 8) {
      fprintf(stderr, "POLYCALL_FAIL: IRELATIVE target escaped image: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    size_t resolver_offset = 0;
    if (elf_vaddr_to_image_offset(program, program->relocs[n].value, 4,
          &resolver_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: IRELATIVE resolver escaped image: %s\n",
        program->path);
      unmap_dependency_images(dep_foreign, dep_sizes, program->dep_count);
      if (tls)
        munmap(tls, tls_size);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    const uint64_t resolver = load_bias + program->relocs[n].value;
    const uint64_t resolved = call_poly_stub(code, target_imm_offset,
      resolver, POLY_CALL_U64);
    write_le64(foreign + program->relocs[n].offset, resolved);
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
  *result = call_poly_stub(code, target_imm_offset, foreign_target,
    entry_call_kind);

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
    const uint64_t fini_result_target = load_bias + program->fini_result_vaddr;
    *result = call_poly_stub(code, target_imm_offset, fini_result_target,
      POLY_CALL_U64);
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
          if (fini_target != 0)
            (void) call_poly_stub(code, target_imm_offset, fini_target,
              POLY_CALL_U64);
        }
      }
      if (dep->fini_vaddr != 0) {
        const uint64_t fini_target =
          dep_load_bias[dep_index] + dep->fini_vaddr;
        (void) call_poly_stub(code, target_imm_offset, fini_target,
          POLY_CALL_U64);
      }
    }
  }
  if (call_kind == POLY_CALL_DEP_FINI_RESULT) {
    uint64_t fini_result_vaddr = 0;
    int base_kind = RELOC_BASE_ABSOLUTE;
    if (resolve_dependency_symbol(program, "poly_needed_fini_result",
          &fini_result_vaddr, &base_kind) < 0 ||
        base_kind < RELOC_BASE_DEP_LOAD_BIAS ||
        base_kind >= RELOC_BASE_DEP_LOAD_BIAS + (int) program->dep_count) {
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
    const size_t dep_index =
      (size_t) (base_kind - RELOC_BASE_DEP_LOAD_BIAS);
    const uint64_t fini_result_target =
      dep_load_bias[dep_index] + fini_result_vaddr;
    *result = call_poly_stub(code, target_imm_offset, fini_result_target,
      POLY_CALL_U64);
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
    for (size_t d = 0; d < program.dep_count; d++) {
      dep_init_count += program.deps[d].init_count;
      dep_fini_count += program.deps[d].fini_count;
    }

    printf("POLYCALL_ELF: arch=%s type=%u image_bytes=%zu loaded_bytes=%zu entry_offset=%zu relocs=%zu deps=%zu dep_inits=%zu dep_finis=%zu tls=%llu inits=%zu finis=%zu symbol=%s path=%s\n",
      program.arch_name, (unsigned) program.elf_type, program.image_size,
      program.loaded_bytes, program.entry_offset, program.reloc_count,
      program.dep_count, dep_init_count, dep_fini_count,
      (unsigned long long) program.tls_memsz, program.init_count,
      program.fini_count,
      symbol_name ? symbol_name : "-", program.path);

    uint64_t result = 0;
    if (emit_and_call(&program, request.call_kind, &result) < 0) {
      free_program(&program);
      return 1;
    }

    printf("POLYCALL_RESULT: arch=%s value=%llu path=%s\n",
      program.arch_name, (unsigned long long) result, program.path);
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
