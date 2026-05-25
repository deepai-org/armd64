#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define POLY_SYS_ARCH_PRCTL 158L
#define POLY_ARCH_SET_FS 0x1002
#define POLY_ARCH_GET_FS 0x1003

static inline void poly_mode_x86(void) { asm volatile(".byte 0x64,0x0f,0x0b,0x58,0x4d,0x4f,0x44,0x45" ::: "memory"); }
static inline void poly_mode_aarch64(void) { asm volatile(".byte 0x65,0x0f,0x0b,0x41,0x41,0x52,0x36,0x34" ::: "memory"); }
static inline void poly_mode_riscv(void) { asm volatile(".byte 0x66,0x0f,0x0b,0x52,0x49,0x53,0x43,0x56" ::: "memory"); }
static inline void poly_raw_aarch64(void) { asm volatile(".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34" ::: "memory"); }
static inline void poly_raw_riscv(void) { asm volatile(".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56" ::: "memory"); }
static inline void poly_call_aarch64(void) { asm volatile(".byte 0xf2,0x0f,0x0b,0x43,0x41,0x4c,0x4c,0x41" ::: "memory"); }
static inline void poly_call_riscv(void) { asm volatile(".byte 0xf2,0x0f,0x0b,0x43,0x41,0x4c,0x4c,0x52" ::: "memory"); }
static inline void poly_ret(void) { asm volatile(".byte 0xf3,0x0f,0x0b,0x52,0x45,0x54,0x52,0x4e" ::: "memory"); }
static inline void poly_syscall_x86(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x30" ::: "memory"); }
static inline void poly_syscall_number_status(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x31" ::: "memory"); }
static inline void poly_syscall_mode_status(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x32" ::: "memory"); }
static inline void poly_libcall_number_status(void) { asm volatile(".byte 0x3e,0x0f,0x0b,0x4c,0x49,0x42,0x43,0x31" ::: "memory"); }
static inline void poly_libcall_mode_status(void) { asm volatile(".byte 0x3e,0x0f,0x0b,0x4c,0x49,0x42,0x43,0x32" ::: "memory"); }
static inline void poly_switch_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x30" ::: "memory"); }
static inline void poly_foreign_insn_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x32" ::: "memory"); }
static inline void poly_foreign_syscall_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x33" ::: "memory"); }
static inline void poly_foreign_libcall_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x34" ::: "memory"); }
static inline void poly_aarch64_movz_x0_42(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x40,0x05,0x80,0xd2,0x00" ::: "memory"); }
static inline void poly_aarch64_add_x0_1(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x00,0x04,0x00,0x91,0x00" ::: "memory"); }
static inline void poly_aarch64_movz_x10_7(void) { asm volatile(".byte 0x67,0x0f,0x0b,0xea,0x00,0x80,0xd2,0x00" ::: "memory"); }
static inline void poly_aarch64_movz_x11_35(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x6b,0x04,0x80,0xd2,0x00" ::: "memory"); }
static inline void poly_aarch64_add_x12_x10_x11(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x4c,0x01,0x0b,0x8b,0x00" ::: "memory"); }
static inline void poly_aarch64_add_x0_x12_x10(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x80,0x01,0x0a,0x8b,0x00" ::: "memory"); }
static inline void poly_aarch64_add_x13_x10_5(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x4d,0x15,0x00,0x91,0x00" ::: "memory"); }
static inline void poly_aarch64_sub_x0_x13_8(void) { asm volatile(".byte 0x67,0x0f,0x0b,0xa0,0x21,0x00,0xd1,0x00" ::: "memory"); }
static inline void poly_aarch64_movz_x8_getpid(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x88,0x15,0x80,0xd2,0x00" ::: "memory"); }
static inline void poly_aarch64_svc(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x01,0x00,0x00,0xd4,0x00" ::: "memory"); }
static inline void poly_aarch64_brk_strlen(void) { asm volatile(".byte 0x67,0x0f,0x0b,0x20,0x00,0x20,0xd4,0x00" ::: "memory"); }
static inline void poly_riscv_addi_a0_17(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x13,0x05,0x10,0x01,0x00" ::: "memory"); }
static inline void poly_riscv_addi_a0_5(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x13,0x05,0x55,0x00,0x00" ::: "memory"); }
static inline void poly_riscv_addi_x16_9(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x13,0x08,0x90,0x00,0x00" ::: "memory"); }
static inline void poly_riscv_addi_x18_33(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x13,0x09,0x10,0x02,0x00" ::: "memory"); }
static inline void poly_riscv_add_x19_x16_x18(void) { asm volatile(".byte 0x26,0x0f,0x0b,0xb3,0x09,0x28,0x01,0x00" ::: "memory"); }
static inline void poly_riscv_add_a0_x19_x16(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x33,0x85,0x09,0x01,0x00" ::: "memory"); }
static inline void poly_riscv_addi_x5_neg3(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x93,0x02,0xd0,0xff,0x00" ::: "memory"); }
static inline void poly_riscv_addi_a0_x5_54(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x13,0x85,0x62,0x03,0x00" ::: "memory"); }
static inline void poly_riscv_addi_a7_1(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x93,0x08,0x10,0x00,0x00" ::: "memory"); }
static inline void poly_riscv_addi_a7_getpid(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x93,0x08,0xc0,0x0a,0x00" ::: "memory"); }
static inline void poly_riscv_ecall(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x73,0x00,0x00,0x00,0x00" ::: "memory"); }
static inline void poly_riscv_ebreak(void) { asm volatile(".byte 0x26,0x0f,0x0b,0x73,0x00,0x10,0x00,0x00" ::: "memory"); }
static inline void poly_raw_aarch64_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0xd2800140\n"
    ".long 0x91001c00\n"
    ".long 0xd42fffe0\n"
    ::: "memory");
}
static inline void poly_raw_riscv_probe(void) {
  asm volatile(
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x02900513\n"
    ".long 0x00150513\n"
    ".long 0x0000000b\n"
    ::: "memory");
}
static inline void poly_raw_aarch64_abi_args_probe(void) {
  asm volatile(
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0x8b020020\n"
    ".long 0x8b030000\n"
    ".long 0x8b040000\n"
    ".long 0x8b050000\n"
    ".long 0x8b060000\n"
    ".long 0xd42fffe0\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "memory");
}
static inline void poly_raw_riscv_abi_args_probe(void) {
  asm volatile(
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x00c58533\n"
    ".long 0x00d50533\n"
    ".long 0x00e50533\n"
    ".long 0x00f50533\n"
    ".long 0x01050533\n"
    ".long 0x0000000b\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "memory");
}
static inline void poly_raw_barrier_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0xd2800120\n"
    ".long 0xd5033fbf\n"
    ".long 0xd5033f9f\n"
    ".long 0xd5033fdf\n"
    ".long 0x91002000\n"
    ".long 0xd42fffe0\n"
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x01400513\n"
    ".long 0x0ff0000f\n"
    ".long 0x0000100f\n"
    ".long 0x00250513\n"
    ".long 0x0000000b\n"
    ::: "rax", "memory");
}

static inline void poly_raw_aarch64_fp64_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0x1e612800\n"
    ".long 0x1e613800\n"
    ".long 0x1e610800\n"
    ".long 0xd42fffe0\n"
    ::: "xmm0", "memory");
}

static inline void poly_raw_riscv_fp64_probe(void) {
  asm volatile(
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x02b50553\n"
    ".long 0x0ab50553\n"
    ".long 0x12b50553\n"
    ".long 0x0000000b\n"
    ::: "xmm0", "memory");
}

static inline long raw_arch_prctl(int code, uint64_t addr) {
  long ret;
  asm volatile("syscall"
    : "=a"(ret)
    : "a"(POLY_SYS_ARCH_PRCTL), "D"((long) code), "S"(addr)
    : "rcx", "r11", "memory");
  return ret;
}

#define POLY_SWITCH_STRESS_STEP() \
  do { \
    poly_mode_aarch64(); \
    poly_aarch64_add_x0_1(); \
    poly_mode_riscv(); \
    poly_riscv_addi_a0_5(); \
    poly_mode_aarch64(); \
    poly_aarch64_add_x0_1(); \
    poly_mode_x86(); \
  } while (0)

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
}

static inline uint64_t read_xmm0_u64(void) {
  uint64_t value;
  asm volatile("movq %%xmm0, %0" : "=r"(value));
  return value;
}

static inline uint64_t read_rsp(void) {
  uint64_t value;
  asm volatile("movq %%rsp, %0" : "=r"(value));
  return value;
}

static inline void write_rax(uint64_t value) {
  asm volatile("" :: "a"(value) : "memory");
}

static inline void write_rdi(uint64_t value) {
  asm volatile("" :: "D"(value) : "memory");
}

static inline void write_xmm0_u64(uint64_t value) {
  asm volatile("movq %0, %%xmm0" :: "r"(value) : "xmm0", "memory");
}

static inline void write_xmm1_u64(uint64_t value) {
  asm volatile("movq %0, %%xmm1" :: "r"(value) : "xmm1", "memory");
}

static int poly_thread_key_probe(void) {
  uint64_t original_fs = 0;
  uint64_t fake_tls[64] __attribute__((aligned(64)));
  uint64_t fake_initial_mode = 0;
  uint64_t fake_switched_mode = 0;
  uint64_t restored_mode = 0;
  int failure = 0;

  memset(fake_tls, 0, sizeof(fake_tls));
  if (raw_arch_prctl(POLY_ARCH_GET_FS, (uint64_t) &original_fs) != 0)
    return 1;

  poly_mode_aarch64();
  poly_syscall_x86();
  if (read_rax() != 1)
    return 2;

  if (raw_arch_prctl(POLY_ARCH_SET_FS, (uint64_t) fake_tls) != 0)
    return 3;

  poly_syscall_x86();
  fake_initial_mode = read_rax();
  poly_mode_riscv();
  poly_syscall_x86();
  fake_switched_mode = read_rax();

  if (raw_arch_prctl(POLY_ARCH_SET_FS, original_fs) != 0)
    _exit(120);

  poly_syscall_x86();
  restored_mode = read_rax();
  poly_mode_x86();

  if (fake_initial_mode != 0)
    failure = 4;
  else if (fake_switched_mode != 2)
    failure = 5;
  else if (restored_mode != 1)
    failure = 6;

  return failure;
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
  uint64_t rsp_before_call = read_rsp();
  poly_call_aarch64();
  uint64_t rax_after_call = read_rax();
  uint64_t rsp_after_call = read_rsp();
  if (rax_after_call != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: polycall clobbered RAX\n");
    return 1;
  }
  if (rsp_after_call != rsp_before_call - 16) {
    fprintf(stderr, "POLY_PROBE_FAIL: polycall did not push 16-byte mode frame on user stack\n");
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
  uint64_t rax_after_ret = read_rax();
  uint64_t rsp_after_ret = read_rsp();
  if (rax_after_ret != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: polyret lost caller state\n");
    return 1;
  }
  if (rsp_after_ret != rsp_before_call) {
    fprintf(stderr, "POLY_PROBE_FAIL: polyret did not pop mode frame from user stack\n");
    return 1;
  }
  poly_syscall_x86();
  if (read_rax() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: polyret did not restore x86 mode\n");
    return 1;
  }

  stage("POLY_STAGE: call-riscv");
  write_rax(sentinel);
  poly_call_riscv();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv polycall clobbered RAX\n");
    return 1;
  }
  poly_syscall_x86();
  if (read_rax() != 2) {
    fprintf(stderr, "POLY_PROBE_FAIL: polycall did not enter riscv mode\n");
    return 1;
  }
  write_rax(sentinel);
  poly_ret();
  if (read_rax() != sentinel) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv polyret lost caller state\n");
    return 1;
  }
  poly_syscall_x86();
  if (read_rax() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv polyret did not restore x86 mode\n");
    return 1;
  }

  stage("POLY_STAGE: nested-call");
  poly_call_aarch64();
  poly_syscall_x86();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: nested call did not enter aarch64 mode\n");
    return 1;
  }
  poly_call_riscv();
  poly_syscall_x86();
  if (read_rax() != 2) {
    fprintf(stderr, "POLY_PROBE_FAIL: nested call did not enter riscv mode\n");
    return 1;
  }
  poly_ret();
  poly_syscall_x86();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: nested return did not restore aarch64 mode\n");
    return 1;
  }
  poly_ret();
  poly_syscall_x86();
  if (read_rax() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: nested return did not restore x86 mode\n");
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

  stage("POLY_STAGE: thread-key");
  int thread_key_status = poly_thread_key_probe();
  if (thread_key_status != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: FS-keyed register state mismatch code=%d\n", thread_key_status);
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

  stage("POLY_STAGE: wide-regs");
  poly_mode_aarch64();
  poly_aarch64_movz_x10_7();
  poly_aarch64_movz_x11_35();
  poly_aarch64_add_x12_x10_x11();
  poly_aarch64_add_x0_x12_x10();
  if (read_rax() != 49) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 wide register stream mismatch\n");
    return 1;
  }
  poly_mode_riscv();
  poly_riscv_addi_x16_9();
  poly_riscv_addi_x18_33();
  poly_riscv_add_x19_x16_x18();
  poly_riscv_add_a0_x19_x16();
  if (read_rax() != 51) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv wide register stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: imm-regs");
  poly_mode_aarch64();
  poly_aarch64_movz_x10_7();
  poly_aarch64_add_x13_x10_5();
  poly_aarch64_sub_x0_x13_8();
  if (read_rax() != 4) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 generic immediate register mismatch\n");
    return 1;
  }
  poly_mode_riscv();
  poly_riscv_addi_x5_neg3();
  poly_riscv_addi_a0_x5_54();
  if (read_rax() != 51) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv generic addi register mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: raw-insn");
  poly_raw_aarch64_probe();
  if (read_rax() != 17) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 stream mismatch\n");
    return 1;
  }
  poly_raw_riscv_probe();
  if (read_rax() != 42) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: abi-args");
  poly_raw_aarch64_abi_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 ABI argument bridge mismatch\n");
    return 1;
  }
  poly_raw_riscv_abi_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv ABI argument bridge mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: fp64-args");
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  poly_raw_aarch64_fp64_probe();
  if (read_xmm0_u64() != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 FP64 argument bridge mismatch\n");
    return 1;
  }
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  poly_raw_riscv_fp64_probe();
  if (read_xmm0_u64() != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv FP64 argument bridge mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: raw-barrier");
  poly_raw_barrier_probe();
  if (read_rax() != 22) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw barrier stream mismatch\n");
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

  stage("POLY_STAGE: switch-stress");
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  write_rax(0);
  POLY_SWITCH_STRESS_STEP();
  POLY_SWITCH_STRESS_STEP();
  POLY_SWITCH_STRESS_STEP();
  POLY_SWITCH_STRESS_STEP();
  POLY_SWITCH_STRESS_STEP();
  POLY_SWITCH_STRESS_STEP();
  POLY_SWITCH_STRESS_STEP();
  POLY_SWITCH_STRESS_STEP();
  if (read_rax() != 56) {
    fprintf(stderr, "POLY_PROBE_FAIL: mode switch stress mismatch\n");
    return 1;
  }
  poly_switch_count_status();
  if (read_rax() != switches_before + 32) {
    fprintf(stderr, "POLY_PROBE_FAIL: mode switch count mismatch\n");
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
  poly_libcall_number_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 libcall number mismatch\n");
    return 1;
  }
  poly_libcall_mode_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 libcall mode mismatch\n");
    return 1;
  }
  write_rdi((uint64_t) mixed_libcall_string);
  poly_mode_riscv();
  poly_riscv_addi_a7_1();
  poly_riscv_ebreak();
  if (read_rax() != 8) {
    fprintf(stderr, "POLY_PROBE_FAIL: mixed riscv libcall mismatch\n");
    return 1;
  }
  poly_libcall_number_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv libcall number mismatch\n");
    return 1;
  }
  poly_libcall_mode_status();
  if (read_rax() != 2) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv libcall mode mismatch\n");
    return 1;
  }
  poly_mode_x86();

  stage("POLY_STAGE: mixed-syscall");
  poly_mode_aarch64();
  poly_aarch64_movz_x8_getpid();
  poly_aarch64_svc();
  if (read_rax() != 4242) {
    fprintf(stderr, "POLY_PROBE_FAIL: mixed aarch64 syscall mismatch\n");
    return 1;
  }
  poly_syscall_number_status();
  if (read_rax() != 172) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 syscall number mismatch\n");
    return 1;
  }
  poly_syscall_mode_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 syscall mode mismatch\n");
    return 1;
  }
  poly_mode_riscv();
  poly_riscv_addi_a7_getpid();
  poly_riscv_ecall();
  if (read_rax() != 4242) {
    fprintf(stderr, "POLY_PROBE_FAIL: mixed riscv syscall mismatch\n");
    return 1;
  }
  poly_syscall_number_status();
  if (read_rax() != 172) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv syscall number mismatch\n");
    return 1;
  }
  poly_syscall_mode_status();
  if (read_rax() != 2) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv syscall mode mismatch\n");
    return 1;
  }
  poly_mode_x86();

  stage("POLY_STAGE: syscall");
  poly_mode_x86();
  poly_syscall_x86();
  if (read_rax() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: syscall return value mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: counters");
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_mode_aarch64();
  poly_aarch64_movz_x0_42();
  poly_aarch64_add_x0_1();
  poly_foreign_insn_count_status();
  if (read_rax() != insns_before + 2) {
    fprintf(stderr, "POLY_PROBE_FAIL: foreign instruction count mismatch\n");
    return 1;
  }

  poly_foreign_syscall_count_status();
  uint64_t syscalls_before = read_rax();
  poly_aarch64_movz_x8_getpid();
  poly_aarch64_svc();
  poly_foreign_syscall_count_status();
  if (read_rax() != syscalls_before + 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: foreign syscall count mismatch\n");
    return 1;
  }

  const char counter_libcall_string[] = "count";
  write_rdi((uint64_t) counter_libcall_string);
  poly_foreign_libcall_count_status();
  uint64_t libcalls_before = read_rax();
  poly_aarch64_brk_strlen();
  poly_foreign_libcall_count_status();
  if (read_rax() != libcalls_before + 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: foreign libcall count mismatch\n");
    return 1;
  }
  poly_mode_x86();

  puts("POLY_PROBE_OK");
  return 0;
}
