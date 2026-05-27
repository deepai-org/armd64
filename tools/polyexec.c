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

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
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

__attribute__((naked, noinline, used))
static void poly_trap_vector_handler(void) {
  __asm__(
    "cmpq $1, %rax\n"
    "jne 3f\n"
    "cmpq $3, %rbx\n"
    "je 1f\n"
    "cmpq $4, %rbx\n"
    "jne 9f\n"
    "1:\n"
    "cmpq $172, %rcx\n"
    "je 20f\n"
    "cmpq $173, %rcx\n"
    "je 21f\n"
    "cmpq $174, %rcx\n"
    "je 22f\n"
    "cmpq $175, %rcx\n"
    "je 23f\n"
    "cmpq $176, %rcx\n"
    "je 24f\n"
    "cmpq $177, %rcx\n"
    "je 25f\n"
    "cmpq $178, %rcx\n"
    "je 26f\n"
    "jmp 9f\n"
    "20:\n"
    "movq $39, %rax\n"
    "jmp 2f\n"
    "21:\n"
    "movq $110, %rax\n"
    "jmp 2f\n"
    "22:\n"
    "movq $102, %rax\n"
    "jmp 2f\n"
    "23:\n"
    "movq $107, %rax\n"
    "jmp 2f\n"
    "24:\n"
    "movq $104, %rax\n"
    "jmp 2f\n"
    "25:\n"
    "movq $108, %rax\n"
    "jmp 2f\n"
    "26:\n"
    "movq $186, %rax\n"
    "2:\n"
    "syscall\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "3:\n"
    "cmpq $2, %rax\n"
    "jne 9f\n"
    "cmpq $3, %rbx\n"
    "je 4f\n"
    "cmpq $4, %rbx\n"
    "jne 9f\n"
    "4:\n"
    "cmpq $1, %rcx\n"
    "je 5f\n"
    "cmpq $2, %rcx\n"
    "je 7f\n"
    "cmpq $3, %rcx\n"
    "je 11f\n"
    "cmpq $4, %rcx\n"
    "je 15f\n"
    "jmp 9f\n"
    "5:\n"
    "xorq %rax, %rax\n"
    "6:\n"
    "cmpb $0, (%rdi,%rax,1)\n"
    "je 10f\n"
    "incq %rax\n"
    "jmp 6b\n"
    "7:\n"
    POLY_OP_TRAP_ARG3
    "movq %rax, %r8\n"
    POLY_OP_TRAP_ARG1
    "movb %al, %r9b\n"
    "xorq %rax, %rax\n"
    "8:\n"
    "cmpq %r8, %rax\n"
    "jae 10f\n"
    "movb %r9b, (%rdi,%rax,1)\n"
    "incq %rax\n"
    "jmp 8b\n"
    "11:\n"
    POLY_OP_TRAP_ARG1
    "movq %rax, %r8\n"
    POLY_OP_TRAP_ARG2
    "movq %rax, %r9\n"
    "xorq %rax, %rax\n"
    "12:\n"
    "cmpq %r9, %rax\n"
    "jae 14f\n"
    "movzbl (%rdi,%rax,1), %r10d\n"
    "movzbl (%r8,%rax,1), %r11d\n"
    "cmpq %r11, %r10\n"
    "jne 13f\n"
    "incq %rax\n"
    "jmp 12b\n"
    "13:\n"
    "movq %r10, %rax\n"
    "subq %r11, %rax\n"
    "jmp 10f\n"
    "14:\n"
    "xorq %rax, %rax\n"
    "jmp 10f\n"
    "15:\n"
    POLY_OP_TRAP_ARG3
    "movq %rax, %r8\n"
    POLY_OP_TRAP_ARG1
    "movq %rax, %r9\n"
    "xorq %rax, %rax\n"
    "16:\n"
    "cmpq %r8, %rax\n"
    "jae 10f\n"
    "movb (%r9,%rax,1), %r10b\n"
    "movb %r10b, (%rdi,%rax,1)\n"
    "incq %rax\n"
    "jmp 16b\n"
    "10:\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "9:\n"
    "movq $-38, %rax\n"
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

  char scratch[64] = "poly!";
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
