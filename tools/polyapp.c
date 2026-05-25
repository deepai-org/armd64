#include <errno.h>
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
  POLY_LIBCALL_STATUS = 0,
  MAX_INSNS = 32,
  SCRATCH_SIZE = 16
};

struct payload {
  const char *path;
  const char *arch_name;
  char elf_path[128];
  int arch;
  uint64_t expected;
  uint64_t syscall_expected;
  uint64_t syscall_number_expected;
  uint64_t libcall_expected;
  uint64_t libcall_number_expected;
  char scratch_expected[SCRATCH_SIZE + 1];
  char scratch_hex_expected[SCRATCH_SIZE * 2 + 1];
  uint32_t insns[MAX_INSNS];
  size_t insn_count;
  unsigned libcall_id;
  int check_syscall;
  int check_syscall_number;
  int check_libcall;
  int check_libcall_number;
  int check_scratch;
  int check_scratch_hex;
  int use_elf;
};

static inline void poly_mode_x86(void) { asm volatile(".byte 0x64,0x0f,0x0b,0x58,0x4d,0x4f,0x44,0x45" ::: "memory"); }
static inline void poly_mode_aarch64(void) { asm volatile(".byte 0x65,0x0f,0x0b,0x41,0x41,0x52,0x36,0x34" ::: "memory"); }
static inline void poly_mode_riscv(void) { asm volatile(".byte 0x66,0x0f,0x0b,0x52,0x49,0x53,0x43,0x56" ::: "memory"); }
static inline void poly_syscall_status(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x30" ::: "memory"); }
static inline void poly_syscall_number_status(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x31" ::: "memory"); }
static inline void poly_libcall_status(void) { asm volatile(".byte 0x3e,0x0f,0x0b,0x4c,0x49,0x42,0x43,0x30" ::: "memory"); }
static inline void poly_libcall_number_status(void) { asm volatile(".byte 0x3e,0x0f,0x0b,0x4c,0x49,0x42,0x43,0x31" ::: "memory"); }

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
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
    if (phdr->p_filesz == 0 || (phdr->p_filesz % 4) != 0 || phdr->p_filesz / 4 > MAX_INSNS ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset) {
      fprintf(stderr, "POLYAPP_FAIL: bad ELF executable segment: %s\n", payload->elf_path);
      free(data);
      return -1;
    }

    payload->insn_count = (size_t) (phdr->p_filesz / 4);
    for (size_t insn = 0; insn < payload->insn_count; insn++)
      payload->insns[insn] = read_le32(data + phdr->p_offset + insn * 4);
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
      if (strcmp(arch, "aarch64") == 0) {
        payload->arch = POLY_ARCH_AARCH64;
        payload->arch_name = "aarch64";
      } else if (strcmp(arch, "riscv") == 0) {
        payload->arch = POLY_ARCH_RISCV;
        payload->arch_name = "riscv";
      } else {
        fprintf(stderr, "POLYAPP_FAIL: unsupported arch in %s: %s\n", path, arch);
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
    } else if (strncmp(line, "libcall_expected=", 17) == 0) {
      if (parse_u64(line + 17, &payload->libcall_expected) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: bad libcall expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->check_libcall = 1;
    } else if (strncmp(line, "libcall_number_expected=", 24) == 0) {
      if (parse_u64(line + 24, &payload->libcall_number_expected) < 0) {
        fprintf(stderr, "POLYAPP_FAIL: bad libcall number expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->check_libcall_number = 1;
    } else if (strncmp(line, "scratch_expected=", 17) == 0) {
      if (strlen(line + 17) > SCRATCH_SIZE) {
        fprintf(stderr, "POLYAPP_FAIL: scratch expected value too long in %s\n", path);
        fclose(file);
        return -1;
      }
      strcpy(payload->scratch_expected, line + 17);
      payload->check_scratch = 1;
    } else if (strncmp(line, "scratch_hex_expected=", 21) == 0) {
      size_t hex_len = strlen(line + 21);
      if (hex_len != SCRATCH_SIZE * 2 || !is_hex_string(line + 21)) {
        fprintf(stderr, "POLYAPP_FAIL: bad scratch hex expected value in %s\n", path);
        fclose(file);
        return -1;
      }
      strcpy(payload->scratch_hex_expected, line + 21);
      payload->check_scratch_hex = 1;
    } else if (strncmp(line, "libcall_id=", 11) == 0) {
      uint64_t libcall_id = 0;
      if (parse_u64(line + 11, &libcall_id) < 0 || libcall_id != POLY_LIBCALL_STATUS) {
        fprintf(stderr, "POLYAPP_FAIL: unsupported libcall id in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->libcall_id = (unsigned) libcall_id;
    } else if (strncmp(line, "insn=", 5) == 0) {
      uint64_t insn = 0;
      if (payload->insn_count >= MAX_INSNS || parse_u64(line + 5, &insn) < 0 || insn > UINT32_MAX) {
        fprintf(stderr, "POLYAPP_FAIL: bad instruction in %s\n", path);
        fclose(file);
        return -1;
      }
      payload->insns[payload->insn_count++] = (uint32_t) insn;
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
  return 0;
}

static int emit_and_run(const struct payload *payload, uint64_t *result, uint64_t *syscall_result, uint64_t *syscall_number_result, uint64_t *libcall_result, uint64_t *libcall_number_result, char scratch_result[SCRATCH_SIZE + 1], char scratch_hex_result[SCRATCH_SIZE * 2 + 1]) {
  const size_t code_size = 3 + 8 + payload->insn_count * 4 + 4 + 1;
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
    const uint8_t raw_switch[] = { 0x65, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x36, 0x34 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
  } else {
    const uint8_t raw_switch[] = { 0x66, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x52, 0x56 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
  }
  for (size_t n = 0; n < payload->insn_count; n++) {
    const uint32_t insn = payload->insns[n];
    code[offset++] = (uint8_t) (insn & 0xff);
    code[offset++] = (uint8_t) ((insn >> 8) & 0xff);
    code[offset++] = (uint8_t) ((insn >> 16) & 0xff);
    code[offset++] = (uint8_t) ((insn >> 24) & 0xff);
  }
  const uint32_t escape = payload->arch == POLY_ARCH_AARCH64 ? 0xd42fffe0U : 0x0000000bU;
  code[offset++] = (uint8_t) (escape & 0xff);
  code[offset++] = (uint8_t) ((escape >> 8) & 0xff);
  code[offset++] = (uint8_t) ((escape >> 16) & 0xff);
  code[offset++] = (uint8_t) ((escape >> 24) & 0xff);
  code[offset++] = 0xc3;

  char scratch[SCRATCH_SIZE] = "poly!";
  uint64_t (*entry)(uint64_t *) = (uint64_t (*)(uint64_t *)) code;
  *result = entry((uint64_t *) scratch);
  if (payload->arch == POLY_ARCH_AARCH64)
    poly_mode_aarch64();
  else
    poly_mode_riscv();
  if (payload->check_syscall) {
    poly_syscall_status();
    *syscall_result = read_rax();
  }
  if (payload->check_syscall_number) {
    poly_syscall_number_status();
    *syscall_number_result = read_rax();
  }
  if (payload->check_libcall) {
    poly_libcall_status();
    *libcall_result = read_rax();
  }
  if (payload->check_libcall_number) {
    poly_libcall_number_status();
    *libcall_number_result = read_rax();
  }
  memcpy(scratch_result, scratch, SCRATCH_SIZE);
  scratch_result[SCRATCH_SIZE] = '\0';
  for (size_t n = 0; n < SCRATCH_SIZE; n++) {
    scratch_hex_result[n * 2] = hex_digit((unsigned char) scratch[n] >> 4);
    scratch_hex_result[n * 2 + 1] = hex_digit((unsigned char) scratch[n]);
  }
  scratch_hex_result[SCRATCH_SIZE * 2] = '\0';
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
    uint64_t libcall_result = 0;
    uint64_t libcall_number_result = 0;
    char scratch_result[SCRATCH_SIZE + 1];
    char scratch_hex_result[SCRATCH_SIZE * 2 + 1];
    if (emit_and_run(&payload, &result, &syscall_result, &syscall_number_result, &libcall_result, &libcall_number_result, scratch_result, scratch_hex_result) < 0)
      return 1;

    printf("POLYAPP_RESULT: arch=%s value=%llu path=%s\n",
      payload.arch_name, (unsigned long long) result, payload.path);
    if (result != payload.expected) {
      fprintf(stderr, "POLYAPP_FAIL: %s expected %llu got %llu\n",
        payload.path, (unsigned long long) payload.expected, (unsigned long long) result);
      return 1;
    }
    if (payload.check_syscall) {
      printf("POLYAPP_SYSCALL: arch=%s value=%llu path=%s\n",
        payload.arch_name, (unsigned long long) syscall_result, payload.path);
      if (syscall_result != payload.syscall_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s syscall expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.syscall_expected, (unsigned long long) syscall_result);
        return 1;
      }
    }
    if (payload.check_syscall_number) {
      printf("POLYAPP_SYSCALL_NUMBER: arch=%s value=%llu path=%s\n",
        payload.arch_name, (unsigned long long) syscall_number_result, payload.path);
      if (syscall_number_result != payload.syscall_number_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s syscall number expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.syscall_number_expected, (unsigned long long) syscall_number_result);
        return 1;
      }
    }
    if (payload.check_libcall) {
      printf("POLYAPP_LIBCALL: arch=%s id=%u value=%llu path=%s\n",
        payload.arch_name, payload.libcall_id, (unsigned long long) libcall_result, payload.path);
      if (libcall_result != payload.libcall_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s libcall expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.libcall_expected, (unsigned long long) libcall_result);
        return 1;
      }
    }
    if (payload.check_libcall_number) {
      printf("POLYAPP_LIBCALL_NUMBER: arch=%s value=%llu path=%s\n",
        payload.arch_name, (unsigned long long) libcall_number_result, payload.path);
      if (libcall_number_result != payload.libcall_number_expected) {
        fprintf(stderr, "POLYAPP_FAIL: %s libcall number expected %llu got %llu\n",
          payload.path, (unsigned long long) payload.libcall_number_expected, (unsigned long long) libcall_number_result);
        return 1;
      }
    }
    if (payload.check_scratch) {
      printf("POLYAPP_SCRATCH: arch=%s value=%s path=%s\n",
        payload.arch_name, scratch_result, payload.path);
      if (strcmp(scratch_result, payload.scratch_expected) != 0) {
        fprintf(stderr, "POLYAPP_FAIL: %s scratch expected %s got %s\n",
          payload.path, payload.scratch_expected, scratch_result);
        return 1;
      }
    }
    if (payload.check_scratch_hex) {
      printf("POLYAPP_SCRATCH_HEX: arch=%s value=%s path=%s\n",
        payload.arch_name, scratch_hex_result, payload.path);
      if (strcmp(scratch_hex_result, payload.scratch_hex_expected) != 0) {
        fprintf(stderr, "POLYAPP_FAIL: %s scratch hex expected %s got %s\n",
          payload.path, payload.scratch_hex_expected, scratch_hex_result);
        return 1;
      }
    }
  }

  puts("POLYAPP_OK");
  return 0;
}
