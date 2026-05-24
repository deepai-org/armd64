#include <stdint.h>
#include <stdio.h>

static inline void poly_mode_x86(void) { asm volatile(".byte 0x0f,0x3f,0x00" ::: "memory"); }
static inline void poly_mode_aarch64(void) { asm volatile(".byte 0x0f,0x3f,0x01" ::: "memory"); }
static inline void poly_mode_riscv(void) { asm volatile(".byte 0x0f,0x3f,0x02" ::: "memory"); }
static inline void poly_call_aarch64(void) { asm volatile(".byte 0x0f,0x3e,0x01" ::: "memory"); }
static inline void poly_ret(void) { asm volatile(".byte 0x0f,0x3d" ::: "memory"); }
static inline void poly_syscall_x86(void) { asm volatile(".byte 0x0f,0x39,0x00" ::: "memory"); }

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
}

static inline void write_rax(uint64_t value) {
  asm volatile("" :: "a"(value) : "memory");
}

int main(void) {
  const uint64_t sentinel = 0x1122334455667788ULL;

  puts("POLY_PROBE: start");
  write_rax(sentinel);

  poly_call_aarch64();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: polycall clobbered RAX\n");
    return 1;
  }

  poly_ret();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: polyret lost caller state\n");
    return 1;
  }

  write_rax(sentinel);
  poly_mode_aarch64();
  poly_mode_riscv();
  poly_mode_x86();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: mode round-trip lost RAX\n");
    return 1;
  }

  write_rax(sentinel);
  poly_mode_riscv();
  poly_mode_x86();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv round-trip lost RAX\n");
    return 1;
  }

  poly_mode_x86();
  poly_syscall_x86();
  if (read_rax() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: syscall return value mismatch\n");
    return 1;
  }

  puts("POLY_PROBE_OK");
  return 0;
}
