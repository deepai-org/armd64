#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "polycpuid.h"

static inline void poly_mode_x86(void) { asm volatile(".byte 0x64,0x0f,0x0b,0x58,0x4d,0x4f,0x44,0x45" ::: "memory"); }
static inline void poly_syscall_x86(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x30" ::: "memory"); }
static inline void poly_syscall_number_status(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x31" ::: "memory"); }
static inline void poly_syscall_mode_status(void) { asm volatile(".byte 0x2e,0x0f,0x0b,0x53,0x59,0x53,0x43,0x32" ::: "memory"); }
static inline void poly_libcall_number_status(void) { asm volatile(".byte 0x3e,0x0f,0x0b,0x4c,0x49,0x42,0x43,0x31" ::: "memory"); }
static inline void poly_libcall_mode_status(void) { asm volatile(".byte 0x3e,0x0f,0x0b,0x4c,0x49,0x42,0x43,0x32" ::: "memory"); }
static inline void poly_switch_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x30" ::: "memory"); }
static inline void poly_foreign_insn_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x32" ::: "memory"); }
static inline void poly_foreign_syscall_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x33" ::: "memory"); }
static inline void poly_foreign_libcall_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x34" ::: "memory"); }
static inline void poly_trap_reason_status(void) { asm volatile(".byte 0x36,0x0f,0x0b,0x54,0x52,0x41,0x50,0x30" ::: "memory"); }
static inline void poly_trap_mode_status(void) { asm volatile(".byte 0x36,0x0f,0x0b,0x54,0x52,0x41,0x50,0x31" ::: "memory"); }
static inline void poly_trap_number_status(void) { asm volatile(".byte 0x36,0x0f,0x0b,0x54,0x52,0x41,0x50,0x32" ::: "memory"); }
static inline void poly_trap_arg0_status(void) { asm volatile(".byte 0x36,0x0f,0x0b,0x54,0x52,0x41,0x50,0x33" ::: "memory"); }

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

static void stage(const char *msg) {
  if (write(1, msg, strlen(msg)) < 0)
    return;
  ssize_t ignored = write(1, "\n", 1);
  (void) ignored;
}

static inline void raw_aarch64_arith_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0xd2800540\n" // movz x0,#42
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: "rax", "memory");
}

static inline void raw_riscv_arith_probe(void) {
  asm volatile(
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x01100513\n" // addi a0,zero,17
    ".long 0x00550513\n" // addi a0,a0,5
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: "rax", "memory");
}

static inline void raw_aarch64_wide_regs_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0xd28000ea\n" // movz x10,#7
    ".long 0xd280046b\n" // movz x11,#35
    ".long 0x8b0b014c\n" // add x12,x10,x11
    ".long 0x8b0a0180\n" // add x0,x12,x10
    ".long 0xd42fffe0\n"
    ::: "rax", "memory");
}

static inline void raw_riscv_wide_regs_probe(void) {
  asm volatile(
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x00900813\n" // addi x16,zero,9
    ".long 0x02100913\n" // addi x18,zero,33
    ".long 0x012809b3\n" // add x19,x16,x18
    ".long 0x01098533\n" // add a0,x19,x16
    ".long 0x0000000b\n"
    ::: "rax", "memory");
}

static inline void raw_aarch64_imm_regs_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0xd28000ea\n" // movz x10,#7
    ".long 0x9100154d\n" // add x13,x10,#5
    ".long 0xd10021a0\n" // sub x0,x13,#8
    ".long 0xd42fffe0\n"
    ::: "rax", "memory");
}

static inline void raw_riscv_imm_regs_probe(void) {
  asm volatile(
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0xffd00293\n" // addi x5,zero,-3
    ".long 0x03628513\n" // addi a0,x5,54
    ".long 0x0000000b\n"
    ::: "rax", "memory");
}

static inline void raw_aarch64_abi_args_probe(void) {
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

static inline void raw_riscv_abi_args_probe(void) {
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

static inline void pcall_aarch64_sysv_args_probe(void) {
  asm volatile(
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x40,0x0f,0x0b,0x50,0x43,0x41,0x36,0x34\n"
    "1:\n"
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0x8b020000\n" // add x0,x0,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
}

static inline void pcall_riscv_sysv_args_probe(void) {
  asm volatile(
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x40,0x0f,0x0b,0x50,0x43,0x52,0x56,0x36\n"
    "1:\n"
    ".long 0x00b50533\n" // add a0,a0,a1
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x00008067\n" // ret
    "2:\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
}

static inline void raw_fp64_aarch64_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0x1e612800\n"
    ".long 0x1e613800\n"
    ".long 0x1e610800\n"
    ".long 0xd42fffe0\n"
    ::: "xmm0", "memory");
}

static inline void raw_fp64_riscv_probe(void) {
  asm volatile(
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x02b50553\n"
    ".long 0x0ab50553\n"
    ".long 0x12b50553\n"
    ".long 0x0000000b\n"
    ::: "xmm0", "memory");
}

static inline void pcall_fp64_aarch64_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x40,0x0f,0x0b,0x50,0x43,0x41,0x36,0x34\n"
    "1:\n"
    ".long 0x1e612800\n"
    ".long 0x1e613800\n"
    ".long 0x1e610800\n"
    ".long 0xd65f03c0\n"
    "2:\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "xmm0", "memory");
}

static inline void pcall_fp64_riscv_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x40,0x0f,0x0b,0x50,0x43,0x52,0x56,0x36\n"
    "1:\n"
    ".long 0x02b50553\n"
    ".long 0x0ab50553\n"
    ".long 0x12b50553\n"
    ".long 0x00008067\n"
    "2:\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "xmm0", "memory");
}

static inline void raw_barrier_probe(void) {
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

static inline void raw_mixed_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0x91000400\n"
    ".long 0xd42fffc0\n"
    ".long 0x00550513\n"
    ".long 0x0000002b\n"
    ".long 0x91000400\n"
    ".long 0xd42fffe0\n"
    ::: "rax", "memory");
}

static inline void raw_switch_stress_step(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0x91000400\n"
    ".long 0xd42fffc0\n"
    ".long 0x00550513\n"
    ".long 0x0000002b\n"
    ".long 0x91000400\n"
    ".long 0xd42fffe0\n"
    ::: "rax", "memory");
}

static inline void raw_aarch64_strlen_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0xd4200020\n"
    ".long 0xd42fffe0\n"
    ::: "rax", "memory");
}

static inline void raw_riscv_strlen_probe(void) {
  asm volatile(
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x00100893\n"
    ".long 0x00100073\n"
    ".long 0x0000000b\n"
    ::: "rax", "memory");
}

static inline void raw_aarch64_getpid_probe(void) {
  asm volatile(
    ".byte 0x65,0x0f,0x0b,0x52,0x41,0x57,0x36,0x34\n"
    ".long 0xd2801588\n"
    ".long 0xd4000001\n"
    ".long 0xd42fffe0\n"
    ::: "rax", "memory");
}

static inline void raw_riscv_getpid_probe(void) {
  asm volatile(
    ".byte 0x66,0x0f,0x0b,0x52,0x41,0x57,0x52,0x56\n"
    ".long 0x0ac00893\n"
    ".long 0x00000073\n"
    ".long 0x0000000b\n"
    ::: "rax", "memory");
}

int main(void) {
  stage("POLY_PROBE: start");

  stage("POLY_STAGE: cpuid");
  struct poly_cpuid_regs poly_vendor = poly_read_cpuid(POLY_CPUID_BASE, 0);
  char vendor[13];
  poly_cpuid_vendor_string(&poly_vendor, vendor);
  if (poly_vendor.eax < POLY_CPUID_MAX ||
      !poly_cpuid_vendor_matches(&poly_vendor)) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID vendor mismatch max=0x%x vendor=%s\n",
      poly_vendor.eax, vendor);
    return 1;
  }
  struct poly_cpuid_regs poly_features =
    poly_read_cpuid(POLY_CPUID_BASE + 1, 0);
  if (poly_features.eax != POLY_CPUID_ABI_VERSION ||
      poly_features.ebx != poly_cpuid_expected_mode_mask() ||
      poly_features.ecx != poly_cpuid_expected_feature_mask() ||
      poly_features.edx != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID feature mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_features.eax, poly_features.ebx, poly_features.ecx, poly_features.edx);
    return 1;
  }

  stage("POLY_STAGE: x86-status");
  poly_mode_x86();
  poly_syscall_x86();
  if (read_rax() != POLY_MODE_X86) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 status mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: raw-insn");
  raw_aarch64_arith_probe();
  if (read_rax() != 43) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 instruction stream mismatch\n");
    return 1;
  }
  raw_riscv_arith_probe();
  if (read_rax() != 22) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv instruction stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: wide-regs");
  raw_aarch64_wide_regs_probe();
  if (read_rax() != 49) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 wide register stream mismatch\n");
    return 1;
  }
  raw_riscv_wide_regs_probe();
  if (read_rax() != 51) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv wide register stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: imm-regs");
  raw_aarch64_imm_regs_probe();
  if (read_rax() != 4) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 immediate register mismatch\n");
    return 1;
  }
  raw_riscv_imm_regs_probe();
  if (read_rax() != 51) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv immediate register mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: abi-args");
  raw_aarch64_abi_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 ABI argument bridge mismatch\n");
    return 1;
  }
  raw_riscv_abi_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv ABI argument bridge mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: pcall-abi");
  pcall_aarch64_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall aarch64 SysV argument bridge mismatch\n");
    return 1;
  }
  pcall_riscv_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall riscv SysV argument bridge mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: fp64-args");
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  raw_fp64_aarch64_probe();
  if (read_xmm0_u64() != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 FP64 bridge mismatch\n");
    return 1;
  }
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  raw_fp64_riscv_probe();
  if (read_xmm0_u64() != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv FP64 bridge mismatch\n");
    return 1;
  }
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  pcall_fp64_aarch64_probe();
  uint64_t pcall_aarch64_fp64 = read_xmm0_u64();
  if (pcall_aarch64_fp64 != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall aarch64 FP64 bridge mismatch got=0x%llx\n",
            (unsigned long long) pcall_aarch64_fp64);
    return 1;
  }
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  pcall_fp64_riscv_probe();
  uint64_t pcall_riscv_fp64 = read_xmm0_u64();
  if (pcall_riscv_fp64 != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall riscv FP64 bridge mismatch got=0x%llx\n",
            (unsigned long long) pcall_riscv_fp64);
    return 1;
  }

  stage("POLY_STAGE: raw-barrier");
  raw_barrier_probe();
  if (read_rax() != 22) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw barrier stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: mixed-raw");
  write_rax(40);
  raw_mixed_probe();
  if (read_rax() != 47) {
    fprintf(stderr, "POLY_PROBE_FAIL: mixed raw instruction stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: switch-stress");
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  write_rax(0);
  for (unsigned n = 0; n < 8; n++)
    raw_switch_stress_step();
  if (read_rax() != 56) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw switch stress result mismatch\n");
    return 1;
  }
  poly_switch_count_status();
  if (read_rax() != switches_before + 32) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw switch count mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: raw-libcall");
  const char libcall_string[] = "polyglot";
  write_rdi((uint64_t) libcall_string);
  raw_aarch64_strlen_probe();
  if (read_rax() != 8) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 libcall mismatch\n");
    return 1;
  }
  poly_libcall_number_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 libcall number mismatch\n");
    return 1;
  }
  poly_libcall_mode_status();
  if (read_rax() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 libcall mode mismatch\n");
    return 1;
  }
  write_rdi((uint64_t) libcall_string);
  raw_riscv_strlen_probe();
  if (read_rax() != 8) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv libcall mismatch\n");
    return 1;
  }
  poly_libcall_number_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv libcall number mismatch\n");
    return 1;
  }
  poly_libcall_mode_status();
  if (read_rax() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv libcall mode mismatch\n");
    return 1;
  }
  poly_trap_reason_status();
  if (read_rax() != POLY_TRAP_BREAK) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw break trap reason mismatch\n");
    return 1;
  }
  poly_trap_mode_status();
  if (read_rax() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw break trap mode mismatch\n");
    return 1;
  }
  poly_trap_number_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw break trap number mismatch\n");
    return 1;
  }
  poly_trap_arg0_status();
  if (read_rax() != (uint64_t) libcall_string) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw break trap arg0 mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: raw-syscall");
  raw_aarch64_getpid_probe();
  if (read_rax() != 4242) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 syscall mismatch\n");
    return 1;
  }
  poly_syscall_number_status();
  if (read_rax() != 172) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 syscall number mismatch\n");
    return 1;
  }
  poly_syscall_mode_status();
  if (read_rax() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 syscall mode mismatch\n");
    return 1;
  }
  raw_riscv_getpid_probe();
  if (read_rax() != 4242) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv syscall mismatch\n");
    return 1;
  }
  poly_syscall_number_status();
  if (read_rax() != 172) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv syscall number mismatch\n");
    return 1;
  }
  poly_syscall_mode_status();
  if (read_rax() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv syscall mode mismatch\n");
    return 1;
  }
  poly_trap_reason_status();
  if (read_rax() != POLY_TRAP_SYSCALL) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw syscall trap reason mismatch\n");
    return 1;
  }
  poly_trap_mode_status();
  if (read_rax() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw syscall trap mode mismatch\n");
    return 1;
  }
  poly_trap_number_status();
  if (read_rax() != 172) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw syscall trap number mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: counters");
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  raw_aarch64_arith_probe();
  poly_foreign_insn_count_status();
  if (read_rax() != insns_before + 3) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw foreign instruction count mismatch\n");
    return 1;
  }

  poly_foreign_syscall_count_status();
  uint64_t syscalls_before = read_rax();
  raw_aarch64_getpid_probe();
  poly_foreign_syscall_count_status();
  if (read_rax() != syscalls_before + 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw foreign syscall count mismatch\n");
    return 1;
  }

  write_rdi((uint64_t) libcall_string);
  poly_foreign_libcall_count_status();
  uint64_t libcalls_before = read_rax();
  raw_aarch64_strlen_probe();
  poly_foreign_libcall_count_status();
  if (read_rax() != libcalls_before + 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw foreign libcall count mismatch\n");
    return 1;
  }

  puts("POLY_PROBE_OK");
  return 0;
}
