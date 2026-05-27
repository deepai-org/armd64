#include <errno.h>
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>

#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x24,0x60,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x24,0x63,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x24,0x62,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG1 ".byte 0x0f,0x24,0x54,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG2 ".byte 0x0f,0x24,0x55,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG3 ".byte 0x0f,0x24,0x56,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG4 ".byte 0x0f,0x24,0x57,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG5 ".byte 0x0f,0x24,0x58,0x50,0x4f,0x4c,0x59,0x21\n"

#ifndef R_AARCH64_RELATIVE
#define R_AARCH64_RELATIVE 1027
#endif

#ifndef R_RISCV_RELATIVE
#define R_RISCV_RELATIVE 3
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

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
  POLY_MODE_X86 = 0,
  POLY_MODE_RAW_AARCH64 = 3,
  POLY_MODE_RAW_RISCV = 4,
  POLY_TRAP_SYSCALL = 1,
  POLY_TRAP_BREAK = 2,
  MAX_PROGRAM_BYTES = 1024 * 1024
};

struct poly_program {
  const char *path;
  const char *arch_name;
  int arch;
  uint64_t base_vaddr;
  size_t entry_offset;
  size_t dynamic_offset;
  size_t dynamic_size;
  uint8_t *code_bytes;
  size_t code_size;
};

struct poly_request {
  char path[160];
  char symbol[80];
  uint64_t expected;
  int check_expected;
};

static inline void poly_mode_x86(void) { asm volatile(".byte 0x0f,0x24,0x00,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }

static inline void write_rax(uint64_t value) {
  asm volatile("" :: "a"(value) : "memory");
}

static inline void poly_trap_vector_set(void) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET ::: "memory");
}

static inline void poly_trap_vector_mode_set(void) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET ::: "memory");
}

static inline uint64_t poly_trap_arg1(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_ARG1 : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_arg2(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_ARG2 : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_arg3(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_ARG3 : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_arg4(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_ARG4 : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_arg5(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_ARG5 : "=a"(value) :: "memory");
  return value;
}

static int poly_is_raw_foreign_mode(uint64_t mode) {
  return mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV;
}

static uint64_t align_down_u64(uint64_t value, uint64_t alignment) {
  return value & ~(alignment - 1);
}

static uint64_t read_u64_le(const uint8_t *bytes) {
  uint64_t value = 0;
  for (unsigned n = 0; n < 8; n++)
    value |= (uint64_t) bytes[n] << (n * 8);
  return value;
}

static void write_u64_le(uint8_t *bytes, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    bytes[n] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static uint32_t relative_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_RELATIVE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_RELATIVE;
  return 0;
}

static int elf_vaddr_to_image_offset(const struct poly_program *program,
    uint64_t vaddr, uint64_t size, size_t *offset) {
  if (vaddr < program->base_vaddr || vaddr > UINT64_MAX - size)
    return -1;
  uint64_t image_offset = vaddr - program->base_vaddr;
  if (image_offset > program->code_size || size > program->code_size - image_offset)
    return -1;
  *offset = (size_t) image_offset;
  return 0;
}

static long poly_x86_syscall6(long number, uint64_t arg0, uint64_t arg1,
    uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
  register long rax __asm__("rax") = number;
  register long rdi __asm__("rdi") = (long) arg0;
  register long rsi __asm__("rsi") = (long) arg1;
  register long rdx __asm__("rdx") = (long) arg2;
  register long r10 __asm__("r10") = (long) arg3;
  register long r8 __asm__("r8") = (long) arg4;
  register long r9 __asm__("r9") = (long) arg5;
  asm volatile("syscall"
      : "+r"(rax)
      : "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8), "r"(r9)
      : "rcx", "r11", "memory");
  return rax;
}

static int poly_generic_linux_syscall_to_x86(uint64_t number, long *x86_number) {
  switch (number) {
    case 17: *x86_number = SYS_getcwd; return 1;
    case 56: *x86_number = SYS_openat; return 1;
    case 57: *x86_number = SYS_close; return 1;
    case 61: *x86_number = SYS_getdents64; return 1;
    case 62: *x86_number = SYS_lseek; return 1;
    case 63: *x86_number = SYS_read; return 1;
    case 64: *x86_number = SYS_write; return 1;
    case 65: *x86_number = SYS_readv; return 1;
    case 66: *x86_number = SYS_writev; return 1;
    case 78: *x86_number = SYS_readlinkat; return 1;
    case 79: *x86_number = SYS_newfstatat; return 1;
    case 80: *x86_number = SYS_fstat; return 1;
    case 93: *x86_number = SYS_exit; return 1;
    case 94: *x86_number = SYS_exit_group; return 1;
    case 96: *x86_number = SYS_set_tid_address; return 1;
    case 98: *x86_number = SYS_futex; return 1;
    case 99: *x86_number = SYS_set_robust_list; return 1;
    case 100: *x86_number = SYS_get_robust_list; return 1;
    case 113: *x86_number = SYS_clock_gettime; return 1;
    case 114: *x86_number = SYS_clock_getres; return 1;
    case 134: *x86_number = SYS_rt_sigaction; return 1;
    case 135: *x86_number = SYS_rt_sigprocmask; return 1;
    case 160: *x86_number = SYS_uname; return 1;
    case 167: *x86_number = SYS_prctl; return 1;
    case 169: *x86_number = SYS_gettimeofday; return 1;
    case 172: *x86_number = SYS_getpid; return 1;
    case 173: *x86_number = SYS_getppid; return 1;
    case 174: *x86_number = SYS_getuid; return 1;
    case 175: *x86_number = SYS_geteuid; return 1;
    case 176: *x86_number = SYS_getgid; return 1;
    case 177: *x86_number = SYS_getegid; return 1;
    case 178: *x86_number = SYS_gettid; return 1;
    case 179: *x86_number = SYS_sysinfo; return 1;
    case 214: *x86_number = SYS_brk; return 1;
    case 215: *x86_number = SYS_munmap; return 1;
    case 222: *x86_number = SYS_mmap; return 1;
    case 226: *x86_number = SYS_mprotect; return 1;
    case 233: *x86_number = SYS_madvise; return 1;
    case 261: *x86_number = SYS_prlimit64; return 1;
    case 278: *x86_number = SYS_getrandom; return 1;
    case 291: *x86_number = SYS_statx; return 1;
    case 293: *x86_number = SYS_rseq; return 1;
    default: return 0;
  }
}

__attribute__((noinline, used))
uint64_t poly_trap_vector_dispatch(uint64_t reason, uint64_t mode,
    uint64_t number, uint64_t pc, uint64_t selector, uint64_t arg0) {
  (void) pc;
  (void) selector;

  if (!poly_is_raw_foreign_mode(mode))
    return (uint64_t) -ENOSYS;

  if (reason == POLY_TRAP_SYSCALL) {
    long x86_number = -1;
    if (!poly_generic_linux_syscall_to_x86(number, &x86_number))
      return (uint64_t) -ENOSYS;
    return (uint64_t) poly_x86_syscall6(x86_number, arg0, poly_trap_arg1(),
      poly_trap_arg2(), poly_trap_arg3(), poly_trap_arg4(), poly_trap_arg5());
  }

  if (reason == POLY_TRAP_BREAK) {
    if (number == 1) {
      const char *text = (const char *) arg0;
      uint64_t length = 0;
      while (length < 4096 && text[length] != '\0')
        length++;
      return length;
    }
    if (number == 2) {
      uint8_t *dest = (uint8_t *) arg0;
      uint8_t value = (uint8_t) poly_trap_arg1();
      uint64_t count = poly_trap_arg2();
      if (count > 4096)
        count = 4096;
      for (uint64_t n = 0; n < count; n++)
        dest[n] = value;
      return count;
    }
    if (number == 3) {
      const uint8_t *left = (const uint8_t *) arg0;
      const uint8_t *right = (const uint8_t *) poly_trap_arg1();
      uint64_t count = poly_trap_arg2();
      if (count > 4096)
        count = 4096;
      for (uint64_t n = 0; n < count; n++) {
        if (left[n] != right[n])
          return (uint64_t) ((int64_t) left[n] - (int64_t) right[n]);
      }
      return 0;
    }
    if (number == 4) {
      uint8_t *dest = (uint8_t *) arg0;
      const uint8_t *src = (const uint8_t *) poly_trap_arg1();
      uint64_t count = poly_trap_arg2();
      if (count > 4096)
        count = 4096;
      for (uint64_t n = 0; n < count; n++)
        dest[n] = src[n];
      return count;
    }
  }

  return (uint64_t) -ENOSYS;
}

__attribute__((naked, noinline, used))
static void poly_trap_vector_handler(void) {
  __asm__(
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq %r11\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rdi, %r9\n"
    "movq %rsi, %r8\n"
    "movq %rcx, %r10\n"
    "movq %rdx, %rcx\n"
    "movq %r10, %rdx\n"
    "movq %rbx, %rsi\n"
    "movq %rax, %rdi\n"
    "subq $8, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "addq $8, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    POLY_OP_TRAP_RETURN
    "ud2\n");
}

static void install_poly_trap_vector(void) {
  write_rax(POLY_MODE_X86);
  poly_trap_vector_mode_set();
  write_rax((uint64_t) (void *) poly_trap_vector_handler);
  poly_trap_vector_set();
}

static void clear_poly_trap_vector(void) {
  write_rax(0);
  poly_trap_vector_set();
  write_rax(POLY_MODE_X86);
  poly_trap_vector_mode_set();
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
  const char *expected = strchr(arg, '=');
  size_t path_len = expected ? (size_t) (expected - arg) : strlen(arg);
  const char *symbol = memchr(arg, '#', path_len);
  if (symbol) {
    size_t symbol_len = path_len - (size_t) (symbol + 1 - arg);
    path_len = (size_t) (symbol - arg);
    if (symbol_len == 0 || symbol_len >= sizeof(request->symbol)) {
      fprintf(stderr, "POLYEXEC_FAIL: bad symbol argument: %s\n", arg);
      return -1;
    }
    memcpy(request->symbol, symbol + 1, symbol_len);
    request->symbol[symbol_len] = '\0';
  }
  if (path_len == 0 || path_len >= sizeof(request->path)) {
    fprintf(stderr, "POLYEXEC_FAIL: bad argument: %s\n", arg);
    return -1;
  }

  memcpy(request->path, arg, path_len);
  request->path[path_len] = '\0';
  if (expected) {
    if (strcmp(expected + 1, "pid") == 0) {
      request->expected = (uint64_t) getpid();
    }
    else if (strcmp(expected + 1, "ppid") == 0) {
      request->expected = (uint64_t) getppid();
    }
    else if (strcmp(expected + 1, "uid") == 0) {
      request->expected = (uint64_t) getuid();
    }
    else if (strcmp(expected + 1, "euid") == 0) {
      request->expected = (uint64_t) geteuid();
    }
    else if (strcmp(expected + 1, "gid") == 0) {
      request->expected = (uint64_t) getgid();
    }
    else if (strcmp(expected + 1, "egid") == 0) {
      request->expected = (uint64_t) getegid();
    }
    else if (strcmp(expected + 1, "tid") == 0) {
      request->expected = (uint64_t) syscall(SYS_gettid);
    }
    else if (strcmp(expected + 1, "cwd") == 0) {
      char cwd[256];
      long result = syscall(SYS_getcwd, cwd, sizeof(cwd));
      if (result < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: unable to compute getcwd expected value: %s\n",
          strerror(errno));
        return -1;
      }
      request->expected = (uint64_t) result;
    }
    else if (parse_u64(expected + 1, &request->expected) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: bad expected value: %s\n", arg);
      return -1;
    }
    request->check_expected = 1;
  }
  return 0;
}

static int read_file(const char *path, unsigned char **data, size_t *size) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to open %s: %s\n", path, strerror(errno));
    return -1;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to seek %s\n", path);
    fclose(file);
    return -1;
  }
  long file_size = ftell(file);
  if (file_size <= 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to size %s\n", path);
    fclose(file);
    return -1;
  }
  rewind(file);

  unsigned char *buffer = malloc((size_t) file_size);
  if (!buffer) {
    fprintf(stderr, "POLYEXEC_FAIL: out of memory reading %s\n", path);
    fclose(file);
    return -1;
  }
  if (fread(buffer, 1, (size_t) file_size, file) != (size_t) file_size) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to read %s\n", path);
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

static void emit_bytes(uint8_t *code, size_t *offset, const uint8_t *bytes, size_t size) {
  memcpy(code + *offset, bytes, size);
  *offset += size;
}

static uint64_t run_poly_entry(const uint8_t *code, uint8_t *scratch) {
  register uint64_t rax __asm__("rax") = (uint64_t) (uintptr_t) scratch;
  register uint64_t rdi __asm__("rdi") = (uint64_t) (uintptr_t) scratch;
  register uint64_t rsi __asm__("rsi") = (uint64_t) (uintptr_t) scratch;
  asm volatile("call *%3"
      : "+a"(rax), "+D"(rdi), "+S"(rsi)
      : "r"(code)
      : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static uint32_t aarch64_adr(unsigned rd, int64_t byte_offset) {
  uint32_t imm = (uint32_t) byte_offset & 0x1fffffU;
  return 0x10000000U | ((imm & 0x3U) << 29) | (((imm >> 2) & 0x7ffffU) << 5) | (rd & 0x1fU);
}

static int aarch64_b(int64_t byte_offset, uint32_t *insn) {
  if ((byte_offset & 3) != 0 ||
      byte_offset < -(INT64_C(1) << 27) ||
      byte_offset >= (INT64_C(1) << 27))
    return -1;
  *insn = 0x14000000U | (((uint32_t) (byte_offset >> 2)) & 0x03ffffffU);
  return 0;
}

static uint32_t riscv_auipc(unsigned rd, int64_t byte_offset) {
  int64_t hi20 = (byte_offset + 0x800) >> 12;
  return (((uint32_t) hi20 & 0xfffffU) << 12) | ((rd & 0x1fU) << 7) | 0x17U;
}

static uint32_t riscv_addi(unsigned rd, unsigned rs1, int64_t byte_offset) {
  int64_t hi20 = (byte_offset + 0x800) >> 12;
  int64_t lo12 = byte_offset - (hi20 << 12);
  return (((uint32_t) lo12 & 0xfffU) << 20) |
    ((rs1 & 0x1fU) << 15) | ((rd & 0x1fU) << 7) | 0x13U;
}

static uint32_t riscv_jalr(unsigned rd, unsigned rs1, int16_t byte_offset) {
  return (((uint32_t) byte_offset & 0xfffU) << 20) |
    ((rs1 & 0x1fU) << 15) | ((rd & 0x1fU) << 7) | 0x67U;
}

static int apply_relative_relocations(const struct poly_program *program,
    uint8_t *loaded_image) {
  if (!program->dynamic_size)
    return 0;

  const Elf64_Dyn *dyn = (const Elf64_Dyn *) (loaded_image + program->dynamic_offset);
  const size_t dyn_count = program->dynamic_size / sizeof(Elf64_Dyn);
  uint64_t rela_vaddr = 0, rela_size = 0, rela_ent = sizeof(Elf64_Rela);
  uint64_t rel_vaddr = 0, rel_size = 0, rel_ent = sizeof(Elf64_Rel);
  uint64_t relr_vaddr = 0, relr_size = 0, relr_ent = sizeof(uint64_t);
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_RELA: rela_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_RELASZ: rela_size = dyn[n].d_un.d_val; break;
      case DT_RELAENT: rela_ent = dyn[n].d_un.d_val; break;
      case DT_REL: rel_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_RELSZ: rel_size = dyn[n].d_un.d_val; break;
      case DT_RELENT: rel_ent = dyn[n].d_un.d_val; break;
      case DT_RELR: relr_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_RELRSZ: relr_size = dyn[n].d_un.d_val; break;
      case DT_RELRENT: relr_ent = dyn[n].d_un.d_val; break;
      default: break;
    }
  }

  const uint32_t relative_type = relative_reloc_type_for_arch(program->arch);
  const uint64_t load_bias = (uint64_t) (uintptr_t) loaded_image - program->base_vaddr;
  if (rela_vaddr && rela_size) {
    if (rela_ent < sizeof(Elf64_Rela) || rela_size % rela_ent)
      return -1;
    size_t rela_offset = 0;
    if (elf_vaddr_to_image_offset(program, rela_vaddr, rela_size, &rela_offset) < 0)
      return -1;
    for (size_t offset = 0; offset < rela_size; offset += (size_t) rela_ent) {
      const Elf64_Rela *rela = (const Elf64_Rela *) (loaded_image + rela_offset + offset);
      if (ELF64_R_SYM(rela->r_info) != 0 || ELF64_R_TYPE(rela->r_info) != relative_type)
        return -1;
      size_t target = 0;
      if (elf_vaddr_to_image_offset(program, rela->r_offset, 8, &target) < 0)
        return -1;
      write_u64_le(loaded_image + target, load_bias + (uint64_t) rela->r_addend);
    }
  }

  if (rel_vaddr && rel_size) {
    if (rel_ent < sizeof(Elf64_Rel) || rel_size % rel_ent)
      return -1;
    size_t rel_offset = 0;
    if (elf_vaddr_to_image_offset(program, rel_vaddr, rel_size, &rel_offset) < 0)
      return -1;
    for (size_t offset = 0; offset < rel_size; offset += (size_t) rel_ent) {
      const Elf64_Rel *rel = (const Elf64_Rel *) (loaded_image + rel_offset + offset);
      if (ELF64_R_SYM(rel->r_info) != 0 || ELF64_R_TYPE(rel->r_info) != relative_type)
        return -1;
      size_t target = 0;
      if (elf_vaddr_to_image_offset(program, rel->r_offset, 8, &target) < 0)
        return -1;
      write_u64_le(loaded_image + target, load_bias + read_u64_le(loaded_image + target));
    }
  }

  if (relr_vaddr && relr_size) {
    if (relr_ent != sizeof(uint64_t) || relr_size % sizeof(uint64_t))
      return -1;
    size_t relr_offset = 0;
    if (elf_vaddr_to_image_offset(program, relr_vaddr, relr_size, &relr_offset) < 0)
      return -1;
    uint64_t where = 0;
    for (size_t offset = 0; offset < relr_size; offset += sizeof(uint64_t)) {
      const uint64_t entry = read_u64_le(loaded_image + relr_offset + offset);
      if ((entry & 1) == 0) {
        where = entry;
        size_t target = 0;
        if (elf_vaddr_to_image_offset(program, where, 8, &target) < 0)
          return -1;
        write_u64_le(loaded_image + target, load_bias + read_u64_le(loaded_image + target));
        where += 8;
      }
      else {
        for (unsigned bit = 1; bit < 64; bit++) {
          if (entry & (UINT64_C(1) << bit)) {
            size_t target = 0;
            if (elf_vaddr_to_image_offset(program, where + (uint64_t) (bit - 1) * 8, 8, &target) < 0)
              return -1;
            write_u64_le(loaded_image + target, load_bias + read_u64_le(loaded_image + target));
          }
        }
        where += 63 * 8;
      }
    }
  }
  return 0;
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

static int resolve_dynamic_symbol(const struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count, const char *symbol_name,
    uint64_t *symbol_vaddr) {
  uint64_t symtab_vaddr = 0;
  uint64_t strtab_vaddr = 0;
  uint64_t strsz = 0;
  uint64_t syment = sizeof(Elf64_Sym);
  uint64_t hash_vaddr = 0;

  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_SYMTAB: symtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRTAB: strtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRSZ: strsz = dyn[n].d_un.d_val; break;
      case DT_SYMENT: syment = dyn[n].d_un.d_val; break;
      case DT_HASH: hash_vaddr = dyn[n].d_un.d_ptr; break;
      default: break;
    }
  }

  if (!symtab_vaddr || !strtab_vaddr || !strsz ||
      syment < sizeof(Elf64_Sym) || !hash_vaddr)
    return -1;

  size_t hash_offset = 0;
  if (elf_vaddr_to_image_offset(program, hash_vaddr, 8, &hash_offset) < 0)
    return -1;
  uint32_t symbol_count = 0;
  memcpy(&symbol_count, program->code_bytes + hash_offset + 4, sizeof(symbol_count));
  if (symbol_count == 0 || symbol_count > 4096)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz, &strtab_offset) < 0)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size, &symtab_offset) < 0)
    return -1;

  for (uint32_t n = 0; n < symbol_count; n++) {
    const Elf64_Sym *sym = (const Elf64_Sym *) (program->code_bytes +
      symtab_offset + (uint64_t) n * syment);
    if (sym->st_name >= strsz)
      continue;
    const char *name = (const char *) (program->code_bytes + strtab_offset + sym->st_name);
    if (strcmp(name, symbol_name) == 0) {
      *symbol_vaddr = sym->st_value;
      return 0;
    }
  }
  return -1;
}

static int resolve_section_symbol(const unsigned char *data, size_t size,
    const Elf64_Ehdr *ehdr, const char *symbol_name, uint64_t *symbol_vaddr) {
  if (!ehdr->e_shoff || !ehdr->e_shnum ||
      ehdr->e_shentsize < sizeof(Elf64_Shdr) ||
      ehdr->e_shoff > size ||
      (uint64_t) ehdr->e_shnum * ehdr->e_shentsize > size - ehdr->e_shoff)
    return -1;

  const Elf64_Shdr *sections = (const Elf64_Shdr *) (data + ehdr->e_shoff);
  for (uint16_t n = 0; n < ehdr->e_shnum; n++) {
    const Elf64_Shdr *sym_shdr = &sections[n];
    if (sym_shdr->sh_type != SHT_DYNSYM && sym_shdr->sh_type != SHT_SYMTAB)
      continue;
    if (sym_shdr->sh_entsize < sizeof(Elf64_Sym) ||
        sym_shdr->sh_offset > size ||
        sym_shdr->sh_size > size - sym_shdr->sh_offset ||
        sym_shdr->sh_link >= ehdr->e_shnum)
      continue;

    const Elf64_Shdr *str_shdr = &sections[sym_shdr->sh_link];
    if (str_shdr->sh_offset > size || str_shdr->sh_size > size - str_shdr->sh_offset)
      continue;

    const size_t symbol_count = (size_t) (sym_shdr->sh_size / sym_shdr->sh_entsize);
    const char *strings = (const char *) (data + str_shdr->sh_offset);
    for (size_t index = 0; index < symbol_count; index++) {
      const Elf64_Sym *sym = (const Elf64_Sym *)
        (data + sym_shdr->sh_offset + (uint64_t) index * sym_shdr->sh_entsize);
      if (sym->st_name >= str_shdr->sh_size)
        continue;
      if (strcmp(strings + sym->st_name, symbol_name) == 0) {
        *symbol_vaddr = sym->st_value;
        return 0;
      }
    }
  }
  return -1;
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
    fprintf(stderr, "POLYEXEC_FAIL: ELF too small: %s\n", path);
    free(data);
    return -1;
  }

  const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *) data;
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
      ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
      (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) ||
      detect_arch(ehdr->e_machine, program) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported ELF header: %s\n", path);
    free(data);
    return -1;
  }

  if (ehdr->e_phentsize < sizeof(Elf64_Phdr) ||
      ehdr->e_phoff > size ||
      (uint64_t) ehdr->e_phnum * ehdr->e_phentsize > size - ehdr->e_phoff) {
    fprintf(stderr, "POLYEXEC_FAIL: bad ELF program header table: %s\n", path);
    free(data);
    return -1;
  }

  uint64_t base_vaddr = UINT64_MAX;
  uint64_t limit_vaddr = 0;
  uint64_t dynamic_vaddr = 0;
  uint64_t dynamic_size = 0;
  int found_load = 0;
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type == PT_DYNAMIC) {
      dynamic_vaddr = phdr->p_vaddr;
      dynamic_size = phdr->p_filesz;
      continue;
    }
    if (phdr->p_type != PT_LOAD)
      continue;

    if (phdr->p_filesz > phdr->p_memsz ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
        phdr->p_vaddr > UINT64_MAX - phdr->p_memsz ||
        phdr->p_vaddr > UINT64_MAX - phdr->p_filesz) {
      fprintf(stderr, "POLYEXEC_FAIL: bad ELF load segment: %s\n", path);
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
      limit_vaddr - base_vaddr > MAX_PROGRAM_BYTES) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported ELF load image: %s\n", path);
    free(data);
    return -1;
  }

  uint64_t image_size = limit_vaddr - base_vaddr;
  const uint64_t instruction_align = program->arch == POLY_ARCH_AARCH64 ? 4 : 2;
  if ((image_size % instruction_align) != 0)
    image_size += instruction_align - (image_size % instruction_align);
  if (image_size == 0 || image_size > MAX_PROGRAM_BYTES) {
    fprintf(stderr, "POLYEXEC_FAIL: ELF loaded image is too large: %s\n", path);
    free(data);
    return -1;
  }

  program->base_vaddr = base_vaddr;
  program->code_size = (size_t) image_size;
  program->code_bytes = calloc(1, program->code_size);
  if (!program->code_bytes) {
    fprintf(stderr, "POLYEXEC_FAIL: out of memory loading %s\n", path);
    free(data);
    return -1;
  }

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD || phdr->p_filesz == 0)
      continue;

    const uint64_t dest_offset = phdr->p_vaddr - base_vaddr;
    if (dest_offset > program->code_size ||
        phdr->p_filesz > program->code_size - dest_offset) {
      fprintf(stderr, "POLYEXEC_FAIL: bad ELF load copy range: %s\n", path);
      free(program->code_bytes);
      program->code_bytes = NULL;
      program->code_size = 0;
      free(data);
      return -1;
    }
    memcpy(program->code_bytes + dest_offset, data + phdr->p_offset, (size_t) phdr->p_filesz);
  }

  uint64_t entry_vaddr = ehdr->e_entry;
  if (symbol_name && symbol_name[0] != '\0') {
    int resolved = -1;
    if (dynamic_vaddr && dynamic_size && dynamic_size % sizeof(Elf64_Dyn) == 0) {
      size_t dynamic_offset = 0;
      if (elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size, &dynamic_offset) == 0) {
        resolved = resolve_dynamic_symbol(program,
          (const Elf64_Dyn *) (program->code_bytes + dynamic_offset),
          (size_t) (dynamic_size / sizeof(Elf64_Dyn)), symbol_name, &entry_vaddr);
      }
    }
    if (resolved < 0)
      resolved = resolve_section_symbol(data, size, ehdr, symbol_name, &entry_vaddr);
    if (resolved < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: symbol not found: %s#%s\n", path, symbol_name);
      free(program->code_bytes);
      program->code_bytes = NULL;
      program->code_size = 0;
      free(data);
      return -1;
    }
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
    fprintf(stderr, "POLYEXEC_FAIL: unsupported ELF entry image: %s\n", path);
    free(program->code_bytes);
    program->code_bytes = NULL;
    program->code_size = 0;
    free(data);
    return -1;
  }
  program->entry_offset = (size_t) (entry_vaddr - base_vaddr);

  if (dynamic_vaddr && dynamic_size) {
    if (dynamic_size % sizeof(Elf64_Dyn) != 0 ||
        elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size,
          &program->dynamic_offset) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: bad dynamic segment: %s\n", path);
      free(program->code_bytes);
      program->code_bytes = NULL;
      program->code_size = 0;
      free(data);
      return -1;
    }
    program->dynamic_size = (size_t) dynamic_size;
  }

  free(data);
  return 0;
}

static int emit_and_run(const struct poly_program *program, uint64_t *result) {
  const size_t return_setup_size = program->arch == POLY_ARCH_AARCH64 ? 4 : 8;
  const size_t branch_size = program->arch == POLY_ARCH_AARCH64 ? 4 : 12;
  const size_t raw_switch_size = 8;
  const size_t prefix_size = raw_switch_size + return_setup_size + branch_size;
  const size_t load_base_offset = 4096;
  const size_t branch_offset = load_base_offset - branch_size;
  const size_t code_offset = branch_offset - raw_switch_size - return_setup_size;
  const size_t mapping_size = load_base_offset + program->code_size + 4 + 1;
  uint8_t *mapping = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: mmap failed: %s\n", strerror(errno));
    return -1;
  }

  uint8_t *code = mapping + code_offset;
  size_t offset = 0;
  const uint64_t return_pc = (uint64_t) (uintptr_t) (mapping + load_base_offset + program->code_size);
  const uint64_t entry_pc = (uint64_t) (uintptr_t) (mapping + load_base_offset + program->entry_offset);
  if (program->arch == POLY_ARCH_AARCH64) {
    const uint8_t raw_switch[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, aarch64_adr(30,
      (int64_t) return_pc - (int64_t) (uintptr_t) (code + offset)));
    uint32_t branch = 0;
    if (aarch64_b((int64_t) entry_pc - (int64_t) (uintptr_t) (code + offset),
          &branch) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: AArch64 entry branch out of range: %s\n",
        program->path);
      munmap(mapping, mapping_size);
      return -1;
    }
    emit_u32(code, &offset, branch);
  } else {
    const uint8_t raw_switch[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    int64_t escape_offset = (int64_t) return_pc - (int64_t) (uintptr_t) (code + offset);
    emit_u32(code, &offset, riscv_auipc(1, escape_offset));
    emit_u32(code, &offset, riscv_addi(1, 1, escape_offset));
    int64_t entry_offset = (int64_t) entry_pc - (int64_t) (uintptr_t) (code + offset);
    emit_u32(code, &offset, riscv_auipc(5, entry_offset));
    emit_u32(code, &offset, riscv_addi(5, 5, entry_offset));
    emit_u32(code, &offset, riscv_jalr(0, 5, 0));
  }
  if (offset != prefix_size) {
    fprintf(stderr, "POLYEXEC_FAIL: internal trampoline size mismatch: %s\n",
      program->path);
    munmap(mapping, mapping_size);
    return -1;
  }
  offset = load_base_offset;
  emit_bytes(mapping, &offset, program->code_bytes, program->code_size);
  if (apply_relative_relocations(program, mapping + load_base_offset) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported dynamic relocations: %s\n",
      program->path);
    munmap(mapping, mapping_size);
    return -1;
  }
  const uint32_t escape = program->arch == POLY_ARCH_AARCH64 ? 0xd42fffe0U : 0x0000000bU;
  emit_u32(mapping, &offset, escape);
  mapping[offset++] = 0xc3;

  char scratch[4096] = "poly!\0/init";
  *result = run_poly_entry(code, (uint8_t *) scratch);
  poly_mode_x86();
  munmap(mapping, mapping_size);
  return 0;
}

static void free_program(struct poly_program *program) {
  free(program->code_bytes);
  program->code_bytes = NULL;
  program->code_size = 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s foreign.elf[=expected]...\n", argv[0]);
    return 2;
  }

  puts("POLYEXEC: start");
  install_poly_trap_vector();
  for (int n = 1; n < argc; n++) {
    struct poly_request request;
    if (parse_request(argv[n], &request) < 0)
      return 1;

    struct poly_program program;
    if (load_elf_program(request.path, request.symbol, &program) < 0)
      return 1;

    printf("POLYEXEC_ELF: arch=%s bytes=%zu entry=%zu path=%s%s%s\n",
      program.arch_name, program.code_size, program.entry_offset,
      program.path, request.symbol[0] ? "#" : "", request.symbol);

    uint64_t result = 0;
    if (emit_and_run(&program, &result) < 0) {
      free_program(&program);
      return 1;
    }

    printf("POLYEXEC_RESULT: arch=%s value=%llu path=%s\n",
      program.arch_name, (unsigned long long) result, program.path);
    if (request.check_expected) {
      printf("POLYEXEC_EXPECT: arch=%s expected=%llu path=%s\n",
        program.arch_name, (unsigned long long) request.expected, program.path);
      if (result != request.expected) {
        fprintf(stderr, "POLYEXEC_FAIL: %s expected %llu got %llu\n",
          program.path, (unsigned long long) request.expected, (unsigned long long) result);
        free_program(&program);
        return 1;
      }
    }
    free_program(&program);
  }

  clear_poly_trap_vector();
  puts("POLYEXEC_OK");
  return 0;
}
