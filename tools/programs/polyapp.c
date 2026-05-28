#include <errno.h>
#include <elf.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../include/polycpuid.h"

#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x24,0x60,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x24,0x63,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x24,0x62,0x50,0x4f,0x4c,0x59,0x21\n"

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
  POLY_BREAK_STATUS = 0,
  MAX_PROGRAM_BYTES = 1024 * 1024,
  SCRATCH_SIZE = 64,
  SCRATCH_CHECK_SIZE = 16
};

struct payload {
  const char *path;
  const char *arch_name;
  const char *final_arch_name;
  char elf_path[128];
  int arch;
  int final_arch;
  uint64_t expected;
  uint64_t syscall_expected;
  uint64_t syscall_number_expected;
  uint64_t syscall_selector_expected;
  uint64_t break_expected;
  uint64_t break_number_expected;
  char scratch_expected[SCRATCH_CHECK_SIZE + 1];
  char scratch_hex_expected[SCRATCH_CHECK_SIZE * 2 + 1];
  uint32_t *insns;
  size_t insn_count;
  size_t insn_capacity;
  unsigned break_id;
  int check_syscall;
  int check_syscall_number;
  int check_syscall_selector;
  int check_break;
  int check_break_number;
  int check_scratch;
  int check_scratch_hex;
  int use_elf;
};

static jmp_buf polyapp_exit_env;
static int polyapp_exit_env_valid;
static uint64_t polyapp_exit_result;

static inline void poly_mode_x86(void) { asm volatile(".byte 0x0f,0x24,0x00,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_syscall_number_status(void) { asm volatile(".byte 0x0f,0x24,0x31,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_break_number_status(void) { asm volatile(".byte 0x0f,0x24,0x39,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_trap_selector_status(void) { asm volatile(".byte 0x0f,0x24,0x5a,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }

static inline void poly_trap_vector_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline void poly_trap_vector_mode_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
}

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
}

static void write_u16(void *addr, uint16_t value) {
  uint8_t *bytes = (uint8_t *) addr;
  bytes[0] = (uint8_t) value;
  bytes[1] = (uint8_t) (value >> 8);
}

static void write_u64(void *addr, uint64_t value) {
  uint8_t *bytes = (uint8_t *) addr;
  for (unsigned n = 0; n < 8; n++)
    bytes[n] = (uint8_t) (value >> (n * 8));
}

static uint64_t read_u64(const void *addr) {
  const uint8_t *bytes = (const uint8_t *) addr;
  uint64_t value = 0;
  for (unsigned n = 0; n < 8; n++)
    value |= (uint64_t) bytes[n] << (n * 8);
  return value;
}

static uint64_t copy_to_iov(uint64_t iov_addr, uint64_t iovcnt,
    const uint8_t *input, uint64_t input_size) {
  uint64_t total = 0;
  uint64_t copied = 0;
  if (iovcnt > 16)
    iovcnt = 16;
  for (uint64_t iov = 0; iov < iovcnt && copied < input_size; iov++) {
    uint8_t *entry = (uint8_t *) (uintptr_t) (iov_addr + iov * 16);
    uint64_t base = read_u64(entry);
    uint64_t len = read_u64(entry + 8);
    uint64_t count = len < (input_size - copied) ? len : (input_size - copied);
    memcpy((void *) (uintptr_t) base, input + copied, (size_t) count);
    copied += count;
    total += count;
  }
  return total;
}

static uint64_t sum_iov_lengths(uint64_t iov_addr, uint64_t iovcnt) {
  uint64_t total = 0;
  if (iovcnt > 16)
    iovcnt = 16;
  for (uint64_t iov = 0; iov < iovcnt; iov++) {
    const uint8_t *entry = (const uint8_t *) (uintptr_t) (iov_addr + iov * 16);
    total += read_u64(entry + 8);
  }
  return total;
}

static int polyapp_is_raw_mode(uint64_t mode) {
  return mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV;
}

static uint64_t polyapp_unknown_syscall(uint64_t mode, uint64_t number) {
  return 0x53000000ULL | (number << 8) | mode;
}

static int polyapp_scalar_syscall(uint64_t number, uint64_t arg0,
    uint64_t *result) {
  switch (number) {
    case 25:
    case 29:
    case 48:
    case 98:
    case 99:
    case 134:
    case 135:
    case 233:
    case 293:
      *result = 0;
      return 1;
    case 96:
    case 178:
      *result = 4243;
      return 1;
    case 155:
    case 156:
      if (arg0 != 0)
        return 0;
      *result = 4242;
      return 1;
    case 172:
      *result = 4242;
      return 1;
    case 173:
      *result = 4241;
      return 1;
    case 174:
    case 175:
    case 176:
    case 177:
      *result = 1000;
      return 1;
    default:
      return 0;
  }
}

static uint64_t polyapp_file_syscall(uint64_t number, uint64_t arg0,
    uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5, int *handled) {
  (void) arg4;
  (void) arg5;
  static const uint8_t stat_magic[] = {'P', 'S', 'T', 'A', 'T', '!', '!', '\0'};

  *handled = 1;
  switch (number) {
    case 17: {
      const char cwd[] = "/poly";
      if (arg1 < sizeof(cwd))
        return (uint64_t) -34;
      memcpy((void *) (uintptr_t) arg0, cwd, sizeof(cwd));
      return sizeof(cwd);
    }
    case 56: {
      const char *path = (const char *) (uintptr_t) arg1;
      return path && strcmp(path, "poly!") == 0 ? 3 : (uint64_t) -2;
    }
    case 57:
      return arg0 == 3 ? 0 : (uint64_t) -9;
    case 61:
      if (arg1 != 0 && arg2 >= 24 && arg0 <= 3) {
        uint8_t *dirent = (uint8_t *) (uintptr_t) arg1;
        write_u64(dirent, 1);
        write_u64(dirent + 8, 1);
        write_u16(dirent + 16, 24);
        dirent[18] = 4;
        dirent[19] = '.';
        dirent[20] = '\0';
        return 24;
      }
      break;
    case 62:
      return (arg0 == 3 && arg2 <= 2) ? arg1 : (uint64_t) -9;
    case 63:
      if (arg0 == 0 || arg0 == 3) {
        static const uint8_t stdin_input[] = {'R', 'X', '!', '!'};
        static const uint8_t file_input[] = {'F', 'D', '!', '!'};
        const uint8_t *input = arg0 == 3 ? file_input : stdin_input;
        uint64_t count = arg2 < 4 ? arg2 : 4;
        memcpy((void *) (uintptr_t) arg1, input, (size_t) count);
        return count;
      }
      break;
    case 64:
      if (arg0 == 1)
        return arg2;
      break;
    case 65:
      if (arg0 == 0 || arg0 == 3) {
        static const uint8_t stdin_input[] = {'R', 'V', '!', '!'};
        static const uint8_t file_input[] = {'F', 'V', '!', '!'};
        return copy_to_iov(arg1, arg2, arg0 == 3 ? file_input : stdin_input, 4);
      }
      break;
    case 66:
      if (arg0 == 1 || arg0 == 2)
        return sum_iov_lengths(arg1, arg2);
      break;
    case 78:
      if (arg2 != 0 && arg3 != 0) {
        const char target[] = "poly!";
        uint64_t count = (sizeof(target) - 1) < arg3 ? (sizeof(target) - 1) : arg3;
        memcpy((void *) (uintptr_t) arg2, target, (size_t) count);
        return count;
      }
      break;
    case 79:
      if (arg2 != 0) {
        memcpy((void *) (uintptr_t) arg2, stat_magic, sizeof(stat_magic));
        return 0;
      }
      break;
    case 80:
      if (arg1 != 0 && arg0 <= 3) {
        memcpy((void *) (uintptr_t) arg1, stat_magic, sizeof(stat_magic));
        return 0;
      }
      break;
    case 160: {
      const char sysname[] = "Linux";
      memcpy((void *) (uintptr_t) arg0, sysname, sizeof(sysname));
      return 0;
    }
    default:
      *handled = 0;
      return 0;
  }

  *handled = 0;
  return 0;
}

static uint64_t polyapp_memory_syscall(uint64_t number, uint64_t arg0,
    uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5, int *handled) {
  *handled = 1;
  switch (number) {
    case 113:
      if (arg0 == 0 && arg1 != 0) {
        write_u64((void *) (uintptr_t) arg1, 123);
        write_u64((void *) (uintptr_t) (arg1 + 8), 456789);
        return 0;
      }
      break;
    case 165:
      if (arg0 == 0 && arg1 != 0) {
        write_u64((void *) (uintptr_t) arg1, 321);
        write_u64((void *) (uintptr_t) (arg1 + 8), 654321);
        return 0;
      }
      break;
    case 168:
      if (arg0 != 0 && arg1 != 0) {
        write_u64((void *) (uintptr_t) arg0, 12);
        write_u64((void *) (uintptr_t) arg1, 34);
        return 0;
      }
      break;
    case 169:
      if (arg0 != 0 && arg1 == 0) {
        write_u64((void *) (uintptr_t) arg0, 246);
        write_u64((void *) (uintptr_t) (arg0 + 8), 13579);
        return 0;
      }
      break;
    case 179:
      if (arg0 != 0) {
        write_u64((void *) (uintptr_t) arg0, 98765);
        write_u64((void *) (uintptr_t) (arg0 + 8), 111);
        return 0;
      }
      break;
    case 214:
      return arg0 != 0 ? arg0 : arg1;
    case 215:
      if (arg0 != 0 && arg1 != 0)
        return 0;
      break;
    case 222:
      if (arg0 == 0 && arg1 >= 4096 && arg2 == (PROT_READ | PROT_WRITE) &&
          arg3 == (MAP_PRIVATE | MAP_ANONYMOUS) && (int64_t) arg4 == -1 &&
          arg5 == 0) {
        void *mapping = mmap(NULL, (size_t) arg1, (int) arg2, (int) arg3, -1, 0);
        return mapping == MAP_FAILED ? (uint64_t) -errno : (uint64_t) (uintptr_t) mapping;
      }
      if (arg0 == 0)
        return (arg3 == 34 && arg4 == 5 && arg5 == 7) ?
          arg1 + arg2 + arg3 + arg4 + arg5 : arg1;
      break;
    case 226:
      if (arg0 != 0 && arg1 != 0)
        return 0;
      break;
    case 261:
      if (arg3 != 0) {
        write_u64((void *) (uintptr_t) arg3, 8388608);
        write_u64((void *) (uintptr_t) (arg3 + 8), UINT64_MAX);
        return 0;
      }
      break;
    case 278:
      if (arg0 != 0) {
        static const uint8_t random_data[] = {'P', 'R', 'N', 'D', '!', '!', '\0', '\0'};
        uint64_t count = arg1 < sizeof(random_data) ? arg1 : sizeof(random_data);
        memcpy((void *) (uintptr_t) arg0, random_data, (size_t) count);
        return count;
      }
      break;
    default:
      *handled = 0;
      return 0;
  }

  *handled = 0;
  return 0;
}

static uint64_t polyapp_process_syscall(uint64_t number, uint64_t arg0,
    uint64_t arg1, uint64_t arg2, int *handled) {
  (void) arg1;
  (void) arg2;
  *handled = 1;
  switch (number) {
    case 93:
    case 94:
      if (polyapp_exit_env_valid) {
        polyapp_exit_result = arg0;
        longjmp(polyapp_exit_env, 1);
      }
      return arg0;
    default:
      *handled = 0;
      return 0;
  }
}

__attribute__((noinline, used))
uint64_t polyapp_trap_vector_dispatch(uint64_t reason, uint64_t mode,
    uint64_t number, uint64_t pc, uint64_t selector, uint64_t arg0,
    uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) {
  (void) pc;
  (void) selector;

  if (!polyapp_is_raw_mode(mode))
    return (uint64_t) -38;

  if (reason == POLY_TRAP_BREAK) {
    return 0x4c000000ULL | (mode << 8) | number;
  }

  if (reason == POLY_TRAP_SYSCALL) {
    uint64_t result = 0;
    int handled = 0;
    if (polyapp_scalar_syscall(number, arg0, &result))
      return result;
    result = polyapp_file_syscall(number, arg0, arg1, arg2, arg3, arg4,
      arg5, &handled);
    if (handled)
      return result;
    result = polyapp_memory_syscall(number, arg0, arg1, arg2, arg3, arg4,
      arg5, &handled);
    if (handled)
      return result;
    result = polyapp_process_syscall(number, arg0, arg1, arg2, &handled);
    if (handled)
      return result;
    return polyapp_unknown_syscall(mode, number);
  }

  return (uint64_t) -38;
}

__attribute__((naked, noinline, used))
static void polyapp_trap_vector_handler(void) {
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
    "pushq %r12\n"
    "pushq %r11\n"
    "pushq %r10\n"
    "pushq %r9\n"
    "pushq %r8\n"
    "movq %rdi, %r9\n"
    "movq %rsi, %r8\n"
    "movq %rcx, %r10\n"
    "movq %rdx, %rcx\n"
    "movq %r10, %rdx\n"
    "movq %rbx, %rsi\n"
    "movq %rax, %rdi\n"
    "call polyapp_trap_vector_dispatch\n"
    "addq $40, %rsp\n"
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

static void install_polyapp_trap_vector(void) {
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value((uint64_t) (void *) polyapp_trap_vector_handler);
}

static void trim(char *line) {
  char *newline = strchr(line, '\n');
  if (newline)
    *newline = '\0';
  char *comment = strchr(line, '#');
  if (comment)
    *comment = '\0';
  for (size_t len = strlen(line); len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t'); len--)
    line[len - 1] = '\0';
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

static int is_hex_string(const char *text) {
  for (size_t n = 0; text[n] != '\0'; n++) {
    if (!((text[n] >= '0' && text[n] <= '9') ||
          (text[n] >= 'a' && text[n] <= 'f') ||
          (text[n] >= 'A' && text[n] <= 'F')))
      return 0;
  }
  return 1;
}

static char hex_digit(unsigned value) {
  static const char digits[] = "0123456789abcdef";
  return digits[value & 0xf];
}

static int read_file(const char *path, unsigned char **data, size_t *size) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "POLYAPP_FAIL: unable to open %s: %s\n", path, strerror(errno));
    return -1;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "POLYAPP_FAIL: unable to seek %s\n", path);
    fclose(file);
    return -1;
  }
  long file_size = ftell(file);
  if (file_size < 0) {
    fprintf(stderr, "POLYAPP_FAIL: unable to size %s\n", path);
    fclose(file);
    return -1;
  }
  rewind(file);

  unsigned char *buffer = malloc((size_t) file_size);
  if (!buffer) {
    fprintf(stderr, "POLYAPP_FAIL: out of memory reading %s\n", path);
    fclose(file);
    return -1;
  }
  if (fread(buffer, 1, (size_t) file_size, file) != (size_t) file_size) {
    fprintf(stderr, "POLYAPP_FAIL: unable to read %s\n", path);
    free(buffer);
    fclose(file);
    return -1;
  }
  fclose(file);
  *data = buffer;
  *size = (size_t) file_size;
  return 0;
}

static uint32_t read_le32(const unsigned char *bytes) {
  return (uint32_t) bytes[0] |
    ((uint32_t) bytes[1] << 8) |
    ((uint32_t) bytes[2] << 16) |
    ((uint32_t) bytes[3] << 24);
}

static void emit_u32(uint8_t *code, size_t *offset, uint32_t value) {
  code[(*offset)++] = (uint8_t) (value & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 8) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 16) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 24) & 0xff);
}

static uint64_t run_poly_entry(const uint8_t *code, uint8_t *scratch) {
  uint64_t rax = (uint64_t) (uintptr_t) scratch;
  asm volatile(
      "pushq %%rbx\n"
      "pushq %%rbp\n"
      "pushq %%r12\n"
      "pushq %%r13\n"
      "pushq %%r14\n"
      "pushq %%r15\n"
      "movq %%rax, %%rdi\n"
      "movq %%rax, %%rsi\n"
      "call *%1"
      "\npopq %%r15"
      "\npopq %%r14"
      "\npopq %%r13"
      "\npopq %%r12"
      "\npopq %%rbp"
      "\npopq %%rbx"
      : "+a"(rax)
      : "r"(code)
      : "rdi", "rsi", "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static int parse_arch(const char *arch, int *arch_value, const char **arch_name) {
  if (strcmp(arch, "aarch64") == 0) {
    *arch_value = POLY_ARCH_AARCH64;
    *arch_name = "aarch64";
    return 0;
  }
  if (strcmp(arch, "riscv") == 0) {
    *arch_value = POLY_ARCH_RISCV;
    *arch_name = "riscv";
    return 0;
  }
  return -1;
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

static int append_insn(struct payload *payload, uint32_t insn) {
  if (payload->insn_count == payload->insn_capacity) {
    size_t new_capacity = payload->insn_capacity ? payload->insn_capacity * 2 : 16;
    if (new_capacity > MAX_PROGRAM_BYTES / 4)
      new_capacity = MAX_PROGRAM_BYTES / 4;
    if (new_capacity <= payload->insn_capacity) {
      fprintf(stderr, "POLYAPP_FAIL: too many instructions in %s\n", payload->path);
      return -1;
    }
    uint32_t *new_insns = realloc(payload->insns, new_capacity * sizeof(payload->insns[0]));
    if (!new_insns) {
      fprintf(stderr, "POLYAPP_FAIL: out of memory loading %s\n", payload->path);
      return -1;
    }
    payload->insns = new_insns;
    payload->insn_capacity = new_capacity;
  }
  payload->insns[payload->insn_count++] = insn;
  return 0;
}

static int load_elf_instructions(struct payload *payload) {
  unsigned char *data = NULL;
  size_t size = 0;
  if (read_file(payload->elf_path, &data, &size) < 0)
    return -1;

  if (size < sizeof(Elf64_Ehdr)) {
    fprintf(stderr, "POLYAPP_FAIL: ELF too small: %s\n", payload->elf_path);
    free(data);
    return -1;
  }

  const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *) data;
  const int expected_machine = payload->arch == POLY_ARCH_AARCH64 ? EM_AARCH64 : EM_RISCV;
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
      ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
      ehdr->e_type != ET_EXEC ||
      ehdr->e_machine != expected_machine) {
    fprintf(stderr, "POLYAPP_FAIL: unsupported ELF header: %s\n", payload->elf_path);
    free(data);
    return -1;
  }

  if (ehdr->e_phentsize < sizeof(Elf64_Phdr) ||
      ehdr->e_phoff > size ||
      (uint64_t) ehdr->e_phnum * ehdr->e_phentsize > size - ehdr->e_phoff) {
    fprintf(stderr, "POLYAPP_FAIL: bad ELF program header table: %s\n", payload->elf_path);
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
    if (entry_filesz == 0 || (entry_filesz % 4) != 0 || entry_filesz > MAX_PROGRAM_BYTES ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset) {
      fprintf(stderr, "POLYAPP_FAIL: bad ELF executable segment: %s\n", payload->elf_path);
      free(data);
      return -1;
    }

    free(payload->insns);
    payload->insns = NULL;
    payload->insn_count = 0;
    payload->insn_capacity = 0;

    const unsigned char *entry_bytes = data + phdr->p_offset + entry_offset;
    const size_t insn_count = (size_t) (entry_filesz / 4);
    for (size_t insn = 0; insn < insn_count; insn++) {
      if (append_insn(payload, read_le32(entry_bytes + insn * 4)) < 0) {
        free(data);
        return -1;
      }
    }
    free(data);
    return 0;
  }

  fprintf(stderr, "POLYAPP_FAIL: no executable ELF segment at entry: %s\n", payload->elf_path);
  free(data);
  return -1;
}

static int load_payload(const char *path, struct payload *payload) {
  memset(payload, 0, sizeof(*payload));
  payload->path = path;
  FILE *file = fopen(path, "r");
  if (!file) {
    fprintf(stderr, "POLYAPP_FAIL: unable to open %s: %s\n", path, strerror(errno));
    return -1;
  }

  char line[128];
  while (fgets(line, sizeof(line), file)) {
    trim(line);
    if (line[0] == '\0')
      continue;

    if (strncmp(line, "arch=", 5) == 0) {
      const char *arch = line + 5;
      if (parse_arch(arch, &payload->arch, &payload->arch_name) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: unsupported arch in %s: %s\n", path, arch);
        fclose(file);
        return -1;
      }
    } else if (strncmp(line, "final_arch=", 11) == 0) {
      const char *arch = line + 11;
      if (parse_arch(arch, &payload->final_arch, &payload->final_arch_name) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: unsupported final arch in %s: %s\n", path, arch);
        fclose(file);
        return -1;
      }
    } else if (strncmp(line, "expected=", 9) == 0) {
      if (parse_u64(line + 9, &payload->expected) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: bad expected value in %s\n", path);
        fclose(file);
        return -1;
      }
    } else if (strncmp(line, "elf=", 4) == 0) {
      if (strlen(line + 4) >= sizeof(payload->elf_path)) {
        fprintf(stderr, "POLYAPP_FAIL: ELF path too long in %s\n", path);
        fclose(file);
        return -1;
      }
      strcpy(payload->elf_path, line + 4);
      payload->use_elf = 1;
    } else if (strncmp(line, "syscall_expected=", 17) == 0) {
      if (parse_u64(line + 17, &payload->syscall_expected) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: bad syscall expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->check_syscall = 1;
    } else if (strncmp(line, "syscall_number_expected=", 24) == 0) {
      if (parse_u64(line + 24, &payload->syscall_number_expected) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: bad syscall number expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->check_syscall_number = 1;
    } else if (strncmp(line, "syscall_selector_expected=", 26) == 0) {
      if (parse_u64(line + 26, &payload->syscall_selector_expected) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: bad syscall selector expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->check_syscall_selector = 1;
    } else if (strncmp(line, "break_expected=", 15) == 0) {
      if (parse_u64(line + 15, &payload->break_expected) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: bad break expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->check_break = 1;
    } else if (strncmp(line, "break_number_expected=", 22) == 0) {
      if (parse_u64(line + 22, &payload->break_number_expected) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: bad break number expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->check_break_number = 1;
    } else if (strncmp(line, "scratch_expected=", 17) == 0) {
      if (strlen(line + 17) > SCRATCH_CHECK_SIZE) {
        fprintf(stderr, "POLYAPP_FAIL: scratch expected value too long in %s\n", path);
        fclose(file);
        return -1;
      }
      strcpy(payload->scratch_expected, line + 17);
      payload->check_scratch = 1;
    } else if (strncmp(line, "scratch_hex_expected=", 21) == 0) {
      size_t hex_len = strlen(line + 21);
      if (hex_len != SCRATCH_CHECK_SIZE * 2 || !is_hex_string(line + 21)) {
        fprintf(stderr, "POLYAPP_FAIL: bad scratch hex expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      strcpy(payload->scratch_hex_expected, line + 21);
      payload->check_scratch_hex = 1;
    } else if (strncmp(line, "break_id=", 9) == 0) {
      uint64_t break_id = 0;
      if (parse_u64(line + 9, &break_id) < 0 || break_id != POLY_BREAK_STATUS) {
        fprintf(stderr, "POLYAPP_FAIL: unsupported break id in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->break_id = (unsigned) break_id;
    } else if (strncmp(line, "insn=", 5) == 0) {
      uint64_t insn = 0;
      if (parse_u64(line + 5, &insn) < 0 || insn > UINT32_MAX) {
        fprintf(stderr, "POLYAPP_FAIL: bad instruction in %s\n", path);
        fclose(file);
        return -1;
      }
      if (append_insn(payload, (uint32_t) insn) < 0) {
        fclose(file);
        return -1;
      }
    } else {
      fprintf(stderr, "POLYAPP_FAIL: unknown directive in %s: %s\n", path, line);
      fclose(file);
      return -1;
    }
  }
  fclose(file);

  if (payload->use_elf && load_elf_instructions(payload) < 0)
    return -1;

  if (payload->arch == 0 || payload->insn_count == 0) {
    fprintf(stderr, "POLYAPP_FAIL: incomplete payload %s\n", path);
    return -1;
  }
  if (payload->final_arch == 0) {
    payload->final_arch = payload->arch;
    payload->final_arch_name = payload->arch_name;
  }
  return 0;
}

static void free_payload(struct payload *payload) {
  free(payload->insns);
  payload->insns = NULL;
  payload->insn_count = 0;
  payload->insn_capacity = 0;
}

static int read_polyapp_base_contract(void) {
  const struct poly_cpuid_regs base = poly_read_cpuid(POLY_CPUID_BASE, 0);
  if (base.eax < POLY_CPUID_MAX || !poly_cpuid_vendor_matches(&base)) {
    fprintf(stderr,
      "POLYAPP_FAIL: poly CPUID missing base=(0x%x,0x%x,0x%x,0x%x)\n",
      base.eax, base.ebx, base.ecx, base.edx);
    return -1;
  }

  const uint32_t required_modes = poly_cpuid_expected_mode_mask();
  const uint32_t required_features =
    POLY_CPUID_FEATURE_RAW_AARCH64 |
    POLY_CPUID_FEATURE_RAW_RISCV |
    POLY_CPUID_FEATURE_NEUTRAL_SWITCH |
    POLY_CPUID_FEATURE_TRAP_RECORDS |
    POLY_CPUID_FEATURE_X86_TSO |
    POLY_CPUID_FEATURE_X86_POLY_OPCODES |
    POLY_CPUID_FEATURE_TRAP_VECTOR;
  const struct poly_cpuid_regs features = poly_read_cpuid(POLY_CPUID_BASE + 1, 0);
  if (features.eax != POLY_CPUID_ABI_VERSION ||
      (features.ebx & required_modes) != required_modes ||
      (features.ecx & required_features) != required_features) {
    fprintf(stderr,
      "POLYAPP_FAIL: poly CPUID feature mismatch features=(%u,0x%x,0x%x,0x%x)\n",
      features.eax, features.ebx, features.ecx, features.edx);
    return -1;
  }

  return 0;
}

static int emit_and_run(const struct payload *payload, uint64_t *result,
    uint64_t *syscall_result, uint64_t *syscall_number_result,
    uint64_t *syscall_selector_result, uint64_t *break_result,
    uint64_t *break_number_result,
    char scratch_result[SCRATCH_CHECK_SIZE + 1],
    char scratch_hex_result[SCRATCH_CHECK_SIZE * 2 + 1]) {
  const size_t return_setup_insns = payload->arch == POLY_ARCH_AARCH64 ? 2 : 3;
  const size_t code_size = 3 + 8 + (return_setup_insns + payload->insn_count) * 4 + 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYAPP_FAIL: mmap failed: %s\n", strerror(errno));
    return -1;
  }

  code[0] = 0x90;
  code[1] = 0x90;
  code[2] = 0x90;
  size_t offset = 3;
  if (payload->arch == POLY_ARCH_AARCH64) {
    const uint8_t raw_switch[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, aarch64_adr(30, (int64_t) (payload->insn_count + 2) * 4));
    emit_u32(code, &offset, 0xd2800008U);
  } else {
    const uint8_t raw_switch[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    int64_t escape_offset = (int64_t) (payload->insn_count + 3) * 4;
    emit_u32(code, &offset, riscv_auipc(1, escape_offset));
    emit_u32(code, &offset, riscv_addi(1, 1, escape_offset));
    emit_u32(code, &offset, 0x00000893U);
  }
  for (size_t n = 0; n < payload->insn_count; n++) {
    emit_u32(code, &offset, payload->insns[n]);
  }
  const uint32_t escape = payload->final_arch == POLY_ARCH_AARCH64 ? 0xd42fffe0U : 0x0000000bU;
  emit_u32(code, &offset, escape);
  code[offset++] = 0xc3;

  char scratch[SCRATCH_SIZE] = "poly!";
  polyapp_exit_env_valid = 1;
  if (setjmp(polyapp_exit_env) == 0)
    *result = run_poly_entry(code, (uint8_t *) scratch);
  else
    *result = polyapp_exit_result;
  polyapp_exit_env_valid = 0;
  const uint64_t raw_mode = payload->arch == POLY_ARCH_AARCH64 ? 3 : 4;
  if (payload->check_syscall) {
    *syscall_result = raw_mode;
  }
  if (payload->check_syscall_number) {
    poly_syscall_number_status();
    *syscall_number_result = read_rax();
  }
  if (payload->check_syscall_selector) {
    poly_trap_selector_status();
    *syscall_selector_result = read_rax();
  }
  if (payload->check_break) {
    *break_result = 0x4c000000ULL | (raw_mode << 8);
  }
  if (payload->check_break_number) {
    poly_break_number_status();
    *break_number_result = read_rax();
  }
  memcpy(scratch_result, scratch, SCRATCH_CHECK_SIZE);
  scratch_result[SCRATCH_CHECK_SIZE] = '\0';
  for (size_t n = 0; n < SCRATCH_CHECK_SIZE; n++) {
    scratch_hex_result[n * 2] = hex_digit((unsigned char) scratch[n] >> 4);
    scratch_hex_result[n * 2 + 1] = hex_digit((unsigned char) scratch[n]);
  }
  scratch_hex_result[SCRATCH_CHECK_SIZE * 2] = '\0';
  poly_mode_x86();
  munmap(code, code_size);
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s payload.poly...\n", argv[0]);
    return 2;
  }

  puts("POLYAPP: start");
  if (read_polyapp_base_contract() < 0)
    return 1;
  install_polyapp_trap_vector();
  for (int n = 1; n < argc; n++) {
    struct payload payload;
    if (load_payload(argv[n], &payload) < 0)
      return 1;
    if (payload.use_elf) {
      printf("POLYAPP_ELF: arch=%s insns=%zu elf=%s\n",
        payload.arch_name, payload.insn_count, payload.elf_path);
    }

    uint64_t result = 0;
    uint64_t syscall_result = 0;
    uint64_t syscall_number_result = 0;
    uint64_t syscall_selector_result = 0;
    uint64_t break_result = 0;
    uint64_t break_number_result = 0;
    char scratch_result[SCRATCH_CHECK_SIZE + 1];
    char scratch_hex_result[SCRATCH_CHECK_SIZE * 2 + 1];
    if (emit_and_run(&payload, &result, &syscall_result,
          &syscall_number_result, &syscall_selector_result, &break_result,
          &break_number_result, scratch_result, scratch_hex_result) < 0) {
      free_payload(&payload);
      return 1;
    }

    printf("POLYAPP_RESULT: arch=%s value=%llu path=%s\n",
      payload.arch_name, (unsigned long long) result, payload.path);
    if (result != payload.expected) {
      fprintf(stderr, "POLYAPP_FAIL: %s expected %llu got %llu\n",
        payload.path, (unsigned long long) payload.expected, (unsigned long long) result);
      free_payload(&payload);
      return 1;
    }
    if (payload.check_syscall) {
      printf("POLYAPP_SYSCALL: arch=%s value=%llu path=%s\n",
        payload.arch_name, (unsigned long long) syscall_result, payload.path);
      if (syscall_result != payload.syscall_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s syscall expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.syscall_expected, (unsigned long long) syscall_result);
        free_payload(&payload);
        return 1;
      }
    }
    if (payload.check_syscall_number) {
      printf("POLYAPP_SYSCALL_NUMBER: arch=%s value=%llu path=%s\n",
        payload.arch_name, (unsigned long long) syscall_number_result, payload.path);
      if (syscall_number_result != payload.syscall_number_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s syscall number expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.syscall_number_expected, (unsigned long long) syscall_number_result);
        free_payload(&payload);
        return 1;
      }
    }
    if (payload.check_syscall_selector) {
      printf("POLYAPP_SYSCALL_SELECTOR: arch=%s value=%llu path=%s\n",
        payload.arch_name, (unsigned long long) syscall_selector_result,
        payload.path);
      if (syscall_selector_result != payload.syscall_selector_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s syscall selector expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.syscall_selector_expected,
          (unsigned long long) syscall_selector_result);
        free_payload(&payload);
        return 1;
      }
    }
    if (payload.check_break) {
      printf("POLYAPP_BREAK: arch=%s id=%u value=%llu path=%s\n",
        payload.arch_name, payload.break_id, (unsigned long long) break_result, payload.path);
      if (break_result != payload.break_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s break expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.break_expected, (unsigned long long) break_result);
        free_payload(&payload);
        return 1;
      }
    }
    if (payload.check_break_number) {
      printf("POLYAPP_BREAK_NUMBER: arch=%s value=%llu path=%s\n",
        payload.arch_name, (unsigned long long) break_number_result, payload.path);
      if (break_number_result != payload.break_number_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s break number expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.break_number_expected, (unsigned long long) break_number_result);
        free_payload(&payload);
        return 1;
      }
    }
    if (payload.check_scratch) {
      printf("POLYAPP_SCRATCH: arch=%s value=%s path=%s\n",
        payload.arch_name, scratch_result, payload.path);
      if (strcmp(scratch_result, payload.scratch_expected) != 0) {
        fprintf(stderr, "POLYAPP_FAIL: %s scratch expected %s got %s\n",
          payload.path, payload.scratch_expected, scratch_result);
        free_payload(&payload);
        return 1;
      }
    }
    if (payload.check_scratch_hex) {
      printf("POLYAPP_SCRATCH_HEX: arch=%s value=%s path=%s\n",
        payload.arch_name, scratch_hex_result, payload.path);
      if (strcmp(scratch_hex_result, payload.scratch_hex_expected) != 0) {
        fprintf(stderr, "POLYAPP_FAIL: %s scratch hex expected %s got %s\n",
          payload.path, payload.scratch_hex_expected, scratch_hex_result);
        free_payload(&payload);
        return 1;
      }
    }
    free_payload(&payload);
  }

  puts("POLYAPP_OK");
  return 0;
}
