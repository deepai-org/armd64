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
  MAX_INSNS = 32
};

struct poly_program {
  const char *path;
  const char *arch_name;
  int arch;
  uint32_t insns[MAX_INSNS];
  size_t insn_count;
};

struct poly_request {
  char path[160];
  uint64_t expected;
  int check_expected;
};

static inline void poly_mode_x86(void) { asm volatile(".byte 0x64,0x0f,0x0b,0x58,0x4d,0x4f,0x44,0x45" ::: "memory"); }
static inline void poly_mode_aarch64(void) { asm volatile(".byte 0x65,0x0f,0x0b,0x41,0x41,0x52,0x36,0x34" ::: "memory"); }
static inline void poly_mode_riscv(void) { asm volatile(".byte 0x66,0x0f,0x0b,0x52,0x49,0x53,0x43,0x56" ::: "memory"); }

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
    if (parse_u64(expected + 1, &request->expected) < 0) {
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

static uint32_t read_le32(const unsigned char *bytes) {
  return (uint32_t) bytes[0] |
    ((uint32_t) bytes[1] << 8) |
    ((uint32_t) bytes[2] << 16) |
    ((uint32_t) bytes[3] << 24);
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
    if (phdr->p_filesz == 0 || (phdr->p_filesz % 4) != 0 || phdr->p_filesz / 4 > MAX_INSNS ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset) {
      fprintf(stderr, "POLYEXEC_FAIL: bad ELF executable segment: %s\n", path);
      free(data);
      return -1;
    }

    program->insn_count = (size_t) (phdr->p_filesz / 4);
    for (size_t insn = 0; insn < program->insn_count; insn++)
      program->insns[insn] = read_le32(data + phdr->p_offset + insn * 4);
    free(data);
    return 0;
  }

  fprintf(stderr, "POLYEXEC_FAIL: no executable ELF segment at entry: %s\n", path);
  free(data);
  return -1;
}

static int emit_and_run(const struct poly_program *program, uint64_t *result) {
  const size_t code_size = 3 + 8 + program->insn_count * 4 + 4 + 1;
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
    const uint8_t raw_switch[] = { 0x65, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x36, 0x34 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
  } else {
    const uint8_t raw_switch[] = { 0x66, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x52, 0x56 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
  }
  for (size_t n = 0; n < program->insn_count; n++) {
    const uint32_t insn = program->insns[n];
    code[offset++] = (uint8_t) (insn & 0xff);
    code[offset++] = (uint8_t) ((insn >> 8) & 0xff);
    code[offset++] = (uint8_t) ((insn >> 16) & 0xff);
    code[offset++] = (uint8_t) ((insn >> 24) & 0xff);
  }
  const uint32_t escape = program->arch == POLY_ARCH_AARCH64 ? 0xd42fffe0U : 0x0000000bU;
  code[offset++] = (uint8_t) (escape & 0xff);
  code[offset++] = (uint8_t) ((escape >> 8) & 0xff);
  code[offset++] = (uint8_t) ((escape >> 16) & 0xff);
  code[offset++] = (uint8_t) ((escape >> 24) & 0xff);
  code[offset++] = 0xc3;

  char scratch[16] = "poly!";
  uint64_t (*entry)(uint64_t *) = (uint64_t (*)(uint64_t *)) code;
  *result = entry((uint64_t *) scratch);
  poly_mode_x86();
  munmap(code, code_size);
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s foreign.elf[=expected]...\n", argv[0]);
    return 2;
  }

  puts("POLYEXEC: start");
  for (int n = 1; n < argc; n++) {
    struct poly_request request;
    if (parse_request(argv[n], &request) < 0)
      return 1;

    struct poly_program program;
    if (load_elf_program(request.path, &program) < 0)
      return 1;

    printf("POLYEXEC_ELF: arch=%s insns=%zu path=%s\n",
      program.arch_name, program.insn_count, program.path);

    uint64_t result = 0;
    if (emit_and_run(&program, &result) < 0)
      return 1;

    printf("POLYEXEC_RESULT: arch=%s value=%llu path=%s\n",
      program.arch_name, (unsigned long long) result, program.path);
    if (request.check_expected) {
      printf("POLYEXEC_EXPECT: arch=%s expected=%llu path=%s\n",
        program.arch_name, (unsigned long long) request.expected, program.path);
      if (result != request.expected) {
        fprintf(stderr, "POLYEXEC_FAIL: %s expected %llu got %llu\n",
          program.path, (unsigned long long) request.expected, (unsigned long long) result);
        return 1;
      }
    }
  }

  puts("POLYEXEC_OK");
  return 0;
}
