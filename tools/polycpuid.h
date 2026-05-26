#ifndef POLYCPUID_H
#define POLYCPUID_H

#include <stdint.h>
#include <string.h>

enum {
  POLY_MODE_X86 = 0,
  POLY_MODE_RAW_AARCH64 = 3,
  POLY_MODE_RAW_RISCV = 4,
  POLY_TRAP_SYSCALL = 1,
  POLY_TRAP_BREAK = 2,
  POLY_CPUID_BASE = 0x40000000,
  POLY_CPUID_MAX = 0x40000002,
  POLY_CPUID_ABI_VERSION = 1,
  POLY_AARCH64_BRK_X86_ESCAPE = 0x7fff,
  POLY_AARCH64_BRK_RISCV_SWITCH = 0x7ffe,
  POLY_AARCH64_BRK_RISCV_CALL = 0x7ffd,
  POLY_AARCH64_BRK_RISCV_CALL_COMPACT_U32_F32 = 0x7ffc,
  POLY_AARCH64_BRK_RISCV_CALL_COMPACT_F32_U32 = 0x7ffb,
  POLY_RISCV_X86_ESCAPE = 0x0000000b,
  POLY_RISCV_AARCH64_SWITCH = 0x0000002b,
  POLY_RISCV_AARCH64_CALL = 0x0000005b,
  POLY_RISCV_AARCH64_CALL_COMPACT_U32_F32 = 0x0000107b,
  POLY_RISCV_AARCH64_CALL_COMPACT_F32_U32 = 0x0000207b,
  POLY_IMPORT_FUNC_X86_SLOT0 = 106,
  POLY_IMPORT_FUNC_X86_SLOT1 = 107,
  POLY_IMPORT_FUNC_X86_SLOT2 = 108,
  POLY_IMPORT_FUNC_X86_SLOT3 = 109,
  POLY_IMPORT_FUNC_X86_SLOT4 = 110,
  POLY_IMPORT_X86_DESCRIPTOR_SIZE = 16,
  POLY_IMPORT_CALL_STRIDE = 16,
  POLY_CPUID_FEATURE_RAW_AARCH64 = (1U << 0),
  POLY_CPUID_FEATURE_RAW_RISCV = (1U << 1),
  POLY_CPUID_FEATURE_NEUTRAL_SWITCH = (1U << 2),
  POLY_CPUID_FEATURE_NATIVE_RET = (1U << 3),
  POLY_CPUID_FEATURE_PCALL_SYSV = (1U << 4),
  POLY_CPUID_FEATURE_PCALL_SRET = (1U << 5),
  POLY_CPUID_FEATURE_FP_BRIDGE = (1U << 6),
  POLY_CPUID_FEATURE_TRAP_RECORDS = (1U << 7),
  POLY_CPUID_FEATURE_USER_RETURN_RESTORE = (1U << 8),
  POLY_CPUID_FEATURE_X86_TSO = (1U << 9),
  POLY_CPUID_FEATURE_THREAD_BANKS = (1U << 10),
  POLY_CPUID_FEATURE_COMPAT_TRAPS = (1U << 11),
  POLY_CPUID_FEATURE_X86_POLY_OPCODES = (1U << 12),
  POLY_CPUID_FEATURE_FPAIR32_RET = (1U << 13),
  POLY_CPUID_FEATURE_FPAIR32_ARG = (1U << 14),
  POLY_CPUID_FEATURE_HETERO_U64_F64 = (1U << 15),
  POLY_CPUID_FEATURE_HETERO_F64_U64 = (1U << 16),
  POLY_CPUID_FEATURE_HETERO_U64_F32 = (1U << 17),
  POLY_CPUID_FEATURE_HETERO_F32_U64 = (1U << 18),
  POLY_CPUID_FEATURE_COMPACT_U32_F32 = (1U << 19),
  POLY_CPUID_FEATURE_COMPACT_F32_U32 = (1U << 20),
  POLY_CPUID_FEATURE_NEUTRAL_COMPACT = (1U << 21),
  POLY_CPUID_FEATURE_X86_IMPORT_DESCRIPTORS = (1U << 22)
};

struct poly_cpuid_regs {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

static inline struct poly_cpuid_regs poly_read_cpuid(uint32_t leaf,
    uint32_t subleaf) {
  struct poly_cpuid_regs regs;
  asm volatile("cpuid"
    : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx), "=d"(regs.edx)
    : "a"(leaf), "c"(subleaf)
    : "memory");
  return regs;
}

static inline uint32_t poly_cpuid_expected_mode_mask(void) {
  return (1U << POLY_MODE_X86) |
    (1U << POLY_MODE_RAW_AARCH64) |
    (1U << POLY_MODE_RAW_RISCV);
}

static inline uint32_t poly_cpuid_expected_feature_mask(void) {
  return POLY_CPUID_FEATURE_RAW_AARCH64 |
    POLY_CPUID_FEATURE_RAW_RISCV |
    POLY_CPUID_FEATURE_NEUTRAL_SWITCH |
    POLY_CPUID_FEATURE_NATIVE_RET |
    POLY_CPUID_FEATURE_PCALL_SYSV |
    POLY_CPUID_FEATURE_PCALL_SRET |
    POLY_CPUID_FEATURE_FP_BRIDGE |
    POLY_CPUID_FEATURE_TRAP_RECORDS |
    POLY_CPUID_FEATURE_USER_RETURN_RESTORE |
    POLY_CPUID_FEATURE_X86_TSO |
    POLY_CPUID_FEATURE_THREAD_BANKS |
    POLY_CPUID_FEATURE_COMPAT_TRAPS |
    POLY_CPUID_FEATURE_X86_POLY_OPCODES |
    POLY_CPUID_FEATURE_FPAIR32_RET |
    POLY_CPUID_FEATURE_FPAIR32_ARG |
    POLY_CPUID_FEATURE_HETERO_U64_F64 |
    POLY_CPUID_FEATURE_HETERO_F64_U64 |
    POLY_CPUID_FEATURE_HETERO_U64_F32 |
    POLY_CPUID_FEATURE_HETERO_F32_U64 |
    POLY_CPUID_FEATURE_COMPACT_U32_F32 |
    POLY_CPUID_FEATURE_COMPACT_F32_U32 |
    POLY_CPUID_FEATURE_NEUTRAL_COMPACT |
    POLY_CPUID_FEATURE_X86_IMPORT_DESCRIPTORS;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_escape_leaf0(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_AARCH64_BRK_X86_ESCAPE |
    (POLY_AARCH64_BRK_RISCV_SWITCH << 16);
  regs.ebx = POLY_AARCH64_BRK_RISCV_CALL;
  regs.ecx = POLY_RISCV_X86_ESCAPE;
  regs.edx = POLY_RISCV_AARCH64_SWITCH;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_escape_leaf1(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_RISCV_AARCH64_CALL;
  regs.ebx = POLY_RISCV_AARCH64_CALL_COMPACT_U32_F32;
  regs.ecx = POLY_RISCV_AARCH64_CALL_COMPACT_F32_U32;
  regs.edx = 0;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_escape_leaf2(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_IMPORT_FUNC_X86_SLOT0;
  regs.ebx = POLY_IMPORT_FUNC_X86_SLOT4 - POLY_IMPORT_FUNC_X86_SLOT0 + 1;
  regs.ecx = POLY_IMPORT_X86_DESCRIPTOR_SIZE;
  regs.edx = POLY_IMPORT_CALL_STRIDE;
  return regs;
}

static inline void poly_cpuid_vendor_string(const struct poly_cpuid_regs *regs,
    char vendor[13]) {
  memcpy(vendor, &regs->ebx, 4);
  memcpy(vendor + 4, &regs->edx, 4);
  memcpy(vendor + 8, &regs->ecx, 4);
  vendor[12] = '\0';
}

static inline int poly_cpuid_vendor_matches(const struct poly_cpuid_regs *regs) {
  char vendor[13];
  poly_cpuid_vendor_string(regs, vendor);
  return strcmp(vendor, "PolyglotCPU!") == 0;
}

static inline int poly_cpuid_present(void) {
  struct poly_cpuid_regs regs = poly_read_cpuid(POLY_CPUID_BASE, 0);
  return regs.eax >= POLY_CPUID_MAX && poly_cpuid_vendor_matches(&regs);
}

#endif
