#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../include/polycpuid.h"

#define POLY_OP_TRAP_VECTOR_SET POLY_X86_CTRL_TRAP_VECTOR_SET_ASM
#define POLY_OP_TRAP_VECTOR_MODE_SET POLY_X86_CTRL_TRAP_VECTOR_MODE_SET_ASM
#define POLY_OP_TRAP_RETURN POLY_X86_CTRL_TRAP_RETURN_ASM
#define POLY_OP_ABI_SIGNATURE_SET POLY_X86_CTRL_ABI_SIGNATURE_SET_ASM
#define POLY_OP_ABI_SIGNATURE_GET POLY_X86_CTRL_ABI_SIGNATURE_GET_ASM
#define POLY_OP_MONITOR_PACKET_SET POLY_X86_CTRL_MONITOR_PACKET_SET_ASM
#define POLYBENCH_AARCH64_PCALL_SIG_IMM(slot) \
  POLY_AARCH64_CTRL_CALL_SIG_IMM(slot)
#define POLYBENCH_RISCV_PCALL_SIG_IMM(slot) \
  POLY_RISCV_CTRL_CALL_SIG_IMM(slot)
#define POLYBENCH_X86_PCALL_SIG_IMM(slot) \
  POLY_X86_CTRL_PCALL_SIG_IMM(slot)

enum {
  POLY_ARCH_AARCH64 = POLY_FRONTEND_AARCH64,
  POLY_ARCH_RISCV = POLY_FRONTEND_RISCV,
  POLY_ARCH_RISCV_COMPRESSED = 3,
  LOOP_ITERS = 200,
  POLYBENCH_LOOP_EXPECTED_SWITCH_DELTA = 3,
  POLYBENCH_LOOP_MAX_RAW_INSNS = 410,
  POLYBENCH_MIXED_EXPECTED_SWITCH_DELTA = 4,
  POLYBENCH_MIXED_MAX_SWITCH_DELTA = 4,
  POLYBENCH_MIXED_MAX_RAW_INSNS = 8,
  POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA = 5,
  POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA = 5,
  POLYBENCH_CROSS_CALL_MAX_RAW_INSNS = 16,
  POLYBENCH_CROSS_CALL_VEC128_EXPECTED_SWITCH_DELTA = 3,
  POLYBENCH_CROSS_CALL_VEC128_MAX_RAW_INSNS = 25,
  POLYBENCH_NEUTRAL_PCALL_EXPECTED_SWITCH_DELTA = 3,
  POLYBENCH_NEUTRAL_PCALL_WRAPPER_SWITCH_DELTA = 1,
  POLYBENCH_NEUTRAL_PCALL_EXPECTED_INNER_SWITCH_DELTA = 2,
  POLYBENCH_NEUTRAL_PCALL_MAX_RAW_INSNS = 32,
  POLYBENCH_NEUTRAL_PCALL_FP64_MAX_RAW_INSNS = 32,
  POLYBENCH_NEUTRAL_PCALL_FP32_MAX_RAW_INSNS = 32,
  POLYBENCH_NESTED_CROSS_CALL_EXPECTED_SWITCH_DELTA = 7,
  POLYBENCH_NESTED_CROSS_CALL_MAX_SWITCH_DELTA = 7,
  POLYBENCH_NESTED_CROSS_CALL_MAX_RAW_INSNS = 26,
  POLYBENCH_DIRECT_X86_PCALL_EXPECTED_SWITCH_DELTA = 5,
  POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA = 5,
  POLYBENCH_DIRECT_X86_PCALL_MAX_RAW_INSNS = 18,
  POLYBENCH_X86_PCALL_SIGNATURE_EXPECTED_SWITCH_DELTA = 2,
  POLYBENCH_X86_PCALL_SIGNATURE_MAX_SWITCH_DELTA = 2,
  POLYBENCH_X86_PCALL_SIGNATURE_MAX_RAW_INSNS = 7,
  POLYBENCH_X86_PCALL_SRET_SIGNATURE_MAX_RAW_INSNS = 10,
  POLYBENCH_X86_PCALL_VEC128_SIGNATURE_MAX_RAW_INSNS = 14,
  POLYBENCH_CROSS_CALL_SIGNATURE_MAX_RAW_INSNS = 16,
  POLYBENCH_DIRECT_X86_LIBCALL_EXPECTED_SWITCH_DELTA = 7,
  POLYBENCH_DIRECT_X86_LIBCALL_MAX_SWITCH_DELTA = 7,
  POLYBENCH_DIRECT_X86_MEMOPS_EXPECTED_SWITCH_DELTA = 11,
  POLYBENCH_DIRECT_X86_MEMOPS_MAX_SWITCH_DELTA = 11
};

typedef uint32_t polybench_vec128_u32 __attribute__((vector_size(16)));

union polybench_vec128_u32_bits {
  polybench_vec128_u32 v;
  uint32_t u[4];
};

struct polybench_sret4 {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t d;
};

static uint32_t polybench_native_signature_slot = 3;
static uint32_t polybench_fp64_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_FP64;
static uint32_t polybench_fp32_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_FP32;
static uint32_t polybench_sret_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_SRET_X86_SYSV_REGS;

struct polybench_monitor_packet {
  struct poly_trap_packet trap;
  uint64_t args[POLY_TRAP_PACKET_ARG_COUNT];
};

static struct polybench_monitor_packet polybench_monitor_packet
  __attribute__((aligned(64)));
static const uint64_t polybench_aarch64_trap_args[POLY_TRAP_PACKET_ARG_COUNT] =
  {77, 78, 79, 80, 81, 82, 88, 99};
static const uint64_t polybench_riscv_syscall_args[POLY_TRAP_PACKET_ARG_COUNT] =
  {77, 78, 79, 80, 81, 82, 88, 172};

static inline void poly_mode_x86(void) {
  asm volatile(
    "movq %%r15, %%r11\n"
    "xorl %%r15d, %%r15d\n"
    POLY_X86_CTRL_PENTER_MODE_ASM
    "movq %%r11, %%r15\n"
    ::: "r11", "memory");
}
static inline uint64_t poly_switch_count_status_value(void) {
  uint64_t value;
  asm volatile(POLY_X86_CTRL_SWITCH_COUNT_STATUS_ASM
      : "=a"(value)
      :
      : "memory");
  return value;
}
static inline uint64_t poly_foreign_insn_count_status_value(void) {
  uint64_t value;
  asm volatile(POLY_X86_CTRL_FOREIGN_INSN_COUNT_STATUS_ASM
      : "=a"(value)
      :
      : "memory");
  return value;
}
static uint64_t polybench_saved_r15;
#define POLYBENCH_SAVE_R15() \
  asm volatile("movq %%r15, %0" : "=m"(polybench_saved_r15) :: "memory")
#define POLYBENCH_RESTORE_R15() \
  asm volatile("movq %0, %%r15" :: "m"(polybench_saved_r15) : "memory")
#define POLYBENCH_CALL_SAVE_R15(lvalue, call_expr) do { \
  POLYBENCH_SAVE_R15(); \
  (lvalue) = (call_expr); \
  POLYBENCH_RESTORE_R15(); \
} while (0)

static int check_polybench_arch_state_contract(void) {
  struct poly_cpuid_contract_failure failure;
  if (!poly_cpuid_verify_arch_state_contract(&failure)) {
    fprintf(stderr,
      "POLYBENCH_FAIL: %s mismatch leaf=0x%x subleaf=%u got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      failure.name, failure.leaf, failure.subleaf,
      failure.actual.eax, failure.actual.ebx,
      failure.actual.ecx, failure.actual.edx,
      failure.expected.eax, failure.expected.ebx,
      failure.expected.ecx, failure.expected.edx);
    return -1;
  }
  return 0;
}

static int check_polybench_contract(void) {
  const struct poly_cpuid_regs base = poly_read_cpuid(POLY_CPUID_BASE, 0);
  if (base.eax < POLY_CPUID_MAX || !poly_cpuid_vendor_matches(&base)) {
    fprintf(stderr,
      "POLYBENCH_FAIL: poly CPUID missing base=(0x%x,0x%x,0x%x,0x%x)\n",
      base.eax, base.ebx, base.ecx, base.edx);
    return -1;
  }

  const struct poly_cpuid_regs features =
    poly_read_cpuid(POLY_CPUID_BASE + 1, 0);
  const uint32_t expected_modes = poly_cpuid_expected_mode_mask();
  const uint32_t expected_features = poly_cpuid_expected_feature_mask();
  const uint32_t forbidden_features = poly_cpuid_forbidden_feature_mask();
  if (features.eax != POLY_CPUID_ABI_VERSION ||
      features.ebx != expected_modes ||
      features.ecx != expected_features ||
      features.edx != POLY_STATE_XSAVE_COMPONENT_ARCH ||
      (features.ecx & forbidden_features) != 0) {
    fprintf(stderr,
      "POLYBENCH_FAIL: poly CPUID feature mismatch features=(%u,0x%x,0x%x,0x%x) expected=(%u,0x%x,0x%x,%u)\n",
      features.eax, features.ebx, features.ecx, features.edx,
      POLY_CPUID_ABI_VERSION, expected_modes, expected_features,
      POLY_STATE_XSAVE_COMPONENT_ARCH);
    return -1;
  }

  if (check_polybench_arch_state_contract() < 0)
    return -1;

  const struct poly_cpuid_regs abi_bridge =
    poly_read_cpuid(POLY_CPUID_BASE + 9, 0);
  const struct poly_cpuid_regs expected_abi_bridge =
    poly_cpuid_expected_abi_bridge_leaf();
  const uint32_t forbidden_abi_bridge =
    poly_cpuid_forbidden_abi_bridge_mask();
  const uint32_t gpr_arg_count = abi_bridge.ecx & 0xffU;
  const uint32_t fp_arg_count = (abi_bridge.ecx >> 8) & 0xffU;
  const uint32_t stack_align = (abi_bridge.ecx >> 16) & 0xffffU;
  const uint32_t descriptor_size = abi_bridge.edx & 0xffffU;
  const uint32_t reserved = abi_bridge.edx >> 16;
  if (abi_bridge.eax != expected_abi_bridge.eax ||
      abi_bridge.ebx != expected_abi_bridge.ebx ||
      abi_bridge.ecx != expected_abi_bridge.ecx ||
      abi_bridge.edx != expected_abi_bridge.edx ||
      (abi_bridge.ebx & forbidden_abi_bridge) != 0 ||
      gpr_arg_count != POLY_ABI_BRIDGE_GPR_ARG_COUNT ||
      fp_arg_count != POLY_ABI_BRIDGE_FP_ARG_COUNT ||
      stack_align != POLY_ABI_BRIDGE_STACK_ALIGN ||
      descriptor_size != 0 ||
      reserved != 0) {
    fprintf(stderr,
      "POLYBENCH_FAIL: CPU ABI bridge mismatch abi=(%u,0x%x,0x%x,0x%x) expected=(%u,0x%x,0x%x,0x%x)\n",
      abi_bridge.eax, abi_bridge.ebx, abi_bridge.ecx, abi_bridge.edx,
      expected_abi_bridge.eax, expected_abi_bridge.ebx,
      expected_abi_bridge.ecx, expected_abi_bridge.edx);
    return -1;
  }

  return 0;
}

static inline uint64_t poly_abi_signature_set(uint64_t slot, uint64_t kind) {
  uint64_t rax = slot;
  uint64_t rdx = poly_abi_signature_control_value(kind);
  asm volatile(POLY_OP_ABI_SIGNATURE_SET
    : "+a"(rax), "+d"(rdx)
    :
    : "memory");
  return rax;
}

static inline uint64_t poly_abi_signature_get(uint64_t slot) {
  uint64_t rax = slot;
  asm volatile(POLY_OP_ABI_SIGNATURE_GET
    : "+a"(rax)
    :
    : "memory");
  return rax;
}

static int setup_polybench_signature_slots(void) {
  const struct poly_cpuid_regs signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 7);
  const struct poly_cpuid_regs expected_signature =
    poly_cpuid_expected_escape_leaf7();
  const struct poly_cpuid_regs fp64_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 22);
  const struct poly_cpuid_regs expected_fp64_signature =
    poly_cpuid_expected_escape_leaf22();
  const struct poly_cpuid_regs fp32_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 23);
  const struct poly_cpuid_regs expected_fp32_signature =
    poly_cpuid_expected_escape_leaf23();
  const struct poly_cpuid_regs sret_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 24);
  const struct poly_cpuid_regs expected_sret_signature =
    poly_cpuid_expected_escape_leaf24();
  const struct poly_cpuid_regs fp128_ret_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 25);
  const struct poly_cpuid_regs expected_fp128_ret_signature =
    poly_cpuid_expected_escape_leaf25();
  const struct poly_cpuid_regs hfa32_ret_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 26);
  const struct poly_cpuid_regs expected_hfa32_ret_signature =
    poly_cpuid_expected_escape_leaf26();
  const struct poly_cpuid_regs hfa32_arg_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 27);
  const struct poly_cpuid_regs expected_hfa32_arg_signature =
    poly_cpuid_expected_escape_leaf27();
  const struct poly_cpuid_regs native_sret_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 28);
  const struct poly_cpuid_regs expected_native_sret_signature =
    poly_cpuid_expected_escape_leaf28();
  const struct poly_cpuid_regs hfa64_ret_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 29);
  const struct poly_cpuid_regs expected_hfa64_ret_signature =
    poly_cpuid_expected_escape_leaf29();
  const uint32_t native_slot = (signature.ecx >> 24) & 0xffU;
  const uint32_t native_kind = (signature.edx >> 24) & 0xffU;
  const uint32_t fp64_slot = fp64_signature.edx;
  const uint32_t fp32_slot = fp32_signature.edx;
  const uint32_t sret_slot = sret_signature.eax;
  if (signature.eax != expected_signature.eax ||
      signature.ebx != expected_signature.ebx ||
      signature.ecx != expected_signature.ecx ||
      signature.edx != expected_signature.edx ||
      native_slot >= signature.ebx ||
      native_kind != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr,
      "POLYBENCH_FAIL: native signature manifest mismatch sig=(0x%x,%u,0x%x,0x%x)\n",
      signature.eax, signature.ebx, signature.ecx, signature.edx);
    return -1;
  }
  if (fp64_signature.eax != expected_fp64_signature.eax ||
      fp64_signature.ebx != expected_fp64_signature.ebx ||
      fp64_signature.ecx != expected_fp64_signature.ecx ||
      fp64_signature.edx != expected_fp64_signature.edx ||
      fp64_slot >= signature.ebx) {
    fprintf(stderr,
      "POLYBENCH_FAIL: FP64 signature manifest mismatch fp64=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      fp64_signature.eax, fp64_signature.ebx, fp64_signature.ecx,
      fp64_signature.edx, expected_fp64_signature.eax,
      expected_fp64_signature.ebx, expected_fp64_signature.ecx,
      expected_fp64_signature.edx);
    return -1;
  }
  if (fp32_signature.eax != expected_fp32_signature.eax ||
      fp32_signature.ebx != expected_fp32_signature.ebx ||
      fp32_signature.ecx != expected_fp32_signature.ecx ||
      fp32_signature.edx != expected_fp32_signature.edx ||
      fp32_slot >= signature.ebx) {
    fprintf(stderr,
      "POLYBENCH_FAIL: FP32 signature manifest mismatch fp32=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      fp32_signature.eax, fp32_signature.ebx, fp32_signature.ecx,
      fp32_signature.edx, expected_fp32_signature.eax,
      expected_fp32_signature.ebx, expected_fp32_signature.ecx,
      expected_fp32_signature.edx);
    return -1;
  }
  if (sret_signature.eax != expected_sret_signature.eax ||
      sret_signature.ebx != expected_sret_signature.ebx ||
      sret_signature.ecx != expected_sret_signature.ecx ||
      sret_signature.edx != expected_sret_signature.edx ||
      sret_slot >= signature.ebx ||
      poly_abi_signature_get(sret_slot) !=
        POLY_ABI_SIGNATURE_KIND_SRET_X86_SYSV_REGS) {
    fprintf(stderr,
      "POLYBENCH_FAIL: SRET signature manifest mismatch sret=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x) kind=%llu\n",
      sret_signature.eax, sret_signature.ebx, sret_signature.ecx,
      sret_signature.edx, expected_sret_signature.eax,
      expected_sret_signature.ebx, expected_sret_signature.ecx,
      expected_sret_signature.edx,
      (unsigned long long) poly_abi_signature_get(sret_slot));
    return -1;
  }
  if (fp128_ret_signature.eax != expected_fp128_ret_signature.eax ||
      fp128_ret_signature.ebx != expected_fp128_ret_signature.ebx ||
      fp128_ret_signature.ecx != expected_fp128_ret_signature.ecx ||
      fp128_ret_signature.edx != expected_fp128_ret_signature.edx) {
    fprintf(stderr,
      "POLYBENCH_FAIL: FP128 return signature manifest mismatch got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      fp128_ret_signature.eax, fp128_ret_signature.ebx,
      fp128_ret_signature.ecx, fp128_ret_signature.edx,
      expected_fp128_ret_signature.eax, expected_fp128_ret_signature.ebx,
      expected_fp128_ret_signature.ecx, expected_fp128_ret_signature.edx);
    return -1;
  }
  if (hfa32_ret_signature.eax != expected_hfa32_ret_signature.eax ||
      hfa32_ret_signature.ebx != expected_hfa32_ret_signature.ebx ||
      hfa32_ret_signature.ecx != expected_hfa32_ret_signature.ecx ||
      hfa32_ret_signature.edx != expected_hfa32_ret_signature.edx) {
    fprintf(stderr,
      "POLYBENCH_FAIL: AArch64 HFA32 return signature manifest mismatch got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      hfa32_ret_signature.eax, hfa32_ret_signature.ebx,
      hfa32_ret_signature.ecx, hfa32_ret_signature.edx,
      expected_hfa32_ret_signature.eax, expected_hfa32_ret_signature.ebx,
      expected_hfa32_ret_signature.ecx, expected_hfa32_ret_signature.edx);
    return -1;
  }
  if (hfa32_arg_signature.eax != expected_hfa32_arg_signature.eax ||
      hfa32_arg_signature.ebx != expected_hfa32_arg_signature.ebx ||
      hfa32_arg_signature.ecx != expected_hfa32_arg_signature.ecx ||
      hfa32_arg_signature.edx != expected_hfa32_arg_signature.edx) {
    fprintf(stderr,
      "POLYBENCH_FAIL: AArch64 HFA32 argument signature manifest mismatch got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      hfa32_arg_signature.eax, hfa32_arg_signature.ebx,
      hfa32_arg_signature.ecx, hfa32_arg_signature.edx,
      expected_hfa32_arg_signature.eax, expected_hfa32_arg_signature.ebx,
      expected_hfa32_arg_signature.ecx, expected_hfa32_arg_signature.edx);
    return -1;
  }
  if (native_sret_signature.eax != expected_native_sret_signature.eax ||
      native_sret_signature.ebx != expected_native_sret_signature.ebx ||
      native_sret_signature.ecx != expected_native_sret_signature.ecx ||
      native_sret_signature.edx != expected_native_sret_signature.edx ||
      native_sret_signature.eax >= signature.ebx ||
      poly_abi_signature_get(native_sret_signature.eax) !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS) {
    fprintf(stderr,
      "POLYBENCH_FAIL: native SRET signature manifest mismatch got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x) kind=%llu\n",
      native_sret_signature.eax, native_sret_signature.ebx,
      native_sret_signature.ecx, native_sret_signature.edx,
      expected_native_sret_signature.eax, expected_native_sret_signature.ebx,
      expected_native_sret_signature.ecx, expected_native_sret_signature.edx,
      (unsigned long long) poly_abi_signature_get(native_sret_signature.eax));
    return -1;
  }
  if (hfa64_ret_signature.eax != expected_hfa64_ret_signature.eax ||
      hfa64_ret_signature.ebx != expected_hfa64_ret_signature.ebx ||
      hfa64_ret_signature.ecx != expected_hfa64_ret_signature.ecx ||
      hfa64_ret_signature.edx != expected_hfa64_ret_signature.edx) {
    fprintf(stderr,
      "POLYBENCH_FAIL: AArch64 HFA64 return signature manifest mismatch got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      hfa64_ret_signature.eax, hfa64_ret_signature.ebx,
      hfa64_ret_signature.ecx, hfa64_ret_signature.edx,
      expected_hfa64_ret_signature.eax, expected_hfa64_ret_signature.ebx,
      expected_hfa64_ret_signature.ecx, expected_hfa64_ret_signature.edx);
    return -1;
  }
  if (poly_abi_signature_set(native_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0 ||
      poly_abi_signature_set(fp64_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64) != 0 ||
      poly_abi_signature_set(fp32_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP32) != 0) {
    fprintf(stderr,
      "POLYBENCH_FAIL: signature slot setup failed native=%u fp64=%u fp32=%u\n",
      native_slot, fp64_slot, fp32_slot);
    return -1;
  }
  polybench_native_signature_slot = native_slot;
  polybench_fp64_signature_slot = fp64_slot;
  polybench_fp32_signature_slot = fp32_slot;
  polybench_sret_signature_slot = sret_slot;
  return 0;
}

static inline void poly_trap_vector_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline void poly_trap_vector_mode_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
}

static inline void poly_monitor_packet_set_value(uint64_t value) {
  asm volatile(POLY_OP_MONITOR_PACKET_SET :: "a"(value) : "memory");
}

static void emit_u16(uint8_t *code, size_t *offset, uint16_t value) {
  code[(*offset)++] = (uint8_t) (value & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 8) & 0xff);
}

static void emit_u32(uint8_t *code, size_t *offset, uint32_t value) {
  code[(*offset)++] = (uint8_t) (value & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 8) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 16) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 24) & 0xff);
}

static void store_u32(uint8_t *code, size_t offset, uint32_t value) {
  code[offset] = (uint8_t) (value & 0xff);
  code[offset + 1] = (uint8_t) ((value >> 8) & 0xff);
  code[offset + 2] = (uint8_t) ((value >> 16) & 0xff);
  code[offset + 3] = (uint8_t) ((value >> 24) & 0xff);
}

static void store_u64(uint8_t *code, size_t offset, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    code[offset + n] = (uint8_t) (value >> (n * 8));
}

static void emit_u64(uint8_t *code, size_t *offset, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    code[(*offset)++] = (uint8_t) (value >> (n * 8));
}

static void emit_bytes(uint8_t *code, size_t *offset, const uint8_t *bytes, size_t size) {
  memcpy(code + *offset, bytes, size);
  *offset += size;
}

static void emit_aarch64_movabs(uint8_t *code, size_t *offset, uint32_t rd,
    uint64_t value) {
  emit_u32(code, offset, 0xd2800000U | (((uint32_t) value & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2a00000U | ((((uint32_t) (value >> 16)) & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2c00000U | ((((uint32_t) (value >> 32)) & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2e00000U | ((((uint32_t) (value >> 48)) & 0xffffU) << 5) | rd);
}

static void emit_aarch64_pcall_sig(uint8_t *code, size_t *offset,
    uint32_t signature_slot);
static uint32_t riscv_fadd_d(uint32_t rd, uint32_t rs1, uint32_t rs2);
static uint32_t riscv_fadd_s(uint32_t rd, uint32_t rs1, uint32_t rs2);

static size_t emit_x86_movabs_r10(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xba;
  size_t imm_offset = *offset;
  emit_u64(code, offset, value);
  return imm_offset;
}

static size_t emit_x86_movabs_rbx(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x48;
  code[(*offset)++] = 0xbb;
  size_t imm_offset = *offset;
  emit_u64(code, offset, value);
  return imm_offset;
}

static size_t emit_x86_movabs_r11(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbb;
  size_t imm_offset = *offset;
  emit_u64(code, offset, value);
  return imm_offset;
}

static size_t emit_x86_movabs_r15(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbf;
  size_t imm_offset = *offset;
  emit_u64(code, offset, value);
  return imm_offset;
}

static uint32_t aarch64_mov_reg(uint32_t rd, uint32_t rn) {
  return 0xaa0003e0U | ((rn & 0x1fU) << 16) | (rd & 0x1fU);
}

static uint32_t aarch64_fadd_d(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0x1e602800U | ((rm & 0x1fU) << 16) |
    ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_fadd_s(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0x1e202800U | ((rm & 0x1fU) << 16) |
    ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_fsub_s(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0x1e203800U | ((rm & 0x1fU) << 16) |
    ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_fmul_s(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0x1e200800U | ((rm & 0x1fU) << 16) |
    ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_fsub_d(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0x1e603800U | ((rm & 0x1fU) << 16) |
    ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_fmul_d(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0x1e600800U | ((rm & 0x1fU) << 16) |
    ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_ldr_x_sp(uint32_t rt, uint32_t offset) {
  return 0xf94003e0U | (((offset / 8U) & 0xfffU) << 10) | (rt & 0x1fU);
}

static uint32_t aarch64_str_x_sp(uint32_t rt, uint32_t offset) {
  return 0xf90003e0U | (((offset / 8U) & 0xfffU) << 10) | (rt & 0x1fU);
}

static uint32_t aarch64_str_x_imm(uint32_t rt, uint32_t rn,
    uint32_t offset) {
  return 0xf9000000U | (((offset / 8U) & 0xfffU) << 10) |
    ((rn & 0x1fU) << 5) | (rt & 0x1fU);
}

static uint32_t aarch64_ldr_d_sp(uint32_t rt, uint32_t offset) {
  return 0xfd4003e0U | (((offset / 8U) & 0xfffU) << 10) | (rt & 0x1fU);
}

static uint32_t aarch64_ubfm(uint32_t rd, uint32_t rn, uint32_t immr,
    uint32_t imms) {
  return 0xd3400000U | ((immr & 0x3fU) << 16) |
    ((imms & 0x3fU) << 10) | ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_lsr_imm(uint32_t rd, uint32_t rn, uint32_t shift) {
  return aarch64_ubfm(rd, rn, shift, 63);
}

static uint32_t aarch64_lsl_imm(uint32_t rd, uint32_t rn, uint32_t shift) {
  return aarch64_ubfm(rd, rn, (64U - shift) & 0x3fU, 63U - shift);
}

static uint32_t aarch64_orr_reg(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0xaa000000U | ((rm & 0x1fU) << 16) |
    ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_fmov_s_from_w(uint32_t rd, uint32_t rn) {
  return 0x1e270000U | ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_fmov_w_from_s(uint32_t rd, uint32_t rn) {
  return 0x1e260000U | ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint64_t fp64_to_bits(double value) {
  union {
    double d;
    uint64_t u;
  } fp;
  fp.d = value;
  return fp.u;
}

static uint32_t fp32_to_bits(float value) {
  union {
    float f;
    uint32_t u;
  } fp;
  fp.f = value;
  return fp.u;
}

static int poly_is_raw_foreign_mode(uint64_t mode) {
  return mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV;
}

static int polybench_monitor_packet_valid(
    const struct polybench_monitor_packet *packet) {
  if (packet->trap.resume_pc == 0 ||
      packet->trap.reserved[0] != 0 ||
      packet->trap.reserved[1] != 0 ||
      (packet->trap.flags & POLY_TRAP_PACKET_REQUIRED_FLAGS) !=
        POLY_TRAP_PACKET_REQUIRED_FLAGS) {
    fprintf(stderr,
      "POLYBENCH_FAIL: monitor packet invalid reason=%u mode=%u number=%llu selector=%llu pc=0x%llx resume=0x%llx flags=0x%llx\n",
      packet->trap.reason, packet->trap.source_mode,
      (unsigned long long) packet->trap.number,
      (unsigned long long) packet->trap.selector,
      (unsigned long long) packet->trap.trap_pc,
      (unsigned long long) packet->trap.resume_pc,
      (unsigned long long) packet->trap.flags);
    return 0;
  }
  return 1;
}

static int polybench_trap_args_equal(
    const uint64_t got[POLY_TRAP_PACKET_ARG_COUNT],
    const uint64_t expected[POLY_TRAP_PACKET_ARG_COUNT]) {
  for (unsigned n = 0; n < POLY_TRAP_PACKET_ARG_COUNT; n++) {
    if (got[n] != expected[n])
      return 0;
  }
  return 1;
}

__attribute__((noinline, used))
uint64_t polybench_trap_vector_dispatch(void) {
  const struct polybench_monitor_packet *packet = &polybench_monitor_packet;
  const uint64_t reason = packet->trap.reason;
  const uint64_t mode = packet->trap.source_mode;
  const uint64_t number = packet->trap.number;

  if (!poly_is_raw_foreign_mode(mode))
    return (uint64_t) -38;
  if (!polybench_monitor_packet_valid(packet))
    return (uint64_t) -38;
  if (reason == POLY_TRAP_SYSCALL && number == 172 &&
      ((mode == POLY_MODE_RAW_AARCH64 &&
        polybench_trap_args_equal(packet->args,
          polybench_aarch64_trap_args)) ||
       (mode == POLY_MODE_RAW_RISCV &&
        polybench_trap_args_equal(packet->args,
          polybench_riscv_syscall_args))))
    return 4242;
  if (reason == POLY_TRAP_BREAK)
    return 0x4c000000ULL | (mode << 8) | number;
  if (reason == POLY_TRAP_IMPORT && number == 8 &&
      polybench_trap_args_equal(packet->args, polybench_aarch64_trap_args))
    return 5555;
  return (uint64_t) -38;
}

__attribute__((naked, noinline, used))
static void polybench_trap_vector_handler(void) {
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
    "subq $56, %rsp\n"
    "call polybench_trap_vector_dispatch\n"
    "addq $56, %rsp\n"
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

static void install_polybench_trap_vector(void) {
  memset(&polybench_monitor_packet, 0, sizeof(polybench_monitor_packet));
  poly_monitor_packet_set_value(
    (uint64_t) (uintptr_t) &polybench_monitor_packet);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value((uint64_t) (void *) polybench_trap_vector_handler);
}

__attribute__((noinline, used))
static uint64_t polybench_x86_strlen(const char *text) {
  uint64_t length = 0;
  while (length < 4096 && text[length] != '\0')
    length++;
  return length;
}

__attribute__((noinline, used))
static uint64_t polybench_x86_strnlen(const char *text, uint64_t limit) {
  uint64_t length = 0;
  if (limit > 4096)
    limit = 4096;
  while (length < limit && text[length] != '\0')
    length++;
  return length;
}

__attribute__((noinline, used))
static uint64_t polybench_x86_memcpy(uint8_t *dest, const uint8_t *src,
    uint64_t size) {
  for (uint64_t n = 0; n < size; n++)
    dest[n] = src[n];
  return (uint64_t) (uintptr_t) dest;
}

__attribute__((noinline, used))
static uint64_t polybench_x86_memset(uint8_t *dest, uint64_t value,
    uint64_t size) {
  for (uint64_t n = 0; n < size; n++)
    dest[n] = (uint8_t) value;
  return (uint64_t) (uintptr_t) dest;
}

__attribute__((noinline, used))
static uint64_t polybench_x86_memcmp(const uint8_t *left,
    const uint8_t *right, uint64_t size) {
  for (uint64_t n = 0; n < size; n++) {
    if (left[n] != right[n])
      return (uint64_t) ((int64_t) left[n] - (int64_t) right[n]);
  }
  return 0;
}

__attribute__((noinline, noipa, used))
static uint64_t polybench_x86_sum6_direct(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  return a0 + a1 + a2 + a3 + a4 + a5;
}

__attribute__((noinline, noipa, used))
static struct polybench_sret4 polybench_x86_sret4_direct(uint64_t a0,
    uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
  struct polybench_sret4 result = {
    .a = a0 + a1,
    .b = a2 + a3,
    .c = a4,
    .d = a0 + a4
  };
  return result;
}

__attribute__((noinline, noipa, used))
static double polybench_x86_fp64_sum6_direct(double a0, double a1,
    double a2, double a3, double a4, double a5) {
  return a0 + a1 + a2 + a3 + a4 + a5;
}

__attribute__((noinline, noipa, used))
static float polybench_x86_fp32_sum6_direct(float a0, float a1,
    float a2, float a3, float a4, float a5) {
  return a0 + a1 + a2 + a3 + a4 + a5;
}

__attribute__((noinline, noipa, used))
static polybench_vec128_u32 polybench_x86_vec128_u32_direct(
    polybench_vec128_u32 a0, polybench_vec128_u32 a1) {
  return a0 + a1;
}

static uint32_t riscv_ld(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (0x3U << 12) | (rd << 7) | 0x03U;
}

static uint32_t riscv_sd(uint32_t rs2, uint32_t rs1, int32_t imm) {
  const uint32_t imm12 = (uint32_t) imm & 0xfffU;
  return ((imm12 >> 5) << 25) | (rs2 << 20) | (rs1 << 15) |
    (0x3U << 12) | ((imm12 & 0x1fU) << 7) | 0x23U;
}

static uint32_t riscv_add(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (rs2 << 20) | (rs1 << 15) | (rd << 7) | 0x33U;
}

static uint32_t riscv_addi(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (rd << 7) | 0x13U;
}

static void emit_aarch64_pcall_sig(uint8_t *code, size_t *offset,
    uint32_t signature_slot) {
  emit_u32(code, offset, POLYBENCH_AARCH64_PCALL_SIG_IMM(signature_slot));
}

static void emit_aarch64_direct_x86_pcall_sig(uint8_t *code, size_t *offset,
    uint64_t target, uint32_t signature_slot) {
  emit_aarch64_movabs(code, offset, 16, target);
  emit_u32(code, offset, 0xd2800011U); // movz x17,#0 (x86 frontend)
  const uint64_t return_pc = (uint64_t) (uintptr_t)
    (code + *offset + 16 + 4);
  emit_aarch64_movabs(code, offset, 18, return_pc);
  emit_aarch64_pcall_sig(code, offset, signature_slot);
}

static void emit_aarch64_direct_x86_pcall(uint8_t *code, size_t *offset,
    uint64_t target) {
  emit_aarch64_direct_x86_pcall_sig(code, offset, target,
    polybench_native_signature_slot);
}

static void emit_riscv_pcall_sig(uint8_t *code, size_t *offset,
    uint32_t signature_slot) {
  emit_u32(code, offset, POLYBENCH_RISCV_PCALL_SIG_IMM(signature_slot));
}

static void emit_riscv_direct_x86_pcall_sig(uint8_t *code, size_t *offset,
    uint32_t signature_slot) {
  emit_u32(code, offset, riscv_addi(6, 0, 0)); // t1 = x86 frontend
  const size_t auipc_return_pc = *offset;
  emit_u32(code, offset, 0x00000397U); // auipc t2,0
  const size_t addi_return_offset = *offset;
  emit_u32(code, offset, 0);
  emit_riscv_pcall_sig(code, offset, signature_slot);
  store_u32(code, addi_return_offset, riscv_addi(7, 7,
    (int32_t) *offset - (int32_t) auipc_return_pc));
}

static void emit_riscv_direct_x86_pcall(uint8_t *code, size_t *offset) {
  emit_riscv_direct_x86_pcall_sig(code, offset,
    polybench_native_signature_slot);
}

static void emit_x86_pcall_sig_imm_mode(uint8_t *code, size_t *offset,
    uint32_t frontend, uint32_t signature_slot) {
  code[(*offset)++] = 0x53; // push rbx
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0x57; // push r15
  emit_x86_movabs_r15(code, offset, frontend);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc, (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(signature_slot)
  };
  emit_bytes(code, offset, pcall, sizeof(pcall));
  const size_t target_offset = *offset;

  if (frontend == POLY_FRONTEND_AARCH64) {
    emit_u32(code, offset, 0x8b010000U); // add x0,x0,x1
    emit_u32(code, offset, 0x8b020000U); // add x0,x0,x2
    emit_u32(code, offset, 0x8b030000U); // add x0,x0,x3
    emit_u32(code, offset, 0x8b040000U); // add x0,x0,x4
    emit_u32(code, offset, 0x8b050000U); // add x0,x0,x5
    emit_u32(code, offset, 0xd65f03c0U); // ret x30
  } else {
    emit_u32(code, offset, 0x00b50533U); // add a0,a0,a1
    emit_u32(code, offset, 0x00c50533U); // add a0,a0,a2
    emit_u32(code, offset, 0x00d50533U); // add a0,a0,a3
    emit_u32(code, offset, 0x00e50533U); // add a0,a0,a4
    emit_u32(code, offset, 0x00f50533U); // add a0,a0,a5
    emit_u32(code, offset, 0x00008067U); // ret
  }

  const size_t return_offset = *offset;
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0x5f; // pop r15
  code[(*offset)++] = 0x5b; // pop rbx
  code[(*offset)++] = 0xc3;
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + target_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + return_offset));
}

static void emit_x86_pcall_sret_sig_imm_mode(uint8_t *code, size_t *offset,
    uint32_t frontend, uint32_t signature_slot) {
  code[(*offset)++] = 0x53; // push rbx
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0x57; // push r15
  emit_x86_movabs_r15(code, offset, frontend);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc, (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(signature_slot)
  };
  emit_bytes(code, offset, pcall, sizeof(pcall));
  const size_t target_offset = *offset;

  if (frontend == POLY_FRONTEND_AARCH64) {
    emit_u32(code, offset, 0x8b010009U); // add x9,x0,x1
    emit_u32(code, offset, aarch64_str_x_imm(9, 8, 0));
    emit_u32(code, offset, 0x8b030049U); // add x9,x2,x3
    emit_u32(code, offset, aarch64_str_x_imm(9, 8, 8));
    emit_u32(code, offset, aarch64_mov_reg(9, 4));
    emit_u32(code, offset, aarch64_str_x_imm(9, 8, 16));
    emit_u32(code, offset, 0x8b040009U); // add x9,x0,x4
    emit_u32(code, offset, aarch64_str_x_imm(9, 8, 24));
    emit_u32(code, offset, 0xd65f03c0U); // ret x30
  } else {
    emit_u32(code, offset, riscv_add(5, 11, 12));
    emit_u32(code, offset, riscv_sd(5, 10, 0));
    emit_u32(code, offset, riscv_add(5, 13, 14));
    emit_u32(code, offset, riscv_sd(5, 10, 8));
    emit_u32(code, offset, riscv_addi(5, 15, 0));
    emit_u32(code, offset, riscv_sd(5, 10, 16));
    emit_u32(code, offset, riscv_add(5, 11, 15));
    emit_u32(code, offset, riscv_sd(5, 10, 24));
    emit_u32(code, offset, 0x00008067U); // ret
  }

  const size_t return_offset = *offset;
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0x5f; // pop r15
  code[(*offset)++] = 0x5b; // pop rbx
  code[(*offset)++] = 0xc3;
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + target_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + return_offset));
}

static void emit_x86_pcall_sig_imm_mode_fp32(uint8_t *code, size_t *offset,
    uint32_t frontend, uint32_t signature_slot) {
  code[(*offset)++] = 0x53; // push rbx
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0x57; // push r15
  emit_x86_movabs_r15(code, offset, frontend);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc, (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(signature_slot)
  };
  emit_bytes(code, offset, pcall, sizeof(pcall));
  const size_t target_offset = *offset;

  if (frontend == POLY_FRONTEND_AARCH64) {
    emit_u32(code, offset, aarch64_fadd_s(0, 0, 1));
    emit_u32(code, offset, aarch64_fadd_s(0, 0, 2));
    emit_u32(code, offset, aarch64_fadd_s(0, 0, 3));
    emit_u32(code, offset, aarch64_fadd_s(0, 0, 4));
    emit_u32(code, offset, aarch64_fadd_s(0, 0, 5));
    emit_u32(code, offset, 0xd65f03c0U); // ret x30
  } else {
    emit_u32(code, offset, riscv_fadd_s(10, 10, 11));
    emit_u32(code, offset, riscv_fadd_s(10, 10, 12));
    emit_u32(code, offset, riscv_fadd_s(10, 10, 13));
    emit_u32(code, offset, riscv_fadd_s(10, 10, 14));
    emit_u32(code, offset, riscv_fadd_s(10, 10, 15));
    emit_u32(code, offset, 0x00008067U); // ret
  }

  const size_t return_offset = *offset;
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0x5f; // pop r15
  code[(*offset)++] = 0x5b; // pop rbx
  code[(*offset)++] = 0xc3;
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + target_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + return_offset));
}

static void emit_x86_pcall_sig_imm_mode_fp64(uint8_t *code, size_t *offset,
    uint32_t frontend, uint32_t signature_slot) {
  code[(*offset)++] = 0x53; // push rbx
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0x57; // push r15
  emit_x86_movabs_r15(code, offset, frontend);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc, (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(signature_slot)
  };
  emit_bytes(code, offset, pcall, sizeof(pcall));
  const size_t target_offset = *offset;

  if (frontend == POLY_FRONTEND_AARCH64) {
    emit_u32(code, offset, aarch64_fadd_d(0, 0, 1));
    emit_u32(code, offset, aarch64_fadd_d(0, 0, 2));
    emit_u32(code, offset, aarch64_fadd_d(0, 0, 3));
    emit_u32(code, offset, aarch64_fadd_d(0, 0, 4));
    emit_u32(code, offset, aarch64_fadd_d(0, 0, 5));
    emit_u32(code, offset, 0xd65f03c0U); // ret x30
  } else {
    emit_u32(code, offset, riscv_fadd_d(10, 10, 11));
    emit_u32(code, offset, riscv_fadd_d(10, 10, 12));
    emit_u32(code, offset, riscv_fadd_d(10, 10, 13));
    emit_u32(code, offset, riscv_fadd_d(10, 10, 14));
    emit_u32(code, offset, riscv_fadd_d(10, 10, 15));
    emit_u32(code, offset, 0x00008067U); // ret
  }

  const size_t return_offset = *offset;
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0x5f; // pop r15
  code[(*offset)++] = 0x5b; // pop rbx
  code[(*offset)++] = 0xc3;
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + target_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + return_offset));
}

static uint32_t riscv_fadd_s(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return ((rs2 & 0x1fU) << 20) | ((rs1 & 0x1fU) << 15) |
    (0x7U << 12) | ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fsub_s(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (0x04U << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | (0x7U << 12) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fmul_s(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (0x08U << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | (0x7U << 12) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_addw(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return ((rs2 & 0x1fU) << 20) | ((rs1 & 0x1fU) << 15) |
    ((rd & 0x1fU) << 7) | 0x3bU;
}

static uint32_t riscv_or(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return ((rs2 & 0x1fU) << 20) | ((rs1 & 0x1fU) << 15) |
    (0x6U << 12) | ((rd & 0x1fU) << 7) | 0x33U;
}

static uint32_t riscv_slli(uint32_t rd, uint32_t rs1, uint32_t shamt) {
  return ((shamt & 0x3fU) << 20) | ((rs1 & 0x1fU) << 15) |
    (0x1U << 12) | ((rd & 0x1fU) << 7) | 0x13U;
}

static uint32_t riscv_srli(uint32_t rd, uint32_t rs1, uint32_t shamt) {
  return ((shamt & 0x3fU) << 20) | ((rs1 & 0x1fU) << 15) |
    (0x5U << 12) | ((rd & 0x1fU) << 7) | 0x13U;
}

static uint32_t riscv_fadd_d(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (0x01U << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | (0x7U << 12) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fsub_d(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (0x05U << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | (0x7U << 12) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fmul_d(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (0x09U << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | (0x7U << 12) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fsgnj_d(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (0x11U << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fmv_d_x(uint32_t rd, uint32_t rs1) {
  return (0x79U << 25) | ((rs1 & 0x1fU) << 15) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fmv_x_w(uint32_t rd, uint32_t rs1) {
  return (0x70U << 25) | ((rs1 & 0x1fU) << 15) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fmv_w_x(uint32_t rd, uint32_t rs1) {
  return (0x78U << 25) | ((rs1 & 0x1fU) << 15) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint64_t call_code_no_args(const uint8_t *code) {
  uint64_t rax;
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%rbp\n"
    "pushq %%r12\n"
    "pushq %%r13\n"
    "pushq %%r14\n"
    "pushq %%r15\n"
    "call *%1\n"
    "popq %%r15\n"
    "popq %%r14\n"
    "popq %%r13\n"
    "popq %%r12\n"
    "popq %%rbp\n"
    "popq %%rbx"
    : "=a"(rax)
    : "r"(code)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static uint64_t call_code_with_rax_arg(const uint8_t *code, const char *payload) {
  register uint64_t rax asm("rax") = (uint64_t) (uintptr_t) payload;
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%rbp\n"
    "pushq %%r12\n"
    "pushq %%r13\n"
    "pushq %%r14\n"
    "pushq %%r15\n"
    "call *%1\n"
    "popq %%r15\n"
    "popq %%r14\n"
    "popq %%r13\n"
    "popq %%r12\n"
    "popq %%rbp\n"
    "popq %%rbx"
    : "+a"(rax)
    : "r"(code)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static uint64_t call_code_with_poly3_args(const uint8_t *code,
    const void *arg0, const void *arg1, uint64_t arg2) {
  uint64_t rax = (uint64_t) (uintptr_t) arg0;
  uint64_t rdx = (uint64_t) (uintptr_t) arg1;
  uint64_t rcx = arg2;
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%rbp\n"
    "pushq %%r12\n"
    "pushq %%r13\n"
    "pushq %%r14\n"
    "pushq %%r15\n"
    "call *%3\n"
    "popq %%r15\n"
    "popq %%r14\n"
    "popq %%r13\n"
    "popq %%r12\n"
    "popq %%rbp\n"
    "popq %%rbx"
    : "+a"(rax), "+d"(rdx), "+c"(rcx)
    : "r"(code)
    : "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static uint64_t call_code_vec128_u32(const uint8_t *code) {
  union polybench_vec128_u32_bits arg0 = { .u = { 1, 2, 3, 4 } };
  union polybench_vec128_u32_bits arg1 = { .u = { 10, 20, 30, 40 } };
  union polybench_vec128_u32_bits result;
  polybench_vec128_u32 (*entry)(polybench_vec128_u32,
    polybench_vec128_u32) =
    (polybench_vec128_u32 (*)(polybench_vec128_u32,
      polybench_vec128_u32)) code;

  POLYBENCH_SAVE_R15();
  result.v = entry(arg0.v, arg1.v);
  POLYBENCH_RESTORE_R15();
  return ((uint64_t) (result.u[3] & 0xffffU) << 48) |
    ((uint64_t) (result.u[2] & 0xffffU) << 32) |
    ((uint64_t) (result.u[1] & 0xffffU) << 16) |
    (uint64_t) (result.u[0] & 0xffffU);
}

static uint64_t call_code_fp64_6(const uint8_t *code) {
  double (*entry)(double, double, double, double, double, double) =
    (double (*)(double, double, double, double, double, double)) code;
  POLYBENCH_SAVE_R15();
  double result = entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
  POLYBENCH_RESTORE_R15();
  return fp64_to_bits(result);
}

static uint32_t call_code_fp32_6(const uint8_t *code) {
  float (*entry)(float, float, float, float, float, float) =
    (float (*)(float, float, float, float, float, float)) code;
  POLYBENCH_SAVE_R15();
  float result = entry(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
  POLYBENCH_RESTORE_R15();
  return fp32_to_bits(result);
}

static uint64_t call_code_u64_6(const uint8_t *code) {
  uint64_t (*entry)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
    uint64_t) =
    (uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
      uint64_t)) code;
  POLYBENCH_SAVE_R15();
  uint64_t result = entry(1, 2, 3, 4, 5, 6);
  POLYBENCH_RESTORE_R15();
  return result;
}

static uint64_t call_code_sret5(const uint8_t *code) {
  struct polybench_sret4 (*entry)(uint64_t, uint64_t, uint64_t, uint64_t,
    uint64_t) =
    (struct polybench_sret4 (*)(uint64_t, uint64_t, uint64_t, uint64_t,
      uint64_t)) code;
  POLYBENCH_SAVE_R15();
  struct polybench_sret4 result = entry(1, 2, 3, 4, 5);
  POLYBENCH_RESTORE_R15();
  return ((result.a & 0xffffULL) << 48) |
    ((result.b & 0xffffULL) << 32) |
    ((result.c & 0xffffULL) << 16) |
    (result.d & 0xffffULL);
}

static int run_loop_program(int arch, uint64_t *result, uint64_t *insn_delta,
    uint64_t *switch_delta) {
  const size_t code_size = 4 + 8 + 4 * 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: mmap failed: %s\n", strerror(errno));
    return -1;
  }

  code[0] = 0x90;
  code[1] = 0x90;
  code[2] = 0x90;
  code[3] = 0x90;

  size_t offset = arch == POLY_ARCH_RISCV_COMPRESSED ? 4 : 3;
  if (arch == POLY_ARCH_AARCH64) {
    const uint8_t raw_switch[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, 0xd2800000U | ((uint32_t) LOOP_ITERS << 5)); // movz x0,#LOOP_ITERS
    emit_u32(code, &offset, 0xd1000400U); // sub x0,x0,#1
    emit_u32(code, &offset, 0xb5ffffe0U); // cbnz x0, previous instruction
    emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  } else if (arch == POLY_ARCH_RISCV) {
    const uint8_t raw_switch[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, ((uint32_t) LOOP_ITERS << 20) | 0x00000513U); // addi a0,zero,LOOP_ITERS
    emit_u32(code, &offset, 0xfff50513U); // addi a0,a0,-1
    emit_u32(code, &offset, 0xfe051ee3U); // bne a0,zero, previous instruction
    emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  } else {
    const uint8_t raw_switch[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, ((uint32_t) LOOP_ITERS << 20) | 0x00000513U); // addi a0,zero,LOOP_ITERS
    emit_u16(code, &offset, 0x157dU); // c.addi a0,-1
    emit_u16(code, &offset, 0xfd7dU); // c.bnez a0, previous instruction
    emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  }
  code[offset++] = 0xc3;

  uint64_t before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  uint64_t after = poly_foreign_insn_count_status_value();
  *insn_delta = after - before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_pcall_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64 direct x86 pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, 0xd2800020U); // movz x0,#1
  emit_u32(code, &offset, 0xd2800041U); // movz x1,#2
  emit_u32(code, &offset, 0xd2800062U); // movz x2,#3
  emit_u32(code, &offset, 0xd2800083U); // movz x3,#4
  emit_u32(code, &offset, 0xd28000a4U); // movz x4,#5
  emit_u32(code, &offset, 0xd28000c5U); // movz x5,#6
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_sum6_direct);
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_pcall_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv direct x86 pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u32(code, &offset, 0x00100513U); // addi a0,zero,1
  emit_u32(code, &offset, 0x00200593U); // addi a1,zero,2
  emit_u32(code, &offset, 0x00300613U); // addi a2,zero,3
  emit_u32(code, &offset, 0x00400693U); // addi a3,zero,4
  emit_u32(code, &offset, 0x00500713U); // addi a4,zero,5
  emit_u32(code, &offset, 0x00600793U); // addi a5,zero,6
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_sum6_direct);
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_sret_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64 direct x86 SRET mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, 0xd10083ffU); // sub sp,sp,#32
  emit_u32(code, &offset, 0x910003e8U); // mov x8,sp (sret pointer)
  emit_u32(code, &offset, 0xd2800020U); // movz x0,#1
  emit_u32(code, &offset, 0xd2800041U); // movz x1,#2
  emit_u32(code, &offset, 0xd2800062U); // movz x2,#3
  emit_u32(code, &offset, 0xd2800083U); // movz x3,#4
  emit_u32(code, &offset, 0xd28000a4U); // movz x4,#5
  emit_aarch64_direct_x86_pcall_sig(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_sret4_direct,
    polybench_sret_signature_slot);
  emit_u32(code, &offset, aarch64_ldr_x_sp(0, 0));
  emit_u32(code, &offset, aarch64_ldr_x_sp(1, 8));
  emit_u32(code, &offset, aarch64_ldr_x_sp(2, 16));
  emit_u32(code, &offset, aarch64_ldr_x_sp(3, 24));
  emit_u32(code, &offset, aarch64_lsl_imm(0, 0, 48));
  emit_u32(code, &offset, aarch64_lsl_imm(1, 1, 32));
  emit_u32(code, &offset, aarch64_orr_reg(0, 0, 1));
  emit_u32(code, &offset, aarch64_lsl_imm(2, 2, 16));
  emit_u32(code, &offset, aarch64_orr_reg(0, 0, 2));
  emit_u32(code, &offset, aarch64_orr_reg(0, 0, 3));
  emit_u32(code, &offset, 0x910083ffU); // add sp,sp,#32
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_sret_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv direct x86 SRET mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u32(code, &offset, riscv_addi(2, 2, -32));
  emit_u32(code, &offset, riscv_addi(10, 2, 0)); // a0 = sret pointer
  emit_u32(code, &offset, riscv_addi(11, 0, 1));
  emit_u32(code, &offset, riscv_addi(12, 0, 2));
  emit_u32(code, &offset, riscv_addi(13, 0, 3));
  emit_u32(code, &offset, riscv_addi(14, 0, 4));
  emit_u32(code, &offset, riscv_addi(15, 0, 5));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall_sig(code, &offset,
    polybench_sret_signature_slot);
  emit_u32(code, &offset, riscv_ld(10, 2, 0));
  emit_u32(code, &offset, riscv_ld(11, 2, 8));
  emit_u32(code, &offset, riscv_ld(12, 2, 16));
  emit_u32(code, &offset, riscv_ld(13, 2, 24));
  emit_u32(code, &offset, riscv_slli(10, 10, 48));
  emit_u32(code, &offset, riscv_slli(11, 11, 32));
  emit_u32(code, &offset, riscv_or(10, 10, 11));
  emit_u32(code, &offset, riscv_slli(12, 12, 16));
  emit_u32(code, &offset, riscv_or(10, 10, 12));
  emit_u32(code, &offset, riscv_or(10, 10, 13));
  emit_u32(code, &offset, riscv_addi(2, 2, 32));
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_sret4_direct);
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_fp64_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64 direct x86 FP mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 1));
  emit_aarch64_direct_x86_pcall_sig(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_fp64_sum6_direct,
    polybench_fp64_signature_slot);
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 5));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result_bits = call_code_fp64_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_fp64_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv direct x86 FP mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 11));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall_sig(code, &offset,
    polybench_fp64_signature_slot);
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 15));
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_fp64_sum6_direct);
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result_bits = call_code_fp64_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_fp32_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64 direct x86 FP32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, aarch64_fadd_s(0, 0, 1));
  emit_aarch64_direct_x86_pcall_sig(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_fp32_sum6_direct,
    polybench_fp32_signature_slot);
  emit_u32(code, &offset, aarch64_fadd_s(0, 0, 5));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result_bits = call_code_fp32_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_fp32_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv direct x86 FP32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u32(code, &offset, riscv_fadd_s(10, 10, 11));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall_sig(code, &offset,
    polybench_fp32_signature_slot);
  emit_u32(code, &offset, riscv_fadd_s(10, 10, 15));
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_fp32_sum6_direct);
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result_bits = call_code_fp32_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_signature_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 128;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-aarch64 signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  emit_x86_pcall_sig_imm_mode(code, &offset, POLY_FRONTEND_AARCH64,
    polybench_native_signature_slot);

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_u64_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_signature_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 128;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-riscv signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  emit_x86_pcall_sig_imm_mode(code, &offset, POLY_FRONTEND_RISCV,
    polybench_native_signature_slot);

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_u64_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_sret_signature_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 160;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-aarch64 SRET signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  emit_x86_pcall_sret_sig_imm_mode(code, &offset, POLY_FRONTEND_AARCH64,
    polybench_sret_signature_slot);

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_sret5(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_sret_signature_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 160;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-riscv SRET signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  emit_x86_pcall_sret_sig_imm_mode(code, &offset, POLY_FRONTEND_RISCV,
    polybench_sret_signature_slot);

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_sret5(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_fp32_signature_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 128;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-aarch64 FP32 signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  emit_x86_pcall_sig_imm_mode_fp32(code, &offset, POLY_FRONTEND_AARCH64,
    polybench_fp32_signature_slot);

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result_bits = call_code_fp32_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_fp32_signature_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 128;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-riscv FP32 signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  emit_x86_pcall_sig_imm_mode_fp32(code, &offset, POLY_FRONTEND_RISCV,
    polybench_fp32_signature_slot);

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result_bits = call_code_fp32_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_fp64_signature_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 128;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-aarch64 FP64 signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  emit_x86_pcall_sig_imm_mode_fp64(code, &offset, POLY_FRONTEND_AARCH64,
    polybench_fp64_signature_slot);

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result_bits = call_code_fp64_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_fp64_signature_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 128;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-riscv FP64 signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  emit_x86_pcall_sig_imm_mode_fp64(code, &offset, POLY_FRONTEND_RISCV,
    polybench_fp64_signature_slot);

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result_bits = call_code_fp64_6(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_mixed_program(uint64_t *result, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 3 + 8 + 5 * 4 + 2 * 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: mixed mmap failed: %s\n", strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, 0xd2800140U); // movz x0,#10
  emit_u32(code, &offset, 0x91001400U); // add x0,x0,#5
  emit_u32(code, &offset, 0x10000070U); // adr x16,riscv target
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_u32(code, &offset, 0xd5032f1fU); // generic poly switch

  emit_u32(code, &offset, 0x01b50513U); // addi a0,a0,27
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_compressed_mixed_program(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 2 + 8 + 5 * 4 + 2 + 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: compressed mixed mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, 0xd2800140U); // movz x0,#10
  emit_u32(code, &offset, 0x91001400U); // add x0,x0,#5
  emit_u32(code, &offset, 0x10000070U); // adr x16,riscv target
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_u32(code, &offset, 0xd5032f1fU); // generic poly switch

  emit_u16(code, &offset, 0x056dU); // c.addi a0,27
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_compressed_reverse_mixed_program(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 2 + 8 + 2 + 4 * 4 + 2 * 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: compressed reverse mixed mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u16(code, &offset, 0x451dU); // c.li a0,7
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t addi_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  emit_u32(code, &offset, 0x1000700bU); // generic poly switch
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91008c00U); // add x0,x0,#35
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;
  store_u32(code, addi_target_offset, riscv_addi(5, 5,
    (int32_t) aarch64_target_offset - (int32_t) auipc_target_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_reverse_mixed_program(uint64_t *result, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 3 + 8 + 5 * 4 + 2 * 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: reverse mixed mmap failed: %s\n", strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u32(code, &offset, 0x00700513U); // addi a0,zero,7
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t addi_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  emit_u32(code, &offset, 0x1000700bU); // generic poly switch
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91008c00U); // add x0,x0,#35
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;
  store_u32(code, addi_target_offset, riscv_addi(5, 5,
    (int32_t) aarch64_target_offset - (int32_t) auipc_target_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, 0x91000400U); // add x0,x0,#1
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01550513U); // addi a0,a0,21
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00150513U); // addi a0,a0,1
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91005400U); // add x0,x0,#21
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_neutral_pcall_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral aarch64-to-riscv pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_AARCH64);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(polybench_native_signature_slot)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t x86_return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_outer_offset = offset;
  const size_t aarch64_after_offset =
    aarch64_outer_offset + 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_after_offset + 4;
  emit_u32(code, &offset, 0xd28004a0U); // movz x0,#37
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_after_offset));
  emit_aarch64_pcall_sig(code, &offset, polybench_native_signature_slot);
  emit_u32(code, &offset, 0xd65f03c0U); // ret x30 to x86 wrapper

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00550513U); // addi a0,a0,5
  emit_u32(code, &offset, 0x00008067U); // ret

  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + aarch64_outer_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + x86_return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_neutral_pcall_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral riscv-to-aarch64 pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_RISCV);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(polybench_native_signature_slot)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t x86_return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t riscv_outer_offset = offset;
  emit_u32(code, &offset, 0x02500513U); // addi a0,zero,37
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_pcall_sig(code, &offset, polybench_native_signature_slot);
  const size_t riscv_after_offset = offset;
  emit_u32(code, &offset, 0x00008067U); // ret to x86 wrapper

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91001400U); // add x0,x0,#5
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_after_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + riscv_outer_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + x86_return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_neutral_pcall_fp64_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral aarch64-to-riscv FP64 pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_AARCH64);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(polybench_fp64_signature_slot)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t x86_return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_outer_offset = offset;
  const size_t aarch64_after_offset =
    aarch64_outer_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_after_offset + 4;
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_after_offset));
  emit_aarch64_pcall_sig(code, &offset, polybench_fp64_signature_slot);
  emit_u32(code, &offset, 0xd65f03c0U); // ret x30 to x86 wrapper

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 11));
  emit_u32(code, &offset, riscv_fsub_d(10, 10, 11));
  emit_u32(code, &offset, riscv_fmul_d(10, 10, 11));
  emit_u32(code, &offset, 0x00008067U); // ret

  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + aarch64_outer_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + x86_return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double, double) = (double (*)(double, double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(1.5, 2.25)));
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_neutral_pcall_fp64_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral riscv-to-aarch64 FP64 pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_RISCV);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(polybench_fp64_signature_slot)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t x86_return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t riscv_outer_offset = offset;
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_pcall_sig(code, &offset, polybench_fp64_signature_slot);
  const size_t riscv_after_offset = offset;
  emit_u32(code, &offset, 0x00008067U); // ret to x86 wrapper

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 1));
  emit_u32(code, &offset, aarch64_fsub_d(0, 0, 1));
  emit_u32(code, &offset, aarch64_fmul_d(0, 0, 1));
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_after_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + riscv_outer_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + x86_return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double, double) = (double (*)(double, double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(1.5, 2.25)));
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_neutral_pcall_fp32_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral aarch64-to-riscv FP32 pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_AARCH64);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(polybench_fp32_signature_slot)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t x86_return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_outer_offset = offset;
  const size_t aarch64_after_offset =
    aarch64_outer_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_after_offset + 4;
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_after_offset));
  emit_aarch64_pcall_sig(code, &offset, polybench_fp32_signature_slot);
  emit_u32(code, &offset, 0xd65f03c0U); // ret x30 to x86 wrapper

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, riscv_fadd_s(10, 10, 11));
  emit_u32(code, &offset, riscv_fsub_s(10, 10, 11));
  emit_u32(code, &offset, riscv_fmul_s(10, 10, 11));
  emit_u32(code, &offset, 0x00008067U); // ret

  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + aarch64_outer_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + x86_return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  float (*entry)(float, float) = (float (*)(float, float)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp32_to_bits(entry(1.5f, 2.25f)));
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_neutral_pcall_fp32_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral riscv-to-aarch64 FP32 pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_RISCV);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(polybench_fp32_signature_slot)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t x86_return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t riscv_outer_offset = offset;
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_pcall_sig(code, &offset, polybench_fp32_signature_slot);
  const size_t riscv_after_offset = offset;
  emit_u32(code, &offset, 0x00008067U); // ret to x86 wrapper

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, aarch64_fadd_s(0, 0, 1));
  emit_u32(code, &offset, aarch64_fsub_s(0, 0, 1));
  emit_u32(code, &offset, aarch64_fmul_s(0, 0, 1));
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_after_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + riscv_outer_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + x86_return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  float (*entry)(float, float) = (float (*)(float, float)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp32_to_bits(entry(1.5f, 2.25f)));
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv FP call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double) = (double (*)(double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(2.0)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 FP call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double) = (double (*)(double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(2.0)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp8_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv FP8 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  for (uint32_t reg = 11; reg <= 17; reg++)
    emit_u32(code, &offset, riscv_fadd_d(10, 10, reg));
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double, double, double, double, double, double, double,
    double) = (double (*)(double, double, double, double, double, double,
      double, double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits,
    fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp8_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 FP8 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  for (uint32_t reg = 1; reg <= 7; reg++)
    emit_u32(code, &offset, aarch64_fadd_d(0, 0, reg));
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double, double, double, double, double, double, double,
    double) = (double (*)(double, double, double, double, double, double,
      double, double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits,
    fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp64_signature_aarch64_to_riscv(
    uint64_t *result_bits, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: aarch64-to-riscv FP64 signature call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_aarch64_pcall_sig(code, &offset, polybench_fp64_signature_slot);
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 11));
  emit_u32(code, &offset, riscv_fsub_d(10, 10, 11));
  emit_u32(code, &offset, riscv_fmul_d(10, 10, 11));
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double, double) = (double (*)(double, double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(1.5, 2.25)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp64_signature_riscv_to_aarch64(
    uint64_t *result_bits, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: riscv-to-aarch64 FP64 signature call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_pcall_sig(code, &offset, polybench_fp64_signature_slot);
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 1));
  emit_u32(code, &offset, aarch64_fsub_d(0, 0, 1));
  emit_u32(code, &offset, aarch64_fmul_d(0, 0, 1));
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double, double) = (double (*)(double, double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(1.5, 2.25)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp32_signature_aarch64_to_riscv(
    uint64_t *result_bits, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: aarch64-to-riscv FP32 signature call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_aarch64_pcall_sig(code, &offset, polybench_fp32_signature_slot);
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, riscv_fadd_s(10, 10, 11));
  emit_u32(code, &offset, riscv_fsub_s(10, 10, 11));
  emit_u32(code, &offset, riscv_fmul_s(10, 10, 11));
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  float (*entry)(float, float) = (float (*)(float, float)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp32_to_bits(entry(1.5f, 2.25f)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp32_signature_riscv_to_aarch64(
    uint64_t *result_bits, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr,
      "POLYBENCH_FAIL: riscv-to-aarch64 FP32 signature call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_pcall_sig(code, &offset, polybench_fp32_signature_slot);
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, aarch64_fadd_s(0, 0, 1));
  emit_u32(code, &offset, aarch64_fsub_s(0, 0, 1));
  emit_u32(code, &offset, aarch64_fmul_s(0, 0, 1));
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  float (*entry)(float, float) = (float (*)(float, float)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp32_to_bits(entry(1.5f, 2.25f)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp64_stack_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv FP64 stack call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 4 + 8 * 8 + 8 * 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 4 + 1;

  emit_u32(code, &offset, 0xd10103ffU); // sub sp,sp,#64
  for (uint32_t n = 0; n < 8; n++) {
    emit_u32(code, &offset, aarch64_ldr_x_sp(8, 72 + n * 8));
    emit_u32(code, &offset, aarch64_str_x_sp(8, n * 8));
  }
  for (uint32_t n = 0; n < 8; n++)
    emit_u32(code, &offset, aarch64_ldr_x_sp(n, n * 8));
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_aarch64_pcall_sig(code, &offset, polybench_fp64_signature_slot);
  emit_u32(code, &offset, 0x910103ffU); // add sp,sp,#64
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  for (uint32_t reg = 11; reg <= 17; reg++)
    emit_u32(code, &offset, riscv_fadd_d(10, 10, reg));
  for (uint32_t n = 0; n < 8; n++) {
    emit_u32(code, &offset, riscv_fmv_d_x(15, 10 + n));
    emit_u32(code, &offset, riscv_fadd_d(10, 10, 15));
  }
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double, double, double, double, double, double, double,
    double, double, double, double, double, double, double, double, double) =
    (double (*)(double, double, double, double, double, double, double,
      double, double, double, double, double, double, double, double,
      double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits,
    fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
      9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp64_stack_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 FP64 stack call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  for (uint32_t n = 0; n < 8; n++)
    emit_u32(code, &offset, riscv_ld(10 + n, 2, 8 + n * 8));
  emit_u32(code, &offset, riscv_addi(2, 2, -64));
  for (uint32_t n = 0; n < 8; n++)
    emit_u32(code, &offset, riscv_sd(10 + n, 2, n * 8));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_pcall_sig(code, &offset, polybench_fp64_signature_slot);
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, riscv_addi(2, 2, 64));
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  for (uint32_t reg = 1; reg <= 7; reg++)
    emit_u32(code, &offset, aarch64_fadd_d(0, 0, reg));
  for (uint32_t n = 0; n < 8; n++) {
    emit_u32(code, &offset, aarch64_ldr_d_sp(1, n * 8));
    emit_u32(code, &offset, aarch64_fadd_d(0, 0, 1));
  }
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double, double, double, double, double, double, double,
    double, double, double, double, double, double, double, double, double) =
    (double (*)(double, double, double, double, double, double, double,
      double, double, double, double, double, double, double, double,
      double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits,
    fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
      9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static void emit_riscv_vec128_u32_pair_add(uint8_t *code, size_t *offset,
    uint32_t rd_lo, uint32_t rd_hi, uint32_t left_lo, uint32_t left_hi,
    uint32_t right_lo, uint32_t right_hi) {
  emit_u32(code, offset, riscv_addw(5, left_lo, right_lo));
  emit_u32(code, offset, riscv_srli(6, left_lo, 32));
  emit_u32(code, offset, riscv_srli(7, right_lo, 32));
  emit_u32(code, offset, riscv_addw(6, 6, 7));
  emit_u32(code, offset, riscv_slli(6, 6, 32));
  emit_u32(code, offset, riscv_or(rd_lo, 5, 6));

  emit_u32(code, offset, riscv_addw(5, left_hi, right_hi));
  emit_u32(code, offset, riscv_srli(6, left_hi, 32));
  emit_u32(code, offset, riscv_srli(7, right_hi, 32));
  emit_u32(code, offset, riscv_addw(6, 6, 7));
  emit_u32(code, offset, riscv_slli(6, 6, 32));
  emit_u32(code, offset, riscv_or(rd_hi, 5, 6));
}

static int run_direct_x86_vec128_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64 direct x86 vec128 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, 0x4ea18400U); // add v0.4s,v0.4s,v1.4s
  emit_aarch64_direct_x86_pcall_sig(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_vec128_u32_direct,
    POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32);
  emit_u32(code, &offset, 0x4ea18400U); // add v0.4s,v0.4s,v1.4s
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_vec128_u32(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_vec128_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv direct x86 vec128 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  const size_t auipc_data_pc = offset;
  emit_u32(code, &offset, 0x00000e17U); // auipc x28,0
  const size_t ld_arg0_lo_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t ld_arg0_hi_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t ld_arg1_lo_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t ld_arg1_hi_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_vec128_u32_pair_add(code, &offset, 10, 11, 10, 11, 12, 13);
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall_sig(code, &offset,
    POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32);
  emit_riscv_vec128_u32_pair_add(code, &offset, 10, 11, 10, 11, 12, 13);
  emit_u32(code, &offset, riscv_addi(14, 10, 0)); // keep lane0/lane1 pair
  emit_u32(code, &offset, riscv_slli(10, 10, 48));
  emit_u32(code, &offset, riscv_srli(10, 10, 48));
  emit_u32(code, &offset, riscv_srli(5, 14, 32));
  emit_u32(code, &offset, riscv_slli(5, 5, 16));
  emit_u32(code, &offset, riscv_or(10, 10, 5));
  emit_u32(code, &offset, riscv_slli(6, 11, 32));
  emit_u32(code, &offset, riscv_or(10, 10, 6));
  emit_u32(code, &offset, riscv_srli(7, 11, 32));
  emit_u32(code, &offset, riscv_slli(7, 7, 48));
  emit_u32(code, &offset, riscv_or(10, 10, 7));
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t arg0_lo_data_offset = offset;
  emit_u64(code, &offset, UINT64_C(0x0000000200000001));
  const size_t arg0_hi_data_offset = offset;
  emit_u64(code, &offset, UINT64_C(0x0000000400000003));
  const size_t arg1_lo_data_offset = offset;
  emit_u64(code, &offset, UINT64_C(0x000000140000000a));
  const size_t arg1_hi_data_offset = offset;
  emit_u64(code, &offset, UINT64_C(0x000000280000001e));
  const size_t target_data_offset = offset;
  emit_u64(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_vec128_u32_direct);
  store_u32(code, ld_arg0_lo_offset, riscv_ld(10, 28,
    (int32_t) arg0_lo_data_offset - (int32_t) auipc_data_pc));
  store_u32(code, ld_arg0_hi_offset, riscv_ld(11, 28,
    (int32_t) arg0_hi_data_offset - (int32_t) auipc_data_pc));
  store_u32(code, ld_arg1_lo_offset, riscv_ld(12, 28,
    (int32_t) arg1_lo_data_offset - (int32_t) auipc_data_pc));
  store_u32(code, ld_arg1_hi_offset, riscv_ld(13, 28,
    (int32_t) arg1_hi_data_offset - (int32_t) auipc_data_pc));
  store_u32(code, ld_target_offset, riscv_ld(5, 28,
    (int32_t) target_data_offset - (int32_t) auipc_data_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_vec128_signature_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 128;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-aarch64 vec128 signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_AARCH64);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(
      POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t target_offset = offset;
  emit_u32(code, &offset, 0x4ea18400U); // add v0.4s,v0.4s,v1.4s
  emit_u32(code, &offset, 0xd65f03c0U); // ret x30
  const size_t return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + target_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_vec128_u32(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_x86_pcall_vec128_signature_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 128;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: x86-to-riscv vec128 signature pcall mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_RISCV);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(
      POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t target_offset = offset;
  emit_riscv_vec128_u32_pair_add(code, &offset, 10, 11, 10, 11, 12, 13);
  emit_u32(code, &offset, 0x00008067U); // ret
  const size_t return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;
  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + target_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_vec128_u32(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_vec128_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv vec128 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_AARCH64);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(
      POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t pcall_return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(
      POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32));
  emit_u32(code, &offset, 0xd65f03c0U); // ret to x86 PCALL return cookie

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_riscv_vec128_u32_pair_add(code, &offset, 10, 11, 10, 11, 12, 13);
  emit_u32(code, &offset, 0x00008067U); // ret

  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + aarch64_body_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + pcall_return_offset));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_vec128_u32(code);
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_vec128_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 vec128 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x53; // push rbx
  code[offset++] = 0x41;
  code[offset++] = 0x57; // push r15
  emit_x86_movabs_r15(code, &offset, POLY_FRONTEND_RISCV);
  const size_t target_imm_offset = emit_x86_movabs_rbx(code, &offset, 0);
  const size_t return_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc,
    (uint8_t) POLYBENCH_X86_PCALL_SIG_IMM(
      POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32)
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t pcall_return_offset = offset;
  code[offset++] = 0x41;
  code[offset++] = 0x5f; // pop r15
  code[offset++] = 0x5b; // pop rbx
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t riscv_body_offset = offset;
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x00100313U); // addi x6,zero,1 (AArch64 frontend)
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(
      POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00008067U); // ret to x86 PCALL return cookie

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x4ea18400U); // add v0.4s,v0.4s,v1.4s
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u64(code, target_imm_offset,
    (uint64_t) (uintptr_t) (code + riscv_body_offset));
  store_u64(code, return_imm_offset,
    (uint64_t) (uintptr_t) (code + pcall_return_offset));
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_vec128_u32(code);
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_mixed_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv mixed call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 8 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_u32(code, &offset, 0xd28000e0U); // movz x0,#7
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0xd22575d3U); // fcvt.d.l fa1,a0
  emit_u32(code, &offset, 0x02b57553U); // fadd.d fa0,fa0,fa1
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double) = (double (*)(double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(2.5)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_mixed_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 mixed call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00700513U); // addi a0,zero,7
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x9e630001U); // ucvtf d1,x0
  emit_u32(code, &offset, 0x1e612800U); // fadd d0,d0,d1
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double) = (double (*)(double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(2.5)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_stack_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv stack call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 4 * 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 3 * 4 + 1;

  emit_u32(code, &offset, 0xd10043ffU); // sub sp,sp,#16
  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_u32(code, &offset, 0xd2800128U); // movz x8,#9
  emit_u32(code, &offset, 0xf90003e8U); // str x8,[sp]
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, 0x910043ffU); // add sp,sp,#16
  emit_u32(code, &offset, 0x91003400U); // add x0,x0,#13
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00013283U); // ld t0,0(sp)
  emit_u32(code, &offset, 0x00550533U); // add a0,a0,t0
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_stack_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 stack call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0xff010113U); // addi sp,sp,-16
  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  emit_u32(code, &offset, 0x00900393U); // addi t2,zero,9
  emit_u32(code, &offset, 0x00713023U); // sd t2,0(sp)
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x01010113U); // addi sp,sp,16
  emit_u32(code, &offset, 0x00d50513U); // addi a0,a0,13
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xf94003e8U); // ldr x8,[sp]
  emit_u32(code, &offset, 0x8b080000U); // add x0,x0,x8
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_saved_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv saved-reg call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 2 * 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 2 * 4 + 1;

  emit_u32(code, &offset, 0xd28000b3U); // movz x19,#5
  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, 0x8b130000U); // add x0,x0,x19
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01150513U); // addi a0,a0,17
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_saved_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 saved-reg call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00500413U); // addi s0,zero,5
  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00850533U); // add a0,a0,s0
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91004400U); // add x0,x0,#17
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_saved_fp_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv saved-fp call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] =
    { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 2 * 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 2 * 4 + 1;

  emit_u32(code, &offset, 0x1e604008U); // fmov d8,d0
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 0));
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 8));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, riscv_fsgnj_d(8, 10, 10)); // fmv.d fs0,fa0
  emit_u32(code, &offset, riscv_fadd_d(10, 8, 10));
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double) = (double (*)(double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(3.0)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_saved_fp_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 saved-fp call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, riscv_fsgnj_d(8, 10, 10)); // fmv.d fs0,fa0
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 10));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 8));
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x1e604008U); // fmov d8,d0
  emit_u32(code, &offset, aarch64_fadd_d(0, 8, 0));
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  double (*entry)(double) = (double (*)(double)) code;
  POLYBENCH_CALL_SAVE_R15(*result_bits, fp64_to_bits(entry(3.0)));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_pair_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv pair call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 2 * 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, 0x8b010000U); // add x0,x0,x1
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  emit_u32(code, &offset, 0x01600593U); // addi a1,zero,22
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_pair_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 pair call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00b50533U); // add a0,a0,a1
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_u32(code, &offset, 0xd28002c1U); // movz x1,#22
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_compact_u32_f32_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv compact u32/f32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 16 + 4 + 8 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 12 + 4 + 1;

  emit_aarch64_movabs(code, &offset, 0, 0x4010000000000003ULL);
  emit_u32(code, &offset, 0xd28000a1U); // movz x1,#5
  emit_u32(code, &offset, aarch64_lsr_imm(9, 0, 32));
  emit_u32(code, &offset, aarch64_fmov_s_from_w(0, 9));
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, aarch64_fmov_w_from_s(9, 0));
  emit_u32(code, &offset, aarch64_lsl_imm(9, 9, 32));
  emit_u32(code, &offset, aarch64_orr_reg(0, 0, 9));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0xd015f7d3U); // fcvt.s.wu fa5,a1
  emit_u32(code, &offset, 0x00b5053bU); // addw a0,a0,a1
  emit_u32(code, &offset, 0x00a7f553U); // fadd.s fa0,fa5,fa0
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_compact_f32_u32_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv compact f32/u32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 16 + 4 + 8 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 12 + 4 + 1;

  emit_aarch64_movabs(code, &offset, 0, 0x0000000340100000ULL);
  emit_u32(code, &offset, 0xd28000a1U); // movz x1,#5
  emit_u32(code, &offset, aarch64_fmov_s_from_w(0, 0));
  emit_u32(code, &offset, aarch64_lsr_imm(0, 0, 32));
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, aarch64_fmov_w_from_s(9, 0));
  emit_u32(code, &offset, aarch64_lsl_imm(0, 0, 32));
  emit_u32(code, &offset, aarch64_orr_reg(0, 0, 9));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0xd015f7d3U); // fcvt.s.wu fa5,a1
  emit_u32(code, &offset, 0x00b5053bU); // addw a0,a0,a1
  emit_u32(code, &offset, 0x00a7f553U); // fadd.s fa0,fa5,fa0
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_compact_u32_f32_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 384;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 compact u32/f32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00300513U); // addi a0,zero,3
  emit_u32(code, &offset, 0x00500593U); // addi a1,zero,5
  emit_u32(code, &offset, riscv_fmv_x_w(28, 10)); // t3=fa0 bits
  emit_u32(code, &offset, riscv_slli(28, 28, 32));
  emit_u32(code, &offset, riscv_or(10, 10, 28));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, riscv_srli(28, 10, 32));
  emit_u32(code, &offset, riscv_fmv_w_x(10, 28));
  emit_u32(code, &offset, 0xe0050653U); // fmv.x.w a2,fa0
  emit_u32(code, &offset, 0x02061613U); // slli a2,a2,32
  emit_u32(code, &offset, 0x00c56533U); // or a0,a0,a2
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91001400U); // add x0,x0,#5
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  uint64_t (*entry)(float) = (uint64_t (*)(float)) code;
  POLYBENCH_CALL_SAVE_R15(*result, entry(2.25f));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_compact_f32_u32_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 384;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 compact f32/u32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00300513U); // addi a0,zero,3
  emit_u32(code, &offset, 0x00500593U); // addi a1,zero,5
  emit_u32(code, &offset, riscv_fmv_x_w(28, 10)); // t3=fa0 bits
  emit_u32(code, &offset, riscv_slli(10, 10, 32));
  emit_u32(code, &offset, riscv_or(10, 10, 28));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, riscv_fmv_w_x(10, 10));
  emit_u32(code, &offset, riscv_srli(10, 10, 32));
  emit_u32(code, &offset, 0xe0050653U); // fmv.x.w a2,fa0
  emit_u32(code, &offset, 0x02051513U); // slli a0,a0,32
  emit_u32(code, &offset, 0x00c56533U); // or a0,a0,a2
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_aarch64_movabs(code, &offset, 1, 0x0000000500000000ULL);
  emit_u32(code, &offset, 0x8b010000U); // add x0,x0,x1
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  uint64_t (*entry)(float) = (uint64_t (*)(float)) code;
  POLYBENCH_CALL_SAVE_R15(*result, entry(2.25f));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_syscall_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv syscall call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x04d00513U); // addi a0,zero,77
  emit_u32(code, &offset, 0x04e00593U); // addi a1,zero,78
  emit_u32(code, &offset, 0x04f00613U); // addi a2,zero,79
  emit_u32(code, &offset, 0x05000693U); // addi a3,zero,80
  emit_u32(code, &offset, 0x05100713U); // addi a4,zero,81
  emit_u32(code, &offset, 0x05200793U); // addi a5,zero,82
  emit_u32(code, &offset, 0x05800813U); // addi a6,zero,88
  emit_u32(code, &offset, 0x0ac00893U); // addi a7,zero,172
  emit_u32(code, &offset, 0x00000073U); // ecall
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_syscall_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 syscall call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xd28009a0U); // movz x0,#77
  emit_u32(code, &offset, 0xd28009c1U); // movz x1,#78
  emit_u32(code, &offset, 0xd28009e2U); // movz x2,#79
  emit_u32(code, &offset, 0xd2800a03U); // movz x3,#80
  emit_u32(code, &offset, 0xd2800a24U); // movz x4,#81
  emit_u32(code, &offset, 0xd2800a45U); // movz x5,#82
  emit_u32(code, &offset, 0xd2800b06U); // movz x6,#88
  emit_u32(code, &offset, 0xd2800c67U); // movz x7,#99
  emit_u32(code, &offset, 0xd2801588U); // movz x8,#172
  emit_u32(code, &offset, 0xd4000001U); // svc #0
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_break_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv break call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00100893U); // addi a7,zero,1
  emit_u32(code, &offset, 0x00100073U); // ebreak
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_break_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 break call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xd4200020U); // brk #1, neutral break trap
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_import_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv import call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0xffffe2b7U); // lui t0,0xffffe
  emit_u32(code, &offset, 0x08028293U); // addi t0,t0,0x80 -> import id 8
  emit_u32(code, &offset, 0x04d00513U); // addi a0,zero,77
  emit_u32(code, &offset, 0x04e00593U); // addi a1,zero,78
  emit_u32(code, &offset, 0x04f00613U); // addi a2,zero,79
  emit_u32(code, &offset, 0x05000693U); // addi a3,zero,80
  emit_u32(code, &offset, 0x05100713U); // addi a4,zero,81
  emit_u32(code, &offset, 0x05200793U); // addi a5,zero,82
  emit_u32(code, &offset, 0x05800813U); // addi a6,zero,88
  emit_u32(code, &offset, 0x06300893U); // addi a7,zero,99
  emit_u32(code, &offset, 0x00028367U); // jalr t1,0(t0), preserve ra cookie
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_import_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 import call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_aarch64_movabs(code, &offset, 16, UINT64_C(0xffffffffffffe080));
  emit_u32(code, &offset, 0xd28009a0U); // movz x0,#77
  emit_u32(code, &offset, 0xd28009c1U); // movz x1,#78
  emit_u32(code, &offset, 0xd28009e2U); // movz x2,#79
  emit_u32(code, &offset, 0xd2800a03U); // movz x3,#80
  emit_u32(code, &offset, 0xd2800a24U); // movz x4,#81
  emit_u32(code, &offset, 0xd2800a45U); // movz x5,#82
  emit_u32(code, &offset, 0xd2800b06U); // movz x6,#88
  emit_u32(code, &offset, 0xd2800c67U); // movz x7,#99
  emit_u32(code, &offset, 0xd61f0200U); // br x16, preserve x30 cookie
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv direct x86 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00008413U); // addi s0,ra,0; save cross-return cookie
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, 0x00040093U); // addi ra,s0,0; restore cross-return cookie
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_strlen);

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  static const char payload[] = "polyglot";
  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_with_rax_arg(code, payload);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 direct x86 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xaa1e03f3U); // mov x19,x30; save cross-return cookie
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_strlen);
  emit_u32(code, &offset, 0xaa1303feU); // mov x30,x19; restore cross-return cookie
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  static const char payload[] = "polyglot";
  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_with_rax_arg(code, payload);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_memcmp_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv direct x86 memcmp mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00008413U); // addi s0,ra,0; save cross-return cookie
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, 0x00040093U); // addi ra,s0,0; restore cross-return cookie
  emit_u32(code, &offset, 0x02a50513U); // addi a0,a0,42
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_memcmp);

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  static const char left[] = "polyglot";
  static const char right[] = "polyglot";
  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_with_poly3_args(code, left, right, sizeof(left));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_memcmp_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 direct x86 memcmp mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xaa1e03f3U); // mov x19,x30; save cross-return cookie
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_memcmp);
  emit_u32(code, &offset, 0xaa1303feU); // mov x30,x19; restore cross-return cookie
  emit_u32(code, &offset, 0x9100a800U); // add x0,x0,#42
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  static const char left[] = "polyglot";
  static const char right[] = "polyglot";
  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_with_poly3_args(code, left, right, sizeof(left));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_memops_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv direct x86 memops mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, riscv_addi(8, 1, 0));   // s0=ra; save cross-return cookie
  emit_u32(code, &offset, riscv_addi(9, 11, 0));  // s1=a1; save source
  emit_u32(code, &offset, riscv_addi(18, 12, 0)); // s2=a2; save count
  emit_u32(code, &offset, riscv_addi(19, 10, 0)); // s3=a0; save destination
  emit_u32(code, &offset, riscv_addi(11, 0, 0x58)); // a1='X'
  const size_t auipc_memset_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_memset_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, riscv_addi(10, 19, 0)); // a0=s3
  emit_u32(code, &offset, riscv_addi(11, 9, 0));  // a1=s1
  emit_u32(code, &offset, riscv_addi(12, 18, 0)); // a2=s2
  const size_t auipc_memcpy_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_memcpy_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, riscv_addi(10, 9, 0)); // a0=s1
  emit_u32(code, &offset, riscv_addi(11, 0, 4)); // a1=4
  const size_t auipc_strnlen_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_strnlen_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, riscv_addi(10, 10, 38)); // a0=42
  emit_u32(code, &offset, riscv_addi(1, 8, 0)); // ra=s0
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t memset_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_memset);
  const size_t memcpy_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_memcpy);
  const size_t strnlen_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_strnlen);

  store_u32(code, ld_memset_offset, riscv_ld(5, 5,
    (int32_t) memset_data_offset - (int32_t) auipc_memset_pc));
  store_u32(code, ld_memcpy_offset, riscv_ld(5, 5,
    (int32_t) memcpy_data_offset - (int32_t) auipc_memcpy_pc));
  store_u32(code, ld_strnlen_offset, riscv_ld(5, 5,
    (int32_t) strnlen_data_offset - (int32_t) auipc_strnlen_pc));

  static const char source[] = "polyglot";
  char dest[sizeof(source)];
  memset(dest, 0, sizeof(dest));
  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  uint64_t raw_result = call_code_with_poly3_args(code, dest,
    source, sizeof(source));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;
  *result = (raw_result == 42 &&
      memcmp(dest, source, sizeof(source)) == 0) ? 42 : 0;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_memops_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 direct x86 memops mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, aarch64_mov_reg(19, 30)); // save cross-return cookie
  emit_u32(code, &offset, aarch64_mov_reg(20, 1));  // save source
  emit_u32(code, &offset, aarch64_mov_reg(21, 2));  // save count
  emit_u32(code, &offset, aarch64_mov_reg(22, 0));  // save destination
  emit_u32(code, &offset, 0xd2800b01U); // movz x1,#'X'
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_memset);
  emit_u32(code, &offset, aarch64_mov_reg(0, 22));
  emit_u32(code, &offset, aarch64_mov_reg(1, 20));
  emit_u32(code, &offset, aarch64_mov_reg(2, 21));
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_memcpy);
  emit_u32(code, &offset, aarch64_mov_reg(0, 20));
  emit_u32(code, &offset, 0xd2800081U); // movz x1,#4
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_strnlen);
  emit_u32(code, &offset, 0x91009800U); // add x0,x0,#38
  emit_u32(code, &offset, aarch64_mov_reg(30, 19)); // restore cross-return cookie
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  static const char source[] = "polyglot";
  char dest[sizeof(source)];
  memset(dest, 0, sizeof(dest));
  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  uint64_t raw_result = call_code_with_poly3_args(code, dest,
    source, sizeof(source));
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;
  *result = (raw_result == 42 &&
      memcmp(dest, source, sizeof(source)) == 0) ? 42 : 0;

  munmap(code, code_size);
  return 0;
}

static int run_nested_cross_call(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 384;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: nested cross call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x6a, 0x01, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_outer_offset = offset;
  const size_t aarch64_outer_return_offset =
    aarch64_outer_offset + 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_outer_return_offset + 4 + 1;

  emit_u32(code, &offset, 0xd2800140U); // movz x0,#10
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_outer_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00b50513U); // addi a0,a0,11
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00150513U); // addi a0,a0,1
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_inner_offset = offset;
  emit_u32(code, &offset, 0x91005000U); // add x0,x0,#20
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_inner_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_nested_reverse_cross_call(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: nested reverse cross call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x6a, 0x02, 0x41, 0x5f, 0x0f, 0x3a, 0xfc, 0x03 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00a00513U); // addi a0,zero,10
  const size_t outer_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t outer_ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t outer_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t outer_ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset,
    POLYBENCH_RISCV_PCALL_SIG_IMM(polybench_native_signature_slot));
  const size_t riscv_outer_return_offset = offset;
  emit_u32(code, &offset, 0x00050513U); // addi a0,a0,0
  emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_target_offset + 4 + 16 + 4 + 16 + 4;
  const size_t riscv_inner_target_offset = aarch64_return_offset + 4 + 4;

  emit_u32(code, &offset, 0x91002c00U); // add x0,x0,#11
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_inner_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset,
    POLYBENCH_AARCH64_PCALL_SIG_IMM(polybench_native_signature_slot));
  emit_u32(code, &offset, 0x91000400U); // add x0,x0,#1
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while (offset < riscv_inner_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01450513U); // addi a0,a0,20
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t outer_target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t outer_return_data_offset = offset;
  emit_u64(code, &offset,
    (uint64_t) (uintptr_t) (code + riscv_outer_return_offset));

  store_u32(code, outer_ld_target_offset, riscv_ld(5, 5,
    (int32_t) outer_target_data_offset - (int32_t) outer_target_pc));
  store_u32(code, outer_ld_return_offset, riscv_ld(7, 7,
    (int32_t) outer_return_data_offset - (int32_t) outer_return_pc));

  uint64_t insns_before = poly_foreign_insn_count_status_value();
  uint64_t switches_before = poly_switch_count_status_value();
  *result = call_code_no_args(code);
  poly_mode_x86();
  *insn_delta = poly_foreign_insn_count_status_value() - insns_before;
  *switch_delta = poly_switch_count_status_value() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int check_loop(const char *name, int arch) {
  uint64_t result = 0;
  uint64_t delta = 0;
  uint64_t switch_delta = 0;
  if (run_loop_program(arch, &result, &delta, &switch_delta) < 0)
    return -1;

  const uint64_t min_expected_delta = 1 + (uint64_t) LOOP_ITERS * 2 + 1;
  printf("POLYBENCH_RESULT: arch=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) delta,
    (unsigned long long) switch_delta);

  if (result != 0) {
    fprintf(stderr, "POLYBENCH_FAIL: %s loop result expected 0 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (delta < min_expected_delta) {
    fprintf(stderr, "POLYBENCH_FAIL: %s raw instruction delta expected at least %llu got %llu\n",
      name, (unsigned long long) min_expected_delta, (unsigned long long) delta);
    return -1;
  }
  if (delta > POLYBENCH_LOOP_MAX_RAW_INSNS) {
    fprintf(stderr, "POLYBENCH_FAIL: %s raw instruction delta expected at most %u got %llu\n",
      name, POLYBENCH_LOOP_MAX_RAW_INSNS, (unsigned long long) delta);
    return -1;
  }
  if (switch_delta != POLYBENCH_LOOP_EXPECTED_SWITCH_DELTA) {
    fprintf(stderr, "POLYBENCH_FAIL: %s loop switch delta expected exactly %u got %llu\n",
      name, POLYBENCH_LOOP_EXPECTED_SWITCH_DELTA,
      (unsigned long long) switch_delta);
    return -1;
  }
  return 0;
}

static int check_raw_insn_delta_max(const char *kind, const char *name,
    uint64_t insn_delta, uint64_t max_insn_delta) {
  if (insn_delta > max_insn_delta) {
    fprintf(stderr, "POLYBENCH_FAIL: %s %s raw instruction delta expected at most %llu got %llu\n",
      kind, name, (unsigned long long) max_insn_delta,
      (unsigned long long) insn_delta);
    return -1;
  }
  return 0;
}

static int check_switch_delta_max(const char *kind, const char *name,
    uint64_t switch_delta, uint64_t max_switch_delta) {
  if (switch_delta > max_switch_delta) {
    fprintf(stderr, "POLYBENCH_FAIL: %s %s switch delta expected at most %llu got %llu\n",
      kind, name, (unsigned long long) max_switch_delta,
      (unsigned long long) switch_delta);
    return -1;
  }
  return 0;
}

static int check_switch_delta_exact(const char *kind, const char *name,
    uint64_t switch_delta, uint64_t expected_switch_delta) {
  if (switch_delta != expected_switch_delta) {
    fprintf(stderr, "POLYBENCH_FAIL: %s %s switch delta expected exactly %llu got %llu\n",
      kind, name, (unsigned long long) expected_switch_delta,
      (unsigned long long) switch_delta);
    return -1;
  }
  return 0;
}

static int check_mixed_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_MIXED_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: mixed %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: mixed %s raw instruction delta expected at least 4 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("mixed", name, insn_delta,
        POLYBENCH_MIXED_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("mixed", name, switch_delta,
        POLYBENCH_MIXED_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("mixed", name, switch_delta,
        POLYBENCH_MIXED_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_mixed(void) {
  if (check_mixed_direction("aarch64-to-riscv", run_mixed_program) < 0)
    return -1;
  if (check_mixed_direction("aarch64-to-compressed-riscv",
        run_compressed_mixed_program) < 0)
    return -1;
  if (check_mixed_direction("riscv-to-aarch64", run_reverse_mixed_program) < 0)
    return -1;
  if (check_mixed_direction("riscv-compressed-to-aarch64",
        run_compressed_reverse_mixed_program) < 0)
    return -1;
  return 0;
}

static int check_cross_call_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *),
    uint64_t max_insn_delta,
    uint64_t expected_switch_delta,
    uint64_t max_switch_delta) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("cross call", name, insn_delta,
        max_insn_delta) < 0)
    return -1;
  if (check_switch_delta_exact("cross call", name, switch_delta,
        expected_switch_delta) < 0)
    return -1;
  if (check_switch_delta_max("cross call", name, switch_delta,
        max_switch_delta) < 0)
    return -1;
  return 0;
}

static int check_neutral_pcall_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  if (switch_delta < POLYBENCH_NEUTRAL_PCALL_WRAPPER_SWITCH_DELTA) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral pcall %s switch delta underflow got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  const uint64_t inner_switch_delta =
    switch_delta - POLYBENCH_NEUTRAL_PCALL_WRAPPER_SWITCH_DELTA;
  printf("POLYBENCH_NEUTRAL_PCALL_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu inner_switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta,
    (unsigned long long) inner_switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: neutral pcall %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: neutral pcall %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("neutral pcall", name, insn_delta,
        POLYBENCH_NEUTRAL_PCALL_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("neutral pcall", name, switch_delta,
        POLYBENCH_NEUTRAL_PCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_exact("neutral pcall inner", name,
        inner_switch_delta,
        POLYBENCH_NEUTRAL_PCALL_EXPECTED_INNER_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_neutral_pcall_fp64_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  if (switch_delta < POLYBENCH_NEUTRAL_PCALL_WRAPPER_SWITCH_DELTA) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral FP64 pcall %s switch delta underflow got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  const uint64_t inner_switch_delta =
    switch_delta - POLYBENCH_NEUTRAL_PCALL_WRAPPER_SWITCH_DELTA;
  printf("POLYBENCH_NEUTRAL_PCALL_FP64_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu inner_switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta,
    (unsigned long long) inner_switch_delta);

  if (result_bits != UINT64_C(0x400b000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: neutral FP64 pcall %s expected 0x400b000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: neutral FP64 pcall %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("neutral FP64 pcall", name, insn_delta,
        POLYBENCH_NEUTRAL_PCALL_FP64_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("neutral FP64 pcall", name, switch_delta,
        POLYBENCH_NEUTRAL_PCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_exact("neutral FP64 pcall inner", name,
        inner_switch_delta,
        POLYBENCH_NEUTRAL_PCALL_EXPECTED_INNER_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_neutral_pcall_fp32_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  if (switch_delta < POLYBENCH_NEUTRAL_PCALL_WRAPPER_SWITCH_DELTA) {
    fprintf(stderr,
      "POLYBENCH_FAIL: neutral FP32 pcall %s switch delta underflow got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  const uint64_t inner_switch_delta =
    switch_delta - POLYBENCH_NEUTRAL_PCALL_WRAPPER_SWITCH_DELTA;
  printf("POLYBENCH_NEUTRAL_PCALL_FP32_RESULT: direction=%s bits=0x%08llx raw_insn_delta=%llu switch_delta=%llu inner_switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta,
    (unsigned long long) inner_switch_delta);

  if (result_bits != UINT64_C(0x40580000)) {
    fprintf(stderr, "POLYBENCH_FAIL: neutral FP32 pcall %s expected 0x40580000 got 0x%08llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: neutral FP32 pcall %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("neutral FP32 pcall", name, insn_delta,
        POLYBENCH_NEUTRAL_PCALL_FP32_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("neutral FP32 pcall", name, switch_delta,
        POLYBENCH_NEUTRAL_PCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_exact("neutral FP32 pcall inner", name,
        inner_switch_delta,
        POLYBENCH_NEUTRAL_PCALL_EXPECTED_INNER_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_direct_x86_pcall_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_DIRECT_X86_PCALL_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 21) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 pcall %s result expected 21 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 pcall %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("direct x86 pcall", name, insn_delta,
        POLYBENCH_DIRECT_X86_PCALL_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("direct x86 pcall", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("direct x86 pcall", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_direct_x86_sret_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_DIRECT_X86_SRET_RESULT: direction=%s packed=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != UINT64_C(0x0003000700050006)) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 SRET %s expected 0x0003000700050006 got 0x%016llx\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 18) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 SRET %s raw instruction delta expected at least 18 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("direct x86 SRET", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("direct x86 SRET", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_direct_x86_fp64_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_DIRECT_X86_FP64_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x403d000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 FP64 %s expected 0x403d000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 FP64 %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("direct x86 FP64", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("direct x86 FP64", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_direct_x86_fp32_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_DIRECT_X86_FP32_RESULT: direction=%s bits=0x%08llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x41e80000)) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 FP32 %s expected 0x41e80000 got 0x%08llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 FP32 %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("direct x86 FP32", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("direct x86 FP32", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_direct_x86_vec128_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_DIRECT_X86_VEC128_RESULT: direction=%s packed=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != UINT64_C(0x007c005d003e001f)) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 vec128 %s expected 0x007c005d003e001f got 0x%016llx\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 vec128 %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("direct x86 vec128", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("direct x86 vec128", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_x86_pcall_signature_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_X86_PCALL_SIGNATURE_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 21) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall signature %s result expected 21 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 6) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall signature %s raw instruction delta expected at least 6 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("x86 pcall signature", name, insn_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("x86 pcall signature", name, switch_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("x86 pcall signature", name, switch_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_x86_pcall_sret_signature_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_X86_PCALL_SRET_SIGNATURE_RESULT: direction=%s packed=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != UINT64_C(0x0003000700050006)) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall SRET signature %s expected 0x0003000700050006 got 0x%016llx\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 9) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall SRET signature %s raw instruction delta expected at least 9 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("x86 pcall SRET signature", name, insn_delta,
        POLYBENCH_X86_PCALL_SRET_SIGNATURE_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("x86 pcall SRET signature", name,
        switch_delta, POLYBENCH_X86_PCALL_SIGNATURE_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("x86 pcall SRET signature", name, switch_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_x86_pcall_fp32_signature_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_X86_PCALL_FP32_SIGNATURE_RESULT: direction=%s bits=0x%08llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x41a80000)) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall FP32 signature %s result expected 0x41a80000 got 0x%08llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 6) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall FP32 signature %s raw instruction delta expected at least 6 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("x86 pcall FP32 signature", name, insn_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("x86 pcall FP32 signature", name, switch_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("x86 pcall FP32 signature", name, switch_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_x86_pcall_fp64_signature_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_X86_PCALL_FP64_SIGNATURE_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4035000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall FP64 signature %s result expected 0x4035000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 6) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall FP64 signature %s raw instruction delta expected at least 6 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("x86 pcall FP64 signature", name, insn_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("x86 pcall FP64 signature", name, switch_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("x86 pcall FP64 signature", name, switch_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_x86_pcall_vec128_signature_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_X86_PCALL_VEC128_SIGNATURE_RESULT: direction=%s packed=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != UINT64_C(0x002c00210016000b)) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall vec128 signature %s expected 0x002c00210016000b got 0x%016llx\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 2) {
    fprintf(stderr, "POLYBENCH_FAIL: x86 pcall vec128 signature %s raw instruction delta expected at least 2 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("x86 pcall vec128 signature", name, insn_delta,
        POLYBENCH_X86_PCALL_VEC128_SIGNATURE_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("x86 pcall vec128 signature", name,
        switch_delta, POLYBENCH_X86_PCALL_SIGNATURE_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("x86 pcall vec128 signature", name, switch_delta,
        POLYBENCH_X86_PCALL_SIGNATURE_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_fp_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_FP_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4030000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP %s result expected 0x4030000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call FP", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call FP", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_fp8_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_FP8_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4042000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP8 %s result expected 0x4042000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 14) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP8 %s raw instruction delta expected at least 14 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call FP8", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call FP8", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_fp64_signature_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_FP64_SIGNATURE_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x400b000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP64 signature %s result expected 0x400b000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP64 signature %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("cross call FP64 signature", name, insn_delta,
        POLYBENCH_CROSS_CALL_SIGNATURE_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("cross call FP64 signature", name,
        switch_delta, POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call FP64 signature", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_fp32_signature_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_FP32_SIGNATURE_RESULT: direction=%s bits=0x%08llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x40580000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP32 signature %s result expected 0x40580000 got 0x%08llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP32 signature %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("cross call FP32 signature", name, insn_delta,
        POLYBENCH_CROSS_CALL_SIGNATURE_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("cross call FP32 signature", name,
        switch_delta, POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call FP32 signature", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_fp64_stack_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_FP64_STACK_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4061000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP64 stack %s result expected 0x4061000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 30) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP64 stack %s raw instruction delta expected at least 30 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call FP64 stack", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call FP64 stack", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_vec128_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_VEC128_RESULT: direction=%s packed=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != UINT64_C(0x002c00210016000b)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call vec128 %s expected 0x002c00210016000b got 0x%016llx\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call vec128 %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_raw_insn_delta_max("cross call vec128", name, insn_delta,
        POLYBENCH_CROSS_CALL_VEC128_MAX_RAW_INSNS) < 0)
    return -1;
  if (check_switch_delta_exact("cross call vec128", name, switch_delta,
        POLYBENCH_CROSS_CALL_VEC128_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call vec128", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_mixed_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_MIXED_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4038000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call mixed %s result expected 0x4038000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call mixed %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call mixed", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call mixed", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_stack_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_STACK_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call stack %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call stack %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call stack", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call stack", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_saved_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_SAVED_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call saved", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call saved", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_saved_fp_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_SAVED_FP_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x402e000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved-fp %s expected 0x402e000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 11) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved-fp %s raw instruction delta expected at least 11 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call saved-fp", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call saved-fp", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_pair_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_PAIR_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call pair %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 9) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call pair %s raw instruction delta expected at least 9 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call pair", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call pair", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_compact_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *), uint64_t expected) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_COMPACT_RESULT: direction=%s packed=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != expected) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call compact %s expected 0x%016llx got 0x%016llx\n",
      name, (unsigned long long) expected, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 9) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call compact %s raw instruction delta expected at least 9 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call compact", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call compact", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_syscall_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_SYSCALL_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 4242) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call syscall %s result expected 4242 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 9) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call syscall %s raw instruction delta expected at least 9 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call syscall", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call syscall", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_break_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *),
    uint64_t expected) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_BREAK_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != expected) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call break %s expected %llu got %llu\n",
      name, (unsigned long long) expected, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call break %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call break", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call break", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_import_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_IMPORT_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 5555) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call import %s expected 5555 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 9) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call import %s raw instruction delta expected at least 9 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call import", name, switch_delta,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call import", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_string_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_STRING_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call string %s strlen expected 8 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call string %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call string", name, switch_delta,
        POLYBENCH_DIRECT_X86_LIBCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call string", name, switch_delta,
        POLYBENCH_DIRECT_X86_LIBCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_direct_x86_memcmp_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_DIRECT_X86_MEMCMP_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memcmp %s expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memcmp %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call direct x86 memcmp", name,
        switch_delta, POLYBENCH_DIRECT_X86_LIBCALL_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call direct x86 memcmp", name,
        switch_delta, POLYBENCH_DIRECT_X86_LIBCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_direct_x86_memops_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_DIRECT_X86_MEMOPS_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memops %s expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 20) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memops %s raw instruction delta expected at least 20 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (check_switch_delta_exact("cross call direct x86 memops", name,
        switch_delta, POLYBENCH_DIRECT_X86_MEMOPS_EXPECTED_SWITCH_DELTA) < 0)
    return -1;
  if (check_switch_delta_max("cross call direct x86 memops", name,
        switch_delta, POLYBENCH_DIRECT_X86_MEMOPS_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_calls(void) {
  if (check_cross_call_direction("aarch64-calls-riscv",
        run_cross_call_aarch64_to_riscv,
        POLYBENCH_CROSS_CALL_MAX_RAW_INSNS,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  if (check_cross_call_direction("riscv-calls-aarch64",
        run_cross_call_riscv_to_aarch64,
        POLYBENCH_CROSS_CALL_MAX_RAW_INSNS,
        POLYBENCH_CROSS_CALL_EXPECTED_SWITCH_DELTA,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  if (check_neutral_pcall_direction("x86-wrapped-aarch64-calls-riscv",
        run_neutral_pcall_aarch64_to_riscv) < 0)
    return -1;
  if (check_neutral_pcall_direction("x86-wrapped-riscv-calls-aarch64",
        run_neutral_pcall_riscv_to_aarch64) < 0)
    return -1;
  if (check_neutral_pcall_fp64_direction(
        "x86-wrapped-aarch64-calls-riscv-fp64",
        run_neutral_pcall_fp64_aarch64_to_riscv) < 0)
    return -1;
  if (check_neutral_pcall_fp64_direction(
        "x86-wrapped-riscv-calls-aarch64-fp64",
        run_neutral_pcall_fp64_riscv_to_aarch64) < 0)
    return -1;
  if (check_neutral_pcall_fp32_direction(
        "x86-wrapped-aarch64-calls-riscv-fp32",
        run_neutral_pcall_fp32_aarch64_to_riscv) < 0)
    return -1;
  if (check_neutral_pcall_fp32_direction(
        "x86-wrapped-riscv-calls-aarch64-fp32",
        run_neutral_pcall_fp32_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_direction("nested-aarch64-riscv-aarch64",
        run_nested_cross_call,
        POLYBENCH_NESTED_CROSS_CALL_MAX_RAW_INSNS,
        POLYBENCH_NESTED_CROSS_CALL_EXPECTED_SWITCH_DELTA,
        POLYBENCH_NESTED_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  if (check_cross_call_direction("nested-riscv-aarch64-riscv",
        run_nested_reverse_cross_call,
        POLYBENCH_NESTED_CROSS_CALL_MAX_RAW_INSNS,
        POLYBENCH_NESTED_CROSS_CALL_EXPECTED_SWITCH_DELTA,
        POLYBENCH_NESTED_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  if (check_direct_x86_pcall_direction("aarch64-calls-x86-direct",
        run_direct_x86_pcall_aarch64) < 0)
    return -1;
  if (check_direct_x86_pcall_direction("riscv-calls-x86-direct",
        run_direct_x86_pcall_riscv) < 0)
    return -1;
  if (check_direct_x86_sret_direction("aarch64-calls-x86-direct-sret",
        run_direct_x86_sret_aarch64) < 0)
    return -1;
  if (check_direct_x86_sret_direction("riscv-calls-x86-direct-sret",
        run_direct_x86_sret_riscv) < 0)
    return -1;
  if (check_direct_x86_fp64_direction("aarch64-calls-x86-direct-fp64",
        run_direct_x86_fp64_aarch64) < 0)
    return -1;
  if (check_direct_x86_fp64_direction("riscv-calls-x86-direct-fp64",
        run_direct_x86_fp64_riscv) < 0)
    return -1;
  if (check_direct_x86_fp32_direction("aarch64-calls-x86-direct-fp32",
        run_direct_x86_fp32_aarch64) < 0)
    return -1;
  if (check_direct_x86_fp32_direction("riscv-calls-x86-direct-fp32",
        run_direct_x86_fp32_riscv) < 0)
    return -1;
  if (check_direct_x86_vec128_direction("aarch64-calls-x86-direct-vec128",
        run_direct_x86_vec128_aarch64) < 0)
    return -1;
  if (check_direct_x86_vec128_direction("riscv-calls-x86-direct-vec128",
        run_direct_x86_vec128_riscv) < 0)
    return -1;
  if (check_x86_pcall_signature_direction("x86-calls-aarch64-signature",
        run_x86_pcall_signature_aarch64) < 0)
    return -1;
  if (check_x86_pcall_signature_direction("x86-calls-riscv-signature",
        run_x86_pcall_signature_riscv) < 0)
    return -1;
  if (check_x86_pcall_sret_signature_direction(
        "x86-calls-aarch64-sret-signature",
        run_x86_pcall_sret_signature_aarch64) < 0)
    return -1;
  if (check_x86_pcall_sret_signature_direction(
        "x86-calls-riscv-sret-signature",
        run_x86_pcall_sret_signature_riscv) < 0)
    return -1;
  if (check_x86_pcall_fp64_signature_direction(
        "x86-calls-aarch64-fp64-signature",
        run_x86_pcall_fp64_signature_aarch64) < 0)
    return -1;
  if (check_x86_pcall_fp64_signature_direction(
        "x86-calls-riscv-fp64-signature",
        run_x86_pcall_fp64_signature_riscv) < 0)
    return -1;
  if (check_x86_pcall_fp32_signature_direction(
        "x86-calls-aarch64-fp32-signature",
        run_x86_pcall_fp32_signature_aarch64) < 0)
    return -1;
  if (check_x86_pcall_fp32_signature_direction(
        "x86-calls-riscv-fp32-signature",
        run_x86_pcall_fp32_signature_riscv) < 0)
    return -1;
  if (check_x86_pcall_vec128_signature_direction(
        "x86-calls-aarch64-vec128-signature",
        run_x86_pcall_vec128_signature_aarch64) < 0)
    return -1;
  if (check_x86_pcall_vec128_signature_direction(
        "x86-calls-riscv-vec128-signature",
        run_x86_pcall_vec128_signature_riscv) < 0)
    return -1;
  if (check_cross_call_fp_direction("aarch64-calls-riscv-fp",
        run_cross_call_fp_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp_direction("riscv-calls-aarch64-fp",
        run_cross_call_fp_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_fp8_direction("aarch64-calls-riscv-fp8",
        run_cross_call_fp8_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp8_direction("riscv-calls-aarch64-fp8",
        run_cross_call_fp8_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_fp64_signature_direction(
        "aarch64-calls-riscv-fp64-signature",
        run_cross_call_fp64_signature_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp64_signature_direction(
        "riscv-calls-aarch64-fp64-signature",
        run_cross_call_fp64_signature_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_fp32_signature_direction(
        "aarch64-calls-riscv-fp32-signature",
        run_cross_call_fp32_signature_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp32_signature_direction(
        "riscv-calls-aarch64-fp32-signature",
        run_cross_call_fp32_signature_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_fp64_stack_direction("aarch64-calls-riscv-fp64-stack",
        run_cross_call_fp64_stack_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp64_stack_direction("riscv-calls-aarch64-fp64-stack",
        run_cross_call_fp64_stack_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_vec128_direction("aarch64-calls-riscv-vec128",
        run_cross_call_vec128_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_vec128_direction("riscv-calls-aarch64-vec128",
        run_cross_call_vec128_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_mixed_direction("aarch64-calls-riscv-mixed",
        run_cross_call_mixed_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_mixed_direction("riscv-calls-aarch64-mixed",
        run_cross_call_mixed_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_stack_direction("aarch64-calls-riscv-stack",
        run_cross_call_stack_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_stack_direction("riscv-calls-aarch64-stack",
        run_cross_call_stack_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_saved_direction("aarch64-calls-riscv-saved",
        run_cross_call_saved_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_saved_direction("riscv-calls-aarch64-saved",
        run_cross_call_saved_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_saved_fp_direction("aarch64-calls-riscv-saved-fp",
        run_cross_call_saved_fp_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_saved_fp_direction("riscv-calls-aarch64-saved-fp",
        run_cross_call_saved_fp_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_pair_direction("aarch64-calls-riscv-pair",
        run_cross_call_pair_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_pair_direction("riscv-calls-aarch64-pair",
        run_cross_call_pair_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_compact_direction("aarch64-calls-riscv-compact-u32-f32",
        run_cross_call_compact_u32_f32_aarch64_to_riscv,
        UINT64_C(0x40e8000000000008)) < 0)
    return -1;
  if (check_cross_call_compact_direction("aarch64-calls-riscv-compact-f32-u32",
        run_cross_call_compact_f32_u32_aarch64_to_riscv,
        UINT64_C(0x0000000840e80000)) < 0)
    return -1;
  if (check_cross_call_compact_direction("riscv-calls-aarch64-compact-u32-f32",
        run_cross_call_compact_u32_f32_riscv_to_aarch64,
        UINT64_C(0x4010000000000008)) < 0)
    return -1;
  if (check_cross_call_compact_direction("riscv-calls-aarch64-compact-f32-u32",
        run_cross_call_compact_f32_u32_riscv_to_aarch64,
        UINT64_C(0x0000000840100000)) < 0)
    return -1;
  if (check_cross_call_syscall_direction("aarch64-calls-riscv-syscall",
        run_cross_call_syscall_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_syscall_direction("riscv-calls-aarch64-syscall",
        run_cross_call_syscall_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_break_direction("aarch64-calls-riscv-break",
        run_cross_call_break_aarch64_to_riscv,
        UINT64_C(0x4c000001) | ((uint64_t) POLY_MODE_RAW_RISCV << 8)) < 0)
    return -1;
  if (check_cross_call_break_direction("riscv-calls-aarch64-break",
        run_cross_call_break_riscv_to_aarch64,
        UINT64_C(0x4c000001) | ((uint64_t) POLY_MODE_RAW_AARCH64 << 8)) < 0)
    return -1;
  if (check_cross_call_import_direction("aarch64-calls-riscv-import",
        run_cross_call_import_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_import_direction("riscv-calls-aarch64-import",
        run_cross_call_import_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_string_direction("aarch64-calls-riscv-direct-x86",
        run_cross_call_direct_x86_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_string_direction("riscv-calls-aarch64-direct-x86",
        run_cross_call_direct_x86_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_direct_x86_memcmp_direction("aarch64-calls-riscv-direct-x86-memcmp",
        run_cross_call_direct_x86_memcmp_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_direct_x86_memcmp_direction("riscv-calls-aarch64-direct-x86-memcmp",
        run_cross_call_direct_x86_memcmp_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_direct_x86_memops_direction("aarch64-calls-riscv-direct-x86-memops",
        run_cross_call_direct_x86_memops_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_direct_x86_memops_direction("riscv-calls-aarch64-direct-x86-memops",
        run_cross_call_direct_x86_memops_riscv_to_aarch64) < 0)
    return -1;
  return 0;
}

int main(void) {
  puts("POLYBENCH: start");
  if (check_polybench_contract() < 0)
    return 1;
  if (setup_polybench_signature_slots() < 0)
    return 1;
  install_polybench_trap_vector();
  if (check_loop("aarch64", POLY_ARCH_AARCH64) < 0)
    return 1;
  if (check_loop("riscv", POLY_ARCH_RISCV) < 0)
    return 1;
  if (check_loop("riscv-compressed", POLY_ARCH_RISCV_COMPRESSED) < 0)
    return 1;
  if (check_mixed() < 0)
    return 1;
  if (check_cross_calls() < 0)
    return 1;
  puts("POLYBENCH_OK");
  return 0;
}
