#include <errno.h>
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

struct payload {
  const char *path;
  const char *arch_name;
  int arch;
  uint64_t expected;
  uint32_t insns[MAX_INSNS];
  size_t insn_count;
};

static inline void poly_mode_x86(void) { asm volatile(".byte 0x64,0x0f,0x0b,0x58,0x4d,0x4f,0x44,0x45" ::: "memory"); }
static inline void poly_mode_aarch64(void) { asm volatile(".byte 0x65,0x0f,0x0b,0x41,0x41,0x52,0x36,0x34" ::: "memory"); }
static inline void poly_mode_riscv(void) { asm volatile(".byte 0x66,0x0f,0x0b,0x52,0x49,0x53,0x43,0x56" ::: "memory"); }

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

  if (payload->arch == 0 || payload->insn_count == 0) {
    fprintf(stderr, "POLYAPP_FAIL: incomplete payload %s\n", path);
    return -1;
  }
  return 0;
}

static int emit_and_run(const struct payload *payload, uint64_t *result) {
  const size_t code_size = 3 + payload->insn_count * 8 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYAPP_FAIL: mmap failed: %s\n", strerror(errno));
    return -1;
  }

  code[0] = 0x90;
  code[1] = 0x90;
  code[2] = 0x90;
  size_t offset = 3;
  const uint8_t prefix = payload->arch == POLY_ARCH_AARCH64 ? 0x67 : 0x26;
  for (size_t n = 0; n < payload->insn_count; n++) {
    const uint32_t insn = payload->insns[n];
    code[offset++] = prefix;
    code[offset++] = 0x0f;
    code[offset++] = 0x0b;
    code[offset++] = (uint8_t) (insn & 0xff);
    code[offset++] = (uint8_t) ((insn >> 8) & 0xff);
    code[offset++] = (uint8_t) ((insn >> 16) & 0xff);
    code[offset++] = (uint8_t) ((insn >> 24) & 0xff);
    code[offset++] = 0x00;
  }
  code[offset++] = 0xc3;

  if (payload->arch == POLY_ARCH_AARCH64)
    poly_mode_aarch64();
  else
    poly_mode_riscv();

  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

    uint64_t result = 0;
    if (emit_and_run(&payload, &result) < 0)
      return 1;

    printf("POLYAPP_RESULT: arch=%s value=%llu path=%s\n",
      payload.arch_name, (unsigned long long) result, payload.path);
    if (result != payload.expected) {
      fprintf(stderr, "POLYAPP_FAIL: %s expected %llu got %llu\n",
        payload.path, (unsigned long long) payload.expected, (unsigned long long) result);
      return 1;
    }
  }

  puts("POLYAPP_OK");
  return 0;
}
