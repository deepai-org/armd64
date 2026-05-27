#ifndef POLYCPUID_H
#define POLYCPUID_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
  POLY_MODE_X86 = 0,
  POLY_MODE_RAW_AARCH64 = 3,
  POLY_MODE_RAW_RISCV = 4,
  POLY_TRAP_SYSCALL = 1,
  POLY_TRAP_BREAK = 2,
  POLY_TRAP_IMPORT = 3,
  POLY_CPUID_BASE = 0x40000000,
  POLY_CPUID_MAX = 0x40000008,
  POLY_CPUID_ABI_VERSION = 1,
  POLY_AARCH64_BRK_X86_ESCAPE = 0x7fff,
  POLY_AARCH64_BRK_RISCV_SWITCH = 0x7ffe,
  POLY_AARCH64_BRK_RISCV_CALL = 0x7ffd,
  POLY_AARCH64_BRK_RISCV_CALL_COMPACT_U32_F32 = 0x7ffc,
  POLY_AARCH64_BRK_RISCV_CALL_COMPACT_F32_U32 = 0x7ffb,
  POLY_AARCH64_BRK_RISCV_CALL_FP64_STACK = 0x7ffa,
  POLY_AARCH64_BRK_TRAP_RETURN = 0x7ff9,
  POLY_RISCV_X86_ESCAPE = 0x0000000b,
  POLY_RISCV_AARCH64_SWITCH = 0x0000002b,
  POLY_RISCV_AARCH64_CALL = 0x0000005b,
  POLY_RISCV_AARCH64_CALL_COMPACT_U32_F32 = 0x0000107b,
  POLY_RISCV_AARCH64_CALL_COMPACT_F32_U32 = 0x0000207b,
  POLY_RISCV_AARCH64_CALL_FP64_STACK = 0x0000307b,
  POLY_RISCV_TRAP_RETURN = 0x0000407b,
  POLY_IMPORT_FUNC_X86_SLOT0 = 106,
  POLY_IMPORT_FUNC_X86_SLOT1 = 107,
  POLY_IMPORT_FUNC_X86_SLOT2 = 108,
  POLY_IMPORT_FUNC_X86_SLOT3 = 109,
  POLY_IMPORT_FUNC_X86_SLOT4 = 110,
  POLY_IMPORT_FUNC_X86_SLOT5 = 111,
  POLY_IMPORT_FUNC_X86_SLOT6 = 112,
  POLY_IMPORT_FUNC_X86_SLOT7 = 113,
  POLY_IMPORT_FUNC_COUNT = 140,
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
  POLY_CPUID_FEATURE_X86_IMPORT_DESCRIPTORS = (1U << 22),
  POLY_CPUID_FEATURE_FP64_STACK_ARGS = (1U << 23),
  POLY_CPUID_FEATURE_NEUTRAL_FP64_STACK = (1U << 24),
  POLY_CPUID_FEATURE_TRAP_VECTOR = (1U << 25),
  POLY_CPUID_FEATURE_STATE_KEY = (1U << 26),
  POLY_CPUID_STATE_OVERLAP_GPRS = (1U << 0),
  POLY_CPUID_STATE_SYNTHETIC_BANKS = (1U << 1),
  POLY_CPUID_STATE_KEY_CR3 = (1U << 2),
  POLY_CPUID_STATE_KEY_FSBASE = (1U << 3),
  POLY_CPUID_STATE_KEY_STACK_REGION = (1U << 4),
  POLY_CPUID_STATE_USER_RETURN_RESTORE = (1U << 5),
  POLY_CPUID_STATE_X86_TSO = (1U << 6),
  POLY_CPUID_STATE_XSAVE_VISIBLE = (1U << 7),
  POLY_CPUID_STATE_KEY_EXPLICIT = (1U << 8),
  POLY_CPUID_STATE_TRANSITION_FRAME_32 = (1U << 9),
  POLY_CPUID_STATE_EXPLICIT_SAVE_RESTORE = (1U << 10),
  POLY_CPUID_STATE_XSAVE_ARCH_CONTRACT = (1U << 11),
  POLY_STATE_STACK_KEY_SHIFT = 23,
  POLY_STATE_XSAVE_MAGIC = 0x31594c50, /* "PLY1" */
  POLY_STATE_XSAVE_COMPONENT_NONE = 0,
  POLY_STATE_XSAVE_BYTES_NONE = 0,
  POLY_STATE_XSAVE_COMPONENT_ARCH = 11,
  POLY_STATE_XSAVE_BYTES_ARCH = 4096,
  POLY_STATE_XSAVE_ALIGN_ARCH = 64,
  POLY_STATE_XSAVE_LAYOUT_VERSION = 1,
  POLY_STATE_XSAVE_FLAG_XCR0_USER = (1U << 0),
  POLY_STATE_XSAVE_FLAG_OSXSAVE_REQUIRED = (1U << 1),
  POLY_STATE_XSAVE_FLAG_INTERRUPT_RESUME = (1U << 2),
  POLY_STATE_XSAVE_FLAG_TRAP_STATE = (1U << 3),
  POLY_STATE_XSAVE_FLAG_NO_HIDDEN_BANKS = (1U << 4),
  POLY_STATE_XSAVE_HEADER_OFFSET = 0x000,
  POLY_STATE_XSAVE_HEADER_BYTES = 0x040,
  POLY_STATE_XSAVE_TRAP_PACKET_OFFSET = 0x040,
  POLY_STATE_XSAVE_TRAP_PACKET_BYTES = 0x040,
  POLY_STATE_XSAVE_TRAP_ARGS_OFFSET = 0x080,
  POLY_STATE_XSAVE_TRAP_ARGS_BYTES = 0x030,
  POLY_STATE_XSAVE_TRANSITION_OFFSET = 0x0b0,
  POLY_STATE_XSAVE_TRANSITION_BYTES = 0x050,
  POLY_STATE_XSAVE_AARCH64_GPR_OFFSET = 0x100,
  POLY_STATE_XSAVE_AARCH64_GPR_BYTES = 0x100,
  POLY_STATE_XSAVE_AARCH64_FP_OFFSET = 0x200,
  POLY_STATE_XSAVE_AARCH64_FP_BYTES = 0x200,
  POLY_STATE_XSAVE_AARCH64_STATUS_OFFSET = 0x400,
  POLY_STATE_XSAVE_AARCH64_STATUS_BYTES = 0x080,
  POLY_STATE_XSAVE_RISCV_GPR_OFFSET = 0x480,
  POLY_STATE_XSAVE_RISCV_GPR_BYTES = 0x100,
  POLY_STATE_XSAVE_RISCV_FP_OFFSET = 0x580,
  POLY_STATE_XSAVE_RISCV_FP_BYTES = 0x200,
  POLY_STATE_XSAVE_RISCV_STATUS_OFFSET = 0x780,
  POLY_STATE_XSAVE_RISCV_STATUS_BYTES = 0x080,
  POLY_STATE_XSAVE_RESERVED_OFFSET = 0x800,
  POLY_STATE_XSAVE_RESERVED_BYTES = 0x800,
  POLY_TRAP_PACKET_LAYOUT_VERSION = 1,
  POLY_TRAP_PACKET_HEADER_BYTES = 64,
  POLY_TRAP_PACKET_ARG_COUNT = 6,
  POLY_TRAP_PACKET_FLAG_VECTOR_DELIVERY = (1U << 0),
  POLY_TRAP_PACKET_FLAG_NO_VECTOR_X86_EXCEPTIONS = (1U << 1),
  POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE = (1U << 2),
  POLY_TRAP_PACKET_FLAG_ALL_FRONTEND_HANDLERS = (1U << 3),
  POLY_TRAP_PACKET_FLAG_STATUS_OPS = (1U << 4),
  POLY_INTERRUPT_ABI_VERSION = 1,
  POLY_INTERRUPT_FLAG_RAW_CPL3_ONLY = (1U << 0),
  POLY_INTERRUPT_FLAG_STANDARD_X86_ENTRY = (1U << 1),
  POLY_INTERRUPT_FLAG_STATE_COMPONENT_SAVE = (1U << 2),
  POLY_INTERRUPT_FLAG_PRECISE_FOREIGN_PC = (1U << 3),
  POLY_INTERRUPT_FLAG_EVENT_CHECK_BETWEEN_INSNS = (1U << 4),
  POLY_INTERRUPT_RETURN_IRET64 = (1U << 0),
  POLY_INTERRUPT_RETURN_SYSRET = (1U << 1),
  POLY_INTERRUPT_RETURN_SYSEXIT = (1U << 2),
  POLY_INTERRUPT_RETURN_SIGNAL = (1U << 3),
  POLY_MEMORY_ABI_VERSION = 1,
  POLY_MEMORY_MODEL_X86_TSO = 1,
  POLY_MEMORY_FLAG_SHARED_X86_MEMORY = (1U << 0),
  POLY_MEMORY_FLAG_AARCH64_BARRIERS_NOOP = (1U << 1),
  POLY_MEMORY_FLAG_RISCV_FENCES_NOOP = (1U << 2),
  POLY_MEMORY_FLAG_ATOMICS_COHERENT = (1U << 3),
  POLY_MEMORY_FLAG_NO_WEAK_REORDERING = (1U << 4),
  POLY_TRANSITION_ABI_VERSION = 1,
  POLY_TRANSITION_FLAG_DECODED_X86_OPCODES = (1U << 0),
  POLY_TRANSITION_FLAG_NATIVE_RAW_ESCAPES = (1U << 1),
  POLY_TRANSITION_FLAG_PIPELINE_FLUSH = (1U << 2),
  POLY_TRANSITION_FLAG_BLOCK_BOUNDARY = (1U << 3),
  POLY_TRANSITION_FLAG_PRECISE_NEXT_PC = (1U << 4),
  POLY_TRANSITION_FLAG_FIXED_RAW_WIDTH = (1U << 5),
  POLY_TRANSITION_FLAG_NEUTRAL_FOREIGN = (1U << 6),
  POLY_TRANSITION_FLAG_NATIVE_RETURN_COOKIE = (1U << 7),
  POLY_TRANSITION_FLAG_TRAP_RETURN = (1U << 8),
  POLY_TRANSITION_AARCH64_ALIGN = 4,
  POLY_TRANSITION_RISCV_ALIGN = 2
};

static const uint64_t POLY_IMPORT_CALL_BASE = 0xffffffffffffe000ULL;

struct poly_cpuid_regs {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

struct poly_u128 {
  uint64_t lo;
  uint64_t hi;
};

struct poly_xsave_header {
  uint32_t magic;
  uint16_t layout_version;
  uint16_t header_bytes;
  uint32_t total_bytes;
  uint32_t current_mode;
  uint64_t flags;
  uint64_t foreign_pc;
  uint64_t foreign_tls_base;
  uint64_t trap_vector_pc;
  uint32_t trap_vector_mode;
  uint32_t reserved0;
  uint64_t reserved1;
};

struct poly_trap_packet {
  uint32_t reason;
  uint32_t source_mode;
  uint64_t number;
  uint64_t selector;
  uint64_t trap_pc;
  uint64_t resume_pc;
  uint64_t flags;
  uint64_t reserved[2];
};

struct poly_transition_frame {
  uint64_t return_pc;
  uint32_t caller_mode;
  uint32_t target_mode;
  uint16_t abi_kind;
  uint16_t flags;
  uint32_t reserved0;
  uint64_t cookie;
};

struct poly_xsave_transition_area {
  struct poly_transition_frame active;
  uint8_t reserved[POLY_STATE_XSAVE_TRANSITION_BYTES -
    sizeof(struct poly_transition_frame)];
};

struct poly_aarch64_status_state {
  uint64_t nzcv;
  uint64_t fpcr;
  uint64_t fpsr;
  uint64_t reservation_addr;
  uint64_t reservation_value;
  uint64_t reserved[11];
};

struct poly_riscv_status_state {
  uint64_t fcsr;
  uint64_t reservation_addr;
  uint64_t reservation_value;
  uint64_t reserved[13];
};

struct poly_xsave_state {
  struct poly_xsave_header header;
  struct poly_trap_packet trap;
  uint64_t trap_args[POLY_TRAP_PACKET_ARG_COUNT];
  struct poly_xsave_transition_area transition;
  uint64_t aarch64_gpr[32];
  struct poly_u128 aarch64_fp[32];
  struct poly_aarch64_status_state aarch64_status;
  uint64_t riscv_gpr[32];
  struct poly_u128 riscv_fp[32];
  struct poly_riscv_status_state riscv_status;
  uint8_t reserved[POLY_STATE_XSAVE_RESERVED_BYTES];
};

#define POLY_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

POLY_STATIC_ASSERT(sizeof(struct poly_u128) == 16,
  "poly_u128 must be 16 bytes");
POLY_STATIC_ASSERT(sizeof(struct poly_xsave_header) ==
  POLY_STATE_XSAVE_HEADER_BYTES,
  "poly XSAVE header size must match CPUID contract");
POLY_STATIC_ASSERT(sizeof(struct poly_trap_packet) ==
  POLY_STATE_XSAVE_TRAP_PACKET_BYTES,
  "poly trap packet size must match CPUID contract");
POLY_STATIC_ASSERT(sizeof(struct poly_transition_frame) == 32,
  "poly transition frame must remain 32 bytes");
POLY_STATIC_ASSERT(sizeof(struct poly_xsave_transition_area) ==
  POLY_STATE_XSAVE_TRANSITION_BYTES,
  "poly transition area size must match XSAVE layout");
POLY_STATIC_ASSERT(sizeof(struct poly_aarch64_status_state) ==
  POLY_STATE_XSAVE_AARCH64_STATUS_BYTES,
  "poly AArch64 status area size must match XSAVE layout");
POLY_STATIC_ASSERT(sizeof(struct poly_riscv_status_state) ==
  POLY_STATE_XSAVE_RISCV_STATUS_BYTES,
  "poly RISC-V status area size must match XSAVE layout");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, header) ==
  POLY_STATE_XSAVE_HEADER_OFFSET,
  "poly XSAVE header offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, trap) ==
  POLY_STATE_XSAVE_TRAP_PACKET_OFFSET,
  "poly trap packet offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, trap_args) ==
  POLY_STATE_XSAVE_TRAP_ARGS_OFFSET,
  "poly trap args offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, transition) ==
  POLY_STATE_XSAVE_TRANSITION_OFFSET,
  "poly transition area offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, aarch64_gpr) ==
  POLY_STATE_XSAVE_AARCH64_GPR_OFFSET,
  "poly AArch64 GPR offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, aarch64_fp) ==
  POLY_STATE_XSAVE_AARCH64_FP_OFFSET,
  "poly AArch64 FP offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, aarch64_status) ==
  POLY_STATE_XSAVE_AARCH64_STATUS_OFFSET,
  "poly AArch64 status offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, riscv_gpr) ==
  POLY_STATE_XSAVE_RISCV_GPR_OFFSET,
  "poly RISC-V GPR offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, riscv_fp) ==
  POLY_STATE_XSAVE_RISCV_FP_OFFSET,
  "poly RISC-V FP offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, riscv_status) ==
  POLY_STATE_XSAVE_RISCV_STATUS_OFFSET,
  "poly RISC-V status offset drifted");
POLY_STATIC_ASSERT(offsetof(struct poly_xsave_state, reserved) ==
  POLY_STATE_XSAVE_RESERVED_OFFSET,
  "poly reserved area offset drifted");
POLY_STATIC_ASSERT(sizeof(struct poly_xsave_state) ==
  POLY_STATE_XSAVE_BYTES_ARCH,
  "poly XSAVE area size must match CPUID contract");

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
  uint32_t mask = POLY_CPUID_FEATURE_RAW_AARCH64 |
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
    POLY_CPUID_FEATURE_X86_IMPORT_DESCRIPTORS |
    POLY_CPUID_FEATURE_FP64_STACK_ARGS |
    POLY_CPUID_FEATURE_NEUTRAL_FP64_STACK |
    POLY_CPUID_FEATURE_TRAP_VECTOR |
    POLY_CPUID_FEATURE_STATE_KEY;
  return mask;
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
  regs.ebx = POLY_IMPORT_FUNC_X86_SLOT7 - POLY_IMPORT_FUNC_X86_SLOT0 + 1;
  regs.ecx = POLY_IMPORT_X86_DESCRIPTOR_SIZE;
  regs.edx = POLY_IMPORT_CALL_STRIDE;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_escape_leaf3(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_AARCH64_BRK_RISCV_CALL_FP64_STACK;
  regs.ebx = POLY_RISCV_AARCH64_CALL_FP64_STACK;
  regs.ecx = 0;
  regs.edx = 0;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_escape_leaf4(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_AARCH64_BRK_TRAP_RETURN;
  regs.ebx = POLY_RISCV_TRAP_RETURN;
  regs.ecx = 0x63;
  regs.edx = 0x64;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_escape_leaf5(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_IMPORT_FUNC_COUNT;
  regs.ebx = (uint32_t) POLY_IMPORT_CALL_BASE;
  regs.ecx = (uint32_t) (POLY_IMPORT_CALL_BASE >> 32);
  regs.edx = POLY_IMPORT_CALL_STRIDE;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_state_leaf(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_CPUID_STATE_OVERLAP_GPRS |
    POLY_CPUID_STATE_SYNTHETIC_BANKS |
    POLY_CPUID_STATE_KEY_CR3 |
    POLY_CPUID_STATE_KEY_FSBASE |
    POLY_CPUID_STATE_KEY_STACK_REGION |
    POLY_CPUID_STATE_USER_RETURN_RESTORE |
    POLY_CPUID_STATE_X86_TSO |
    POLY_CPUID_STATE_KEY_EXPLICIT |
    POLY_CPUID_STATE_TRANSITION_FRAME_32 |
    POLY_CPUID_STATE_EXPLICIT_SAVE_RESTORE |
    POLY_CPUID_STATE_XSAVE_ARCH_CONTRACT;
  regs.ebx = POLY_STATE_STACK_KEY_SHIFT;
  regs.ecx = POLY_STATE_XSAVE_COMPONENT_NONE;
  regs.edx = POLY_STATE_XSAVE_BYTES_NONE;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_arch_state_leaf(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_STATE_XSAVE_COMPONENT_ARCH;
  regs.ebx = (uint32_t) sizeof(struct poly_xsave_state);
  regs.ecx = POLY_STATE_XSAVE_LAYOUT_VERSION |
    (POLY_STATE_XSAVE_ALIGN_ARCH << 16);
  regs.edx = POLY_STATE_XSAVE_FLAG_XCR0_USER |
    POLY_STATE_XSAVE_FLAG_OSXSAVE_REQUIRED |
    POLY_STATE_XSAVE_FLAG_INTERRUPT_RESUME |
    POLY_STATE_XSAVE_FLAG_TRAP_STATE |
    POLY_STATE_XSAVE_FLAG_NO_HIDDEN_BANKS;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_trap_leaf(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_TRAP_PACKET_LAYOUT_VERSION;
  regs.ebx = (uint32_t) sizeof(struct poly_trap_packet);
  regs.ecx = (uint32_t) (sizeof(((struct poly_xsave_state *) 0)->trap_args) /
    sizeof(((struct poly_xsave_state *) 0)->trap_args[0]));
  regs.edx = POLY_TRAP_PACKET_FLAG_VECTOR_DELIVERY |
    POLY_TRAP_PACKET_FLAG_NO_VECTOR_X86_EXCEPTIONS |
    POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE |
    POLY_TRAP_PACKET_FLAG_ALL_FRONTEND_HANDLERS |
    POLY_TRAP_PACKET_FLAG_STATUS_OPS;
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_interrupt_leaf(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_INTERRUPT_ABI_VERSION;
  regs.ebx = POLY_INTERRUPT_FLAG_RAW_CPL3_ONLY |
    POLY_INTERRUPT_FLAG_STANDARD_X86_ENTRY |
    POLY_INTERRUPT_FLAG_STATE_COMPONENT_SAVE |
    POLY_INTERRUPT_FLAG_PRECISE_FOREIGN_PC |
    POLY_INTERRUPT_FLAG_EVENT_CHECK_BETWEEN_INSNS;
  regs.ecx = POLY_INTERRUPT_RETURN_IRET64 |
    POLY_INTERRUPT_RETURN_SYSRET |
    POLY_INTERRUPT_RETURN_SYSEXIT |
    POLY_INTERRUPT_RETURN_SIGNAL;
  regs.edx = (1U << POLY_MODE_RAW_AARCH64) |
    (1U << POLY_MODE_RAW_RISCV);
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_memory_leaf(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_MEMORY_ABI_VERSION;
  regs.ebx = POLY_MEMORY_MODEL_X86_TSO;
  regs.ecx = POLY_MEMORY_FLAG_SHARED_X86_MEMORY |
    POLY_MEMORY_FLAG_AARCH64_BARRIERS_NOOP |
    POLY_MEMORY_FLAG_RISCV_FENCES_NOOP |
    POLY_MEMORY_FLAG_ATOMICS_COHERENT |
    POLY_MEMORY_FLAG_NO_WEAK_REORDERING;
  regs.edx = (1U << POLY_MODE_RAW_AARCH64) |
    (1U << POLY_MODE_RAW_RISCV);
  return regs;
}

static inline struct poly_cpuid_regs poly_cpuid_expected_transition_leaf(void) {
  struct poly_cpuid_regs regs;
  regs.eax = POLY_TRANSITION_ABI_VERSION;
  regs.ebx = POLY_TRANSITION_FLAG_DECODED_X86_OPCODES |
    POLY_TRANSITION_FLAG_NATIVE_RAW_ESCAPES |
    POLY_TRANSITION_FLAG_PIPELINE_FLUSH |
    POLY_TRANSITION_FLAG_BLOCK_BOUNDARY |
    POLY_TRANSITION_FLAG_PRECISE_NEXT_PC |
    POLY_TRANSITION_FLAG_FIXED_RAW_WIDTH |
    POLY_TRANSITION_FLAG_NEUTRAL_FOREIGN |
    POLY_TRANSITION_FLAG_NATIVE_RETURN_COOKIE |
    POLY_TRANSITION_FLAG_TRAP_RETURN;
  regs.ecx = POLY_TRANSITION_AARCH64_ALIGN |
    (POLY_TRANSITION_RISCV_ALIGN << 16);
  regs.edx = poly_cpuid_expected_mode_mask();
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
