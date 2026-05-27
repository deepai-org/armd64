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
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x24,0x62,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG1 ".byte 0x0f,0x24,0x54,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG2 ".byte 0x0f,0x24,0x55,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG3 ".byte 0x0f,0x24,0x56,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG4 ".byte 0x0f,0x24,0x57,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_ARG5 ".byte 0x0f,0x24,0x58,0x50,0x4f,0x4c,0x59,0x21\n"

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
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
  uint8_t *code_bytes;
  size_t code_size;
};

struct poly_request {
  char path[160];
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
      uint64_t count = poly_trap_arg3();
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
      uint64_t count = poly_trap_arg3();
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
  write_rax((uint64_t) (void *) poly_trap_vector_handler);
  poly_trap_vector_set();
}

static void clear_poly_trap_vector(void) {
  write_rax(0);
  poly_trap_vector_set();
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

static uint32_t aarch64_adr(unsigned rd, int64_t byte_offset) {
  uint32_t imm = (uint32_t) byte_offset & 0x1fffffU;
  return 0x10000000U | ((imm & 0x3U) << 29) | (((imm >> 2) & 0x7ffffU) << 5) | (rd & 0x1fU);
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

static int load_elf_program(const char *path, struct poly_program *program) {
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
      ehdr->e_type != ET_EXEC ||
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

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD || !(phdr->p_flags & PF_X))
      continue;
    if (ehdr->e_entry < phdr->p_vaddr || ehdr->e_entry >= phdr->p_vaddr + phdr->p_filesz)
      continue;
    const uint64_t entry_offset = ehdr->e_entry - phdr->p_vaddr;
    const uint64_t entry_filesz = phdr->p_filesz - entry_offset;
    if (entry_filesz == 0 || entry_filesz > MAX_PROGRAM_BYTES ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset) {
      fprintf(stderr, "POLYEXEC_FAIL: bad ELF executable segment: %s\n", path);
      free(data);
      return -1;
    }
    if (program->arch == POLY_ARCH_AARCH64 && (entry_filesz % 4) != 0) {
      fprintf(stderr, "POLYEXEC_FAIL: AArch64 executable segment is not 4-byte aligned: %s\n", path);
      free(data);
      return -1;
    }
    if (program->arch == POLY_ARCH_RISCV && (entry_filesz % 2) != 0) {
      fprintf(stderr, "POLYEXEC_FAIL: RISC-V executable segment is not 2-byte aligned: %s\n", path);
      free(data);
      return -1;
    }

    program->code_size = (size_t) entry_filesz;
    program->code_bytes = malloc(program->code_size);
    if (!program->code_bytes) {
      fprintf(stderr, "POLYEXEC_FAIL: out of memory loading %s\n", path);
      free(data);
      return -1;
    }
    const unsigned char *entry_bytes = data + phdr->p_offset + entry_offset;
    memcpy(program->code_bytes, entry_bytes, program->code_size);
    free(data);
    return 0;
  }

  fprintf(stderr, "POLYEXEC_FAIL: no executable ELF segment at entry: %s\n", path);
  free(data);
  return -1;
}

static int emit_and_run(const struct poly_program *program, uint64_t *result) {
  const size_t return_setup_size = program->arch == POLY_ARCH_AARCH64 ? 4 : 8;
  const size_t code_size = 3 + 8 + return_setup_size + program->code_size + 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: mmap failed: %s\n", strerror(errno));
    return -1;
  }

  code[0] = 0x90;
  code[1] = 0x90;
  code[2] = 0x90;
  size_t offset = 3;
  if (program->arch == POLY_ARCH_AARCH64) {
    const uint8_t raw_switch[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, aarch64_adr(30, (int64_t) program->code_size + 4));
  } else {
    const uint8_t raw_switch[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    int64_t escape_offset = (int64_t) program->code_size + 8;
    emit_u32(code, &offset, riscv_auipc(1, escape_offset));
    emit_u32(code, &offset, riscv_addi(1, 1, escape_offset));
  }
  emit_bytes(code, &offset, program->code_bytes, program->code_size);
  const uint32_t escape = program->arch == POLY_ARCH_AARCH64 ? 0xd42fffe0U : 0x0000000bU;
  emit_u32(code, &offset, escape);
  code[offset++] = 0xc3;

  char scratch[4096] = "poly!\0/init";
  uint64_t (*entry)(uint64_t *, uint64_t *) = (uint64_t (*)(uint64_t *, uint64_t *)) code;
  *result = entry((uint64_t *) scratch, (uint64_t *) scratch);
  poly_mode_x86();
  munmap(code, code_size);
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
    if (load_elf_program(request.path, &program) < 0)
      return 1;

    printf("POLYEXEC_ELF: arch=%s bytes=%zu path=%s\n",
      program.arch_name, program.code_size, program.path);

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
