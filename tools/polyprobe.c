#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static inline void poly_mode_x86(void) { asm volatile(".byte 0x64,0x0f,0x0b,0x58,0x4d,0x4f,0x44,0x45" ::: "memory"); }
static inline void poly_mode_aarch64(void) { asm volatile(".byte 0x65,0x0f,0x0b,0x41,0x41,0x52,0x36,0x34" ::: "memory"); }
static inline void poly_mode_riscv(void) { asm volatile(".byte 0x66,0x0f,0x0b,0x52,0x49,0x53,0x43,0x56" ::: "memory"); }
static inline void poly_call_aarch64(void) { asm volatile(".byte 0xf2,0x0f,0x0b,0x43,0x41,0x4c,0x4c,0x41" ::: "memory"); }
static inline void poly_ret(void) { asm volatile(".byte 0xf3,0x0f,0x0b,0x52,0x45,0x54,0x52,0x4e" ::: "memory"); }
static inline void poly_syscall_x86(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x30" ::: "memory"); }
static inline void poly_aarch64_movz_x0_42(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x40,0x05,0x80,0xd2,0x00" ::: "memory"); }
static inline void poly_aarch64_add_x0_1(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x00,0x04,0x00,0x91,0x00" ::: "memory"); }
static inline void poly_aarch64_brk_strlen(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x20,0x00,0x20,0xd4,0x00" ::: "memory"); }
static inline void poly_riscv_addi_a0_17(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x13,0x05,0x10,0x01,0x00" ::: "memory"); }
static inline void poly_riscv_addi_a0_5(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x13,0x05,0x55,0x00,0x00" ::: "memory"); }
static inline void poly_riscv_addi_a7_1(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x93,0x08,0x10,0x00,0x00" ::: "memory"); }
static inline void poly_riscv_ebreak(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x73,0x00,0x10,0x00,0x00" ::: "memory"); }

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
}

static inline void write_rax(uint64_t value) {
  asm volatile("" :: "a"(value) : "memory");
}

static inline void write_rdi(uint64_t value) {
  asm volatile("" :: "D"(value) : "memory");
}

static void stage(const char *msg) {
  if (write(1, msg, strlen(msg)) < 0)
    return;
  ssize_t ignored = write(1, "\n", 1);
  (void) ignored;
}

int main(void) {
  const uint64_t sentinel = 0x1122334455667788ULL;

  stage("POLY_PROBE: start");

  stage("POLY_STAGE: call");
  write_rax(sentinel);
  poly_call_aarch64();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: polycall clobbered RAX\n");
    return 1;
  }
  poly_syscall_x86();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: polycall did not enter aarch64 mode\n");
    return 1;
  }

  stage("POLY_STAGE: ret");
  write_rax(sentinel);
  poly_ret();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: polyret lost caller state\n");
    return 1;
  }
  poly_syscall_x86();
  if (read_rax() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: polyret did not restore x86 mode\n");
    return 1;
  }

  stage("POLY_STAGE: mode1");
  write_rax(sentinel);
  poly_mode_aarch64();
  poly_mode_riscv();
  poly_mode_x86();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: mode round-trip lost RAX\n");
    return 1;
  }

  stage("POLY_STAGE: mode2");
  write_rax(sentinel);
  poly_mode_riscv();
  poly_mode_x86();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv round-trip lost RAX\n");
    return 1;
  }

  stage("POLY_STAGE: status");
  poly_mode_aarch64();
  poly_syscall_x86();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 syscall status mismatch\n");
    return 1;
  }
  poly_mode_riscv();
  poly_syscall_x86();
  if (read_rax() != 2) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv syscall status mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: foreign-insn");
  poly_mode_aarch64();
  poly_aarch64_movz_x0_42();
  poly_aarch64_add_x0_1();
  if (read_rax() != 43) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 instruction stream mismatch\n");
    return 1;
  }
  poly_mode_riscv();
  poly_riscv_addi_a0_17();
  poly_riscv_addi_a0_5();
  if (read_rax() != 22) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv instruction stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: mixed-insn");
  write_rax(40);
  poly_mode_aarch64();
  poly_aarch64_add_x0_1();
  poly_mode_riscv();
  poly_riscv_addi_a0_5();
  poly_mode_aarch64();
  poly_aarch64_add_x0_1();
  poly_mode_x86();
  if (read_rax() != 47) {
    fprintf(stderr, "POLY_PROBE_FAIL: mixed instruction stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: mixed-libcall");
  const char mixed_libcall_string[] = "polyglot";
  write_rdi((uint64_t) mixed_libcall_string);
  poly_mode_aarch64();
  poly_aarch64_brk_strlen();
  if (read_rax() != 8) {
    fprintf(stderr, "POLY_PROBE_FAIL: mixed aarch64 libcall mismatch\n");
    return 1;
  }
  write_rdi((uint64_t) mixed_libcall_string);
  poly_mode_riscv();
  poly_riscv_addi_a7_1();
  poly_riscv_ebreak();
  poly_mode_x86();
  if (read_rax() != 8) {
    fprintf(stderr, "POLY_PROBE_FAIL: mixed riscv libcall mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: syscall");
  poly_mode_x86();
  poly_syscall_x86();
  if (read_rax() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: syscall return value mismatch\n");
    return 1;
  }

  puts("POLY_PROBE_OK");
  return 0;
}
