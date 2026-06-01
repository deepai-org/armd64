#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/polycpuid.h"

#define POLY_OP_EXIT \
  "movq %%r15, %%r11\n" \
  "xorl %%r15d, %%r15d\n" \
  POLY_X86_CTRL_PENTER_MODE_ASM \
  "movq %%r11, %%r15\n"
#define POLY_OP_ENTER_A64 \
  "movl $1, %%r15d\n" \
  ".balign 4, 0x90\n" \
  POLY_X86_CTRL_PENTER_MODE_ASM
#define POLY_OP_ENTER_RV64 \
  "movl $2, %%r15d\n" \
  ".balign 4, 0x90\n" \
  POLY_X86_CTRL_PENTER_MODE_ASM
#define POLY_OP_ENTER_MODE POLY_X86_CTRL_PENTER_MODE_ASM
#define POLY_OP_SWITCH_MODE POLY_X86_CTRL_PSWITCH_MODE_ASM
#define POLY_OP_LANDING POLY_X86_CTRL_LANDING_ASM
#define POLY_OP_PCALL_A64 \
  "movq %%r10, %%rbx\n" \
  "movl $1, %%r15d\n" \
  POLY_X86_CTRL_PCALL_SIG_IMM_X86_SYSV_REGS_ASM \
  ".balign 4, 0x90\n"
#define POLY_OP_PCALL_RV64 \
  "movq %%r10, %%rbx\n" \
  "movl $2, %%r15d\n" \
  POLY_X86_CTRL_PCALL_SIG_IMM_X86_SYSV_REGS_ASM \
  ".balign 4, 0x90\n"
#define POLY_OP_PCALL_SIG_MODE \
  POLY_X86_CTRL_PCALL_SIG_MODE_ASM \
  ".balign 4, 0x90\n"
#define POLY_OP_PCALL_SIG_A64 \
  "movl $1, %%r15d\n" \
  POLY_OP_PCALL_SIG_MODE
#define POLY_OP_PCALL_SIG_RV64 \
  "movl $2, %%r15d\n" \
  POLY_OP_PCALL_SIG_MODE
#define POLY_OP_PCALL_SIG_IMM_SLOT3 \
  POLY_X86_CTRL_PCALL_SIG_IMM_NATIVE_REGS_ASM \
  ".balign 4, 0x90\n"
#define POLY_OP_TRAP_VECTOR_SET POLY_X86_CTRL_TRAP_VECTOR_SET_ASM
#define POLY_OP_TRAP_VECTOR_GET POLY_X86_CTRL_TRAP_VECTOR_GET_ASM
#define POLY_OP_TRAP_VECTOR_MODE_SET POLY_X86_CTRL_TRAP_VECTOR_MODE_SET_ASM
#define POLY_OP_TRAP_VECTOR_MODE_GET POLY_X86_CTRL_TRAP_VECTOR_MODE_GET_ASM
#define POLY_OP_TRAP_RETURN POLY_X86_CTRL_TRAP_RETURN_ASM
#define POLY_OP_STATE_KEY_SET POLY_X86_CTRL_STATE_KEY_SET_ASM
#define POLY_OP_STATE_KEY_GET POLY_X86_CTRL_STATE_KEY_GET_ASM
#define POLY_OP_STATE_EXPORT POLY_X86_CTRL_STATE_EXPORT_ASM
#define POLY_OP_STATE_IMPORT POLY_X86_CTRL_STATE_IMPORT_ASM
#define POLY_OP_ABI_SIGNATURE_SET POLY_X86_CTRL_ABI_SIGNATURE_SET_ASM
#define POLY_OP_ABI_SIGNATURE_GET POLY_X86_CTRL_ABI_SIGNATURE_GET_ASM
#define POLY_OP_MONITOR_PACKET_SET POLY_X86_CTRL_MONITOR_PACKET_SET_ASM
#define POLY_OP_MONITOR_PACKET_GET POLY_X86_CTRL_MONITOR_PACKET_GET_ASM
#define POLY_OP_LANDING_POLICY_SET POLY_X86_CTRL_LANDING_POLICY_SET_ASM
#define POLY_OP_LANDING_POLICY_GET POLY_X86_CTRL_LANDING_POLICY_GET_ASM
#define POLY_ABI_GPR_CLOBBERS \
  "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r15"
#define POLY_ABI_GPR_CLOBBERS_NO_RAX \
  "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r15"
#define POLY_ABI_GPR_CLOBBERS_NO_RAX_RDI \
  "rcx", "rdx", "rsi", "r8", "r9", "r15"
#define POLY_ABI_GPR_CLOBBERS_NO_RAX_RDX \
  "rcx", "rsi", "rdi", "r8", "r9", "r15"
#define POLY_ABI_FP_CLOBBERS \
  "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#define POLY_ERR_INVAL ((uint64_t) -22)
#define POLY_CROSS_RETURN_COOKIE_VALUE 0xffffffffffffd000ULL
#define POLYPROBE_NEUTRAL_SWITCH_DELTA 3ULL
#define POLYPROBE_NEUTRAL_CALL_DELTA 4ULL
#define POLYPROBE_INVALID_AARCH64_BRANCH_TARGET 2ULL
#define POLYPROBE_INVALID_RISCV_BRANCH_TARGET 0x0100000000000000ULL
#define POLYPROBE_AARCH64_PCALL_SLOT3_INSN 0xd5032a7fULL
#define POLYPROBE_AARCH64_PCALL_GENERIC_INSN 0xd5032f3fULL
#define POLYPROBE_RISCV_PCALL_SLOT3_INSN 0x4600700bULL
#define POLYPROBE_RISCV_PCALL_GENERIC_INSN 0x1200700bULL

static struct poly_xsave_state polyprobe_state __attribute__((aligned(64)));
static uint32_t polyprobe_native_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS;
static uint32_t polyprobe_fp64_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_FP64;

struct polyprobe_monitor_packet {
  struct poly_trap_packet trap;
  uint64_t args[POLY_TRAP_PACKET_ARG_COUNT];
};

static const struct polyprobe_monitor_packet *polyprobe_current_monitor_packet;
static const uint64_t polyprobe_aarch64_trap_args[POLY_TRAP_PACKET_ARG_COUNT] =
  {77, 78, 79, 80, 81, 82, 88, 99};
static const uint64_t polyprobe_riscv_import_trap_args[POLY_TRAP_PACKET_ARG_COUNT] =
  {177, 178, 179, 180, 181, 182, 183, 184};
static const uint64_t polyprobe_riscv_syscall_args[POLY_TRAP_PACKET_ARG_COUNT] =
  {77, 78, 79, 80, 81, 82, 88, 172};

static int polyprobe_check_cpuid_regs(const char *label, uint32_t leaf,
    uint32_t subleaf, struct poly_cpuid_regs expected) {
  struct poly_cpuid_regs got = poly_read_cpuid(leaf, subleaf);
  if (got.eax != expected.eax || got.ebx != expected.ebx ||
      got.ecx != expected.ecx || got.edx != expected.edx) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: %s mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      label, got.eax, got.ebx, got.ecx, got.edx);
    return 1;
  }
  return 0;
}

static int polyprobe_trap_args_equal(
    const uint64_t got[POLY_TRAP_PACKET_ARG_COUNT],
    const uint64_t expected[POLY_TRAP_PACKET_ARG_COUNT]) {
  for (unsigned n = 0; n < POLY_TRAP_PACKET_ARG_COUNT; n++) {
    if (got[n] != expected[n])
      return 0;
  }
  return 1;
}

static int expect_monitor_packet_args(const char *label,
    const struct polyprobe_monitor_packet *packet,
    const uint64_t expected[POLY_TRAP_PACKET_ARG_COUNT]) {
  if (polyprobe_trap_args_equal(packet->args, expected))
    return 0;
  fprintf(stderr,
    "POLY_PROBE_FAIL: monitor packet %s args mismatch got=[%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu] expected=[%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu]\n",
    label,
    (unsigned long long) packet->args[0],
    (unsigned long long) packet->args[1],
    (unsigned long long) packet->args[2],
    (unsigned long long) packet->args[3],
    (unsigned long long) packet->args[4],
    (unsigned long long) packet->args[5],
    (unsigned long long) packet->args[6],
    (unsigned long long) packet->args[7],
    (unsigned long long) expected[0],
    (unsigned long long) expected[1],
    (unsigned long long) expected[2],
    (unsigned long long) expected[3],
    (unsigned long long) expected[4],
    (unsigned long long) expected[5],
    (unsigned long long) expected[6],
    (unsigned long long) expected[7]);
  return 1;
}

static inline void poly_mode_x86(void) {
  asm volatile(POLY_OP_EXIT ::: "r11", "memory");
}
static inline void poly_switch_count_status(void) {
  asm volatile(POLY_X86_CTRL_SWITCH_COUNT_STATUS_ASM ::: "memory");
}
static inline void poly_foreign_insn_count_status(void) {
  asm volatile(POLY_X86_CTRL_FOREIGN_INSN_COUNT_STATUS_ASM ::: "memory");
}
static inline void poly_foreign_syscall_count_status(void) {
  asm volatile(POLY_X86_CTRL_FOREIGN_SYSCALL_COUNT_STATUS_ASM ::: "memory");
}
static inline void poly_foreign_break_count_status(void) {
  asm volatile(POLY_X86_CTRL_FOREIGN_BREAK_COUNT_STATUS_ASM ::: "memory");
}

static inline void poly_trap_vector_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline uint64_t poly_trap_vector_set_status(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET : "+a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_vector_get(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_VECTOR_GET : "=a"(value) :: "memory");
  return value;
}

static inline void poly_trap_vector_mode_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
}

static inline uint64_t poly_trap_vector_mode_set_status(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET : "+a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_vector_mode_get(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_GET : "=a"(value) :: "memory");
  return value;
}

static inline void poly_monitor_packet_set_value(uint64_t value) {
  asm volatile(POLY_OP_MONITOR_PACKET_SET :: "a"(value) : "memory");
}

static inline uint64_t poly_monitor_packet_set_status(uint64_t value) {
  asm volatile(POLY_OP_MONITOR_PACKET_SET : "+a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_monitor_packet_get(void) {
  uint64_t value;
  asm volatile(POLY_OP_MONITOR_PACKET_GET : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_state_key_set_status(uint64_t value) {
  asm volatile(POLY_OP_STATE_KEY_SET : "+a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_state_key_get(void) {
  uint64_t value;
  asm volatile(POLY_OP_STATE_KEY_GET : "=a"(value) :: "memory");
  return value;
}

static inline void poly_state_export(struct poly_xsave_state *state) {
  uint64_t rax = (uint64_t) (uintptr_t) state;
  asm volatile(POLY_OP_STATE_EXPORT : "+a"(rax) :: "memory");
}

static inline void poly_state_import(struct poly_xsave_state *state) {
  uint64_t rax = (uint64_t) (uintptr_t) state;
  asm volatile(POLY_OP_STATE_IMPORT : "+a"(rax)
      :: POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
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

static inline uint64_t poly_abi_signature_set_raw(uint64_t slot,
    uint64_t value) {
  uint64_t rax = slot;
  uint64_t rdx = value;
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

static inline uint64_t poly_landing_policy_set(uint64_t policy) {
  uint64_t rax = policy;
  asm volatile(POLY_OP_LANDING_POLICY_SET
      : "+a"(rax)
      :
      : "memory");
  return rax;
}

static inline uint64_t poly_landing_policy_get(void) {
  uint64_t rax;
  asm volatile(POLY_OP_LANDING_POLICY_GET
      : "=a"(rax)
      :
      : "memory");
  return rax;
}

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
}

static inline uint64_t read_rsp(void) {
  uint64_t value;
  asm volatile("movq %%rsp, %0" : "=r"(value));
  return value;
}

#define CHECK_POLYPROBE_SWITCH_DELTA(name, probe, expected_result, expected_delta) \
  do { \
    poly_switch_count_status(); \
    uint64_t switches_before_ = read_rax(); \
    probe(); \
    uint64_t result_ = read_rax(); \
    poly_switch_count_status(); \
    uint64_t switch_delta_ = read_rax() - switches_before_; \
    if (result_ != (uint64_t) (expected_result)) { \
      fprintf(stderr, \
        "POLY_PROBE_FAIL: %s result expected %llu got %llu\n", \
        (name), (unsigned long long) (uint64_t) (expected_result), \
        (unsigned long long) result_); \
      return 1; \
    } \
    if (switch_delta_ != (uint64_t) (expected_delta)) { \
      fprintf(stderr, \
        "POLY_PROBE_FAIL: %s switch delta expected %llu got %llu\n", \
        (name), (unsigned long long) (uint64_t) (expected_delta), \
        (unsigned long long) switch_delta_); \
      return 1; \
    } \
  } while (0)

static inline uint64_t read_xmm0_u64(void) {
  uint64_t value;
  asm volatile("movq %%xmm0, %0" : "=r"(value));
  return value;
}

static inline void write_xmm0_u64(uint64_t value) {
  asm volatile("movq %0, %%xmm0" :: "r"(value) : "xmm0", "memory");
}

static inline void write_xmm1_u64(uint64_t value) {
  asm volatile("movq %0, %%xmm1" :: "r"(value) : "xmm1", "memory");
}

static inline void write_xmm2_u64(uint64_t value) {
  asm volatile("movq %0, %%xmm2" :: "r"(value) : "xmm2", "memory");
}

struct polyprobe_u128 {
  uint64_t lo;
  uint64_t hi;
} __attribute__((aligned(16)));

static inline void write_xmm0_u128(uint64_t lo, uint64_t hi) {
  struct polyprobe_u128 value = { lo, hi };
  asm volatile("movdqu %0, %%xmm0" :: "m"(value) : "xmm0", "memory");
}

static inline void write_xmm1_u128(uint64_t lo, uint64_t hi) {
  struct polyprobe_u128 value = { lo, hi };
  asm volatile("movdqu %0, %%xmm1" :: "m"(value) : "xmm1", "memory");
}

static inline struct polyprobe_u128 read_xmm0_u128(void) {
  struct polyprobe_u128 value;
  asm volatile("movdqu %%xmm0, %0" : "=m"(value) :: "memory");
  return value;
}

static void stage(const char *msg) {
  if (write(1, msg, strlen(msg)) < 0)
    return;
  ssize_t ignored = write(1, "\n", 1);
  (void) ignored;
}

static int poly_is_raw_foreign_mode(uint64_t mode) {
  return mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV;
}

static int expect_monitor_packet_header(const char *label,
    const struct polyprobe_monitor_packet *packet, uint32_t reason,
    uint32_t source_mode, uint64_t number, uint64_t selector,
    int require_linear_resume) {
  if (packet->trap.reason != reason ||
      packet->trap.source_mode != source_mode ||
      packet->trap.number != number ||
      packet->trap.selector != selector ||
      packet->trap.trap_pc == 0 ||
      packet->trap.resume_pc == 0 ||
      packet->trap.reserved[0] != 0 ||
      packet->trap.reserved[1] != 0 ||
      (require_linear_resume &&
       packet->trap.resume_pc != packet->trap.trap_pc + 4) ||
      (packet->trap.flags & POLY_TRAP_PACKET_REQUIRED_FLAGS) !=
        POLY_TRAP_PACKET_REQUIRED_FLAGS) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: monitor packet %s mismatch reason=%u mode=%u number=%llu selector=%llu pc=0x%llx resume=0x%llx flags=0x%llx reserved=(0x%llx,0x%llx)\n",
      label, packet->trap.reason, packet->trap.source_mode,
      (unsigned long long) packet->trap.number,
      (unsigned long long) packet->trap.selector,
      (unsigned long long) packet->trap.trap_pc,
      (unsigned long long) packet->trap.resume_pc,
      (unsigned long long) packet->trap.flags,
      (unsigned long long) packet->trap.reserved[0],
      (unsigned long long) packet->trap.reserved[1]);
    return 1;
  }
  return 0;
}

static int polyprobe_monitor_packet_contract_valid(
    const struct polyprobe_monitor_packet *packet) {
  return packet->trap.resume_pc != 0 &&
    packet->trap.reserved[0] == 0 &&
    packet->trap.reserved[1] == 0 &&
    (packet->trap.flags & POLY_TRAP_PACKET_REQUIRED_FLAGS) ==
      POLY_TRAP_PACKET_REQUIRED_FLAGS;
}

static inline void polyprobe_clobber_aarch64_trap_state(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2807d13\n" // movz x19,#1000
    ".long 0xd280fa14\n" // movz x20,#2000
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void polyprobe_clobber_riscv_trap_state(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x06400913\n" // addi s2,zero,100
    ".long 0x0c800993\n" // addi s3,zero,200
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void polyprobe_clobber_aarch64_trap_fp_state(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28acf13\n" // movz x19,#0x5678
    ".long 0x9e670274\n" // fmov d20,x19
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "xmm4", "memory");
}

static inline void polyprobe_clobber_riscv_trap_fp_state(void) {
  write_xmm0_u64(0x5678);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x22a50a53\n" // fsgnj.d f20,fa0,fa0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "xmm0", "memory");
}

__attribute__((noinline, used))
uint64_t polyprobe_trap_vector_dispatch(void) {
  const struct polyprobe_monitor_packet *monitor_packet =
    polyprobe_current_monitor_packet;
  if (monitor_packet == 0)
    return (uint64_t) -38;
  if (!polyprobe_monitor_packet_contract_valid(monitor_packet))
    return (uint64_t) -38;
  const uint64_t reason = monitor_packet->trap.reason;
  const uint64_t mode = monitor_packet->trap.source_mode;
  const uint64_t number = monitor_packet->trap.number;
  const uint64_t selector = monitor_packet->trap.selector;

  if (!poly_is_raw_foreign_mode(mode))
    return (uint64_t) -38;
  if (reason == POLY_TRAP_BREAK && number == 2 &&
      mode == POLY_MODE_RAW_AARCH64) {
    polyprobe_clobber_aarch64_trap_state();
    return 0;
  }
  if (reason == POLY_TRAP_BREAK && number == 2 &&
      mode == POLY_MODE_RAW_RISCV) {
    polyprobe_clobber_riscv_trap_state();
    return 0;
  }
  if (reason == POLY_TRAP_BREAK && number == 3 &&
      mode == POLY_MODE_RAW_AARCH64) {
    polyprobe_clobber_aarch64_trap_fp_state();
    return 0;
  }
  if (reason == POLY_TRAP_BREAK && number == 3 &&
      mode == POLY_MODE_RAW_RISCV) {
    polyprobe_clobber_riscv_trap_fp_state();
    return 0;
  }
  if (reason == POLY_TRAP_SYSCALL && number == 172 &&
      ((mode == POLY_MODE_RAW_AARCH64 &&
        polyprobe_trap_args_equal(monitor_packet->args,
          polyprobe_aarch64_trap_args)) ||
       (mode == POLY_MODE_RAW_RISCV &&
        polyprobe_trap_args_equal(monitor_packet->args,
          polyprobe_riscv_syscall_args))))
    return 4242;
  if (reason == POLY_TRAP_BREAK)
    return 0x4c000000ULL | (mode << 8) | number;
  if (reason == POLY_TRAP_IMPORT && number == 8 &&
      mode == POLY_MODE_RAW_AARCH64 &&
      polyprobe_trap_args_equal(monitor_packet->args,
        polyprobe_aarch64_trap_args))
    return 5555;
  if (reason == POLY_TRAP_IMPORT && number == 8 &&
      mode == POLY_MODE_RAW_RISCV &&
      polyprobe_trap_args_equal(monitor_packet->args,
        polyprobe_riscv_import_trap_args))
    return 5555;
  if (reason == POLY_TRAP_ILLEGAL && selector == 4 &&
      mode == POLY_MODE_RAW_AARCH64 && number == 0xd61f0200ULL &&
      monitor_packet->args[0] == POLYPROBE_INVALID_AARCH64_BRANCH_TARGET)
    return 7777;
  if (reason == POLY_TRAP_ILLEGAL && selector == 4 &&
      mode == POLY_MODE_RAW_RISCV && number == 0x00050067ULL &&
      monitor_packet->args[0] == POLYPROBE_INVALID_RISCV_BRANCH_TARGET)
    return 7777;
  if (reason == POLY_TRAP_ILLEGAL && selector == 4 &&
      mode == POLY_MODE_RAW_AARCH64 &&
      (number == POLYPROBE_AARCH64_PCALL_SLOT3_INSN ||
       number == POLYPROBE_AARCH64_PCALL_GENERIC_INSN))
    return 8888;
  if (reason == POLY_TRAP_ILLEGAL && selector == 4 &&
      mode == POLY_MODE_RAW_RISCV &&
      (number == POLYPROBE_RISCV_PCALL_SLOT3_INSN ||
       number == POLYPROBE_RISCV_PCALL_GENERIC_INSN))
    return 8888;
  if (reason == POLY_TRAP_ILLEGAL && number == 0xffffffffULL &&
      selector == 4)
    return 6666;
  return (uint64_t) -38;
}

__attribute__((naked, noinline, used))
static void polyprobe_trap_vector_handler(void) {
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
    "call polyprobe_trap_vector_dispatch\n"
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

static void install_polyprobe_trap_vector(void) {
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value((uint64_t) (void *) polyprobe_trap_vector_handler);
}

static inline void raw_aarch64_arith_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800540\n" // movz x0,#42
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_arith_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01100513\n" // addi a0,zero,17
    ".long 0x00550513\n" // addi a0,a0,5
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void poly_opcode_aarch64_arith_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800540\n" // movz x0,#42
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void poly_opcode_riscv_arith_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01100513\n" // addi a0,zero,17
    ".long 0x00550513\n" // addi a0,a0,5
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_sp_probe(void) {
  asm volatile(
    "movq $19, %%rax\n"
    "movq $23, %%rdx\n"
    POLY_OP_ENTER_A64
    ".long 0xd10043ff\n" // sub sp,sp,#16
    ".long 0xa90007e0\n" // stp x0,x1,[sp]
    ".long 0xa9400fe2\n" // ldp x2,x3,[sp]
    ".long 0x8b030040\n" // add x0,x2,x3
    ".long 0x910043ff\n" // add sp,sp,#16
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_frame_pair_probe(void) {
  asm volatile(
    "movq $19, %%rax\n"
    "movq $23, %%rdx\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0003fd\n" // mov x29,x0
    ".long 0xaa0103fe\n" // mov x30,x1
    ".long 0xa9bf7bfd\n" // stp x29,x30,[sp,#-16]!
    ".long 0xaa1f03fd\n" // mov x29,xzr
    ".long 0xaa1f03fe\n" // mov x30,xzr
    ".long 0xa8c17bfd\n" // ldp x29,x30,[sp],#16
    ".long 0x8b1e03a0\n" // add x0,x29,x30
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_sp_probe(void) {
  asm volatile(
    "movq $17, %%rax\n"
    "movq $25, %%rdx\n"
    POLY_OP_ENTER_RV64
    ".long 0xff010113\n" // addi sp,sp,-16
    ".long 0x00a13023\n" // sd a0,0(sp)
    ".long 0x00b13423\n" // sd a1,8(sp)
    ".long 0x00013603\n" // ld a2,0(sp)
    ".long 0x00813683\n" // ld a3,8(sp)
    ".long 0x00d60533\n" // add a0,a2,a3
    ".long 0x01010113\n" // addi sp,sp,16
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_compressed_frame_probe(void) {
  asm volatile(
    "movq $17, %%rax\n"
    "movq $25, %%rdx\n"
    POLY_OP_ENTER_RV64
    ".short 0x717d\n" // c.addi16sp -16
    ".short 0xe02a\n" // c.sdsp a0,0(sp)
    ".short 0xe42e\n" // c.sdsp a1,8(sp)
    ".long 0x00000513\n" // addi a0,zero,0
    ".short 0x6602\n" // c.ldsp a2,0(sp)
    ".short 0x66a2\n" // c.ldsp a3,8(sp)
    ".short 0x9532\n" // c.add a0,a2
    ".short 0x9536\n" // c.add a0,a3
    ".short 0x6141\n" // c.addi16sp 16
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_wide_regs_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28000ea\n" // movz x10,#7
    ".long 0xd280046b\n" // movz x11,#35
    ".long 0x8b0b014c\n" // add x12,x10,x11
    ".long 0x8b0a0180\n" // add x0,x12,x10
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_wide_regs_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00900813\n" // addi x16,zero,9
    ".long 0x02100913\n" // addi x18,zero,33
    ".long 0x012809b3\n" // add x19,x16,x18
    ".long 0x01098533\n" // add a0,x19,x16
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_state_seed_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2824693\n" // movz x19,#0x1234
    ".long 0x9e670268\n" // fmov d8,x19
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_state_seed_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x32100993\n" // addi x19,zero,0x321
    ".long 0xf2098953\n" // fmv.d.x f18,x19
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_status_seed_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2a01801\n" // movz x1,#0xc0,lsl #16
    ".long 0xd51b4401\n" // msr fpcr,x1
    ".long 0xd2800242\n" // movz x2,#0x12
    ".long 0xd51b4422\n" // msr fpsr,x2
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_status_seed_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x07500293\n" // addi t0,zero,0x75
    ".long 0x00329073\n" // csrw fcsr,t0
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline uint64_t penter_aarch64_tls_probe(uint64_t tls_base) {
  uint64_t result = 0;
  asm volatile(
    "pushq %%r13\n"
    "movq %1, %%r13\n"
    POLY_OP_ENTER_A64
    ".long 0xd53bd040\n" // mrs x0,tpidr_el0
    ".long 0xd5032e1f\n"
    "popq %%r13\n"
    : "=a"(result)
    : "m"(tls_base)
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return result;
}

static inline uint64_t penter_riscv_tls_probe(uint64_t tls_base) {
  uint64_t result = 0;
  asm volatile(
    "pushq %%r13\n"
    "movq %1, %%r13\n"
    POLY_OP_ENTER_RV64
    ".long 0x00020513\n" // mv a0,tp
    ".long 0x0000700b\n"
    "popq %%r13\n"
    : "=a"(result)
    : "m"(tls_base)
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return result;
}

static inline void raw_aarch64_state_gpr_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa1303e0\n" // mov x0,x19
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_state_fp_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x9e660100\n" // fmov x0,d8
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_state_gpr_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00098533\n" // add a0,x19,zero
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_state_fp_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0xe2090553\n" // fmv.x.d a0,f18
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_status_sum_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd53b4400\n" // mrs x0,fpcr
    ".long 0xd53b4421\n" // mrs x1,fpsr
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_status_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00302573\n" // csrr a0,fcsr
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_imm_regs_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28000ea\n" // movz x10,#7
    ".long 0x9100154d\n" // add x13,x10,#5
    ".long 0xd10021a0\n" // sub x0,x13,#8
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_imm_regs_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0xffd00293\n" // addi x5,zero,-3
    ".long 0x03628513\n" // addi a0,x5,54
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void generic_enter_aarch64_probe(void) {
  asm volatile(
    "pushq %%r15\n"
    "movq %0, %%r15\n"
    ".balign 4, 0x90\n"
    POLY_OP_ENTER_MODE
    ".long 0xd28002a0\n" // movz x0,#21
    ".long 0xd5032e1f\n"
    "popq %%r15\n"
    :
    : "i"(POLY_FRONTEND_AARCH64)
    : POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void generic_enter_riscv_probe(void) {
  asm volatile(
    "pushq %%r15\n"
    "movq %0, %%r15\n"
    ".balign 4, 0x90\n"
    POLY_OP_ENTER_MODE
    ".long 0x01500513\n" // addi a0,zero,21
    ".long 0x0000700b\n"
    "popq %%r15\n"
    :
    : "i"(POLY_FRONTEND_RISCV)
    : POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void generic_switch_aarch64_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "movq %0, %%r15\n"
    POLY_OP_SWITCH_MODE
    "jmp 3f\n"
    ".balign 4, 0x90\n"
    "1:\n"
    ".long 0xd2800420\n" // movz x0,#33
    ".long 0xd5032e1f\n"
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    "jmp 4f\n"
    "3:\n"
    "movq $0xbad, %%rax\n"
    "4:\n"
    :
    : "i"(POLY_FRONTEND_AARCH64)
    : POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void generic_switch_riscv_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "movq %0, %%r15\n"
    POLY_OP_SWITCH_MODE
    "jmp 3f\n"
    ".balign 2, 0x90\n"
    "1:\n"
    ".long 0x02100513\n" // addi a0,zero,33
    ".long 0x0000700b\n"
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    "jmp 4f\n"
    "3:\n"
    "movq $0xbad, %%rax\n"
    "4:\n"
    :
    : "i"(POLY_FRONTEND_RISCV)
    : POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void landing_pad_probe(void) {
  asm volatile(
    POLY_OP_LANDING
    "movq $17, %%rax\n"
    POLY_OP_ENTER_A64
    ".long 0xd5032f7f\n" // aarch64 landing pad
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    POLY_OP_ENTER_RV64
    ".long 0x1600700b\n" // riscv landing pad
    ".long 0x00150513\n" // addi a0,a0,1
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void aarch64_abi_signature_control_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28000a0\n" // movz x0,#5
    ".long 0xd2800041\n" // movz x1,#2 (x86 SysV register-only)
    ".long 0xd5032f9f\n" // aarch64 ABI signature set
    ".long 0xd28000a0\n" // movz x0,#5
    ".long 0xd5032fbf\n" // aarch64 ABI signature get
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void aarch64_abi_signature_invalid_slot_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long %c0\n" // movz x0,#POLY_ABI_SIGNATURE_SLOT_COUNT
    ".long 0xd2800001\n" // movz x1,#0
    ".long 0xd5032f9f\n" // aarch64 ABI signature set
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    :
    : "i"(0xd2800000U | (POLY_ABI_SIGNATURE_SLOT_COUNT << 5))
    : POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void aarch64_abi_signature_invalid_kind_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28000a0\n" // movz x0,#5
    ".long %c0\n" // movz x1,#POLY_ABI_SIGNATURE_KIND_INVALID_TEST
    ".long 0xd5032f9f\n" // aarch64 ABI signature set
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    :
    : "i"(0xd2800001U |
        (POLY_ABI_SIGNATURE_KIND_INVALID_TEST << 5))
    : POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void riscv_abi_signature_control_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00600513\n" // addi a0,zero,6
    ".long 0x00000593\n" // addi a1,zero,0 (exchange)
    ".long 0x1800700b\n" // riscv ABI signature set
    ".long 0x00600513\n" // addi a0,zero,6
    ".long 0x1a00700b\n" // riscv ABI signature get
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void riscv_abi_signature_invalid_slot_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long %c0\n" // addi a0,zero,POLY_ABI_SIGNATURE_SLOT_COUNT
    ".long 0x00000593\n" // addi a1,zero,0
    ".long 0x1800700b\n" // riscv ABI signature set
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    :
    : "i"((POLY_ABI_SIGNATURE_SLOT_COUNT << 20) | 0x00000513U)
    : POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void riscv_abi_signature_invalid_kind_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00600513\n" // addi a0,zero,6
    ".long %c0\n" // addi a1,zero,POLY_ABI_SIGNATURE_KIND_INVALID_TEST
    ".long 0x1800700b\n" // riscv ABI signature set
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    :
    : "i"((POLY_ABI_SIGNATURE_KIND_INVALID_TEST << 20) |
        0x00000593U)
    : POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline uint64_t aarch64_landing_policy_control_probe(uint64_t policy) {
  uint64_t rax = policy;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032fdf\n" // aarch64 landing policy set
    ".long 0xd5032fff\n" // aarch64 landing policy get
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t aarch64_landing_policy_invalid_probe(uint64_t policy) {
  uint64_t rax = policy;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032fdf\n" // aarch64 landing policy set
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t riscv_landing_policy_control_probe(uint64_t policy) {
  uint64_t rax = policy;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3c00700b\n" // riscv landing policy set
    ".long 0x3e00700b\n" // riscv landing policy get
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t riscv_landing_policy_invalid_probe(uint64_t policy) {
  uint64_t rax = policy;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3c00700b\n" // riscv landing policy set
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t aarch64_state_key_control_probe(uint64_t key) {
  uint64_t rax = key;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032ddf\n" // aarch64 state key set
    ".long 0xd5032dff\n" // aarch64 state key get
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t riscv_state_key_control_probe(uint64_t key) {
  uint64_t rax = key;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x1c00700b\n" // riscv state key set
    ".long 0x1e00700b\n" // riscv state key get
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t aarch64_foreign_control_plane_probe(uint64_t vector,
    uint64_t packet) {
  uint64_t rax = vector;
  uint64_t rdx = packet;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032d1f\n" // aarch64 trap vector set, x0=vector
    ".long 0xd2800040\n" // movz x0,#2 (RISC-V mode)
    ".long 0xd5032d5f\n" // aarch64 trap vector mode set
    ".long 0xaa0103e0\n" // mov x0,x1 (packet)
    ".long 0xd5032d9f\n" // aarch64 monitor packet set
    ".long 0xd5032d3f\n" // aarch64 trap vector get
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(rax), "+d"(rdx)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDX, "memory");
  return rax;
}

static inline uint64_t aarch64_foreign_trap_vector_invalid_probe(
    uint64_t value) {
  uint64_t rax = 0;
  uint64_t rdx = value;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0103e0\n" // mov x0,x1 (candidate vector)
    ".long 0xd5032d1f\n" // aarch64 trap vector set
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(rax), "+d"(rdx)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDX, "memory");
  return rax;
}

static inline uint64_t aarch64_foreign_monitor_packet_invalid_probe(
    uint64_t value) {
  uint64_t rax = 0;
  uint64_t rdx = value;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0103e0\n" // mov x0,x1 (candidate packet)
    ".long 0xd5032d9f\n" // aarch64 monitor packet set
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(rax), "+d"(rdx)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDX, "memory");
  return rax;
}

static inline uint64_t aarch64_foreign_trap_vector_mode_invalid_probe(
    uint64_t value) {
  uint64_t rax = value;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032d5f\n" // aarch64 trap vector mode set
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t riscv_foreign_control_plane_probe(uint64_t vector,
    uint64_t packet) {
  uint64_t rax = vector;
  uint64_t rdx = packet;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3000700b\n" // riscv trap vector set, a0=vector
    ".long 0x00100513\n" // addi a0,zero,1 (AArch64 mode)
    ".long 0x3400700b\n" // riscv trap vector mode set
    ".long 0x00058513\n" // mv a0,a1 (packet)
    ".long 0x3800700b\n" // riscv monitor packet set
    ".long 0x3200700b\n" // riscv trap vector get
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(rax), "+d"(rdx)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDX, "memory");
  return rax;
}

static inline uint64_t riscv_foreign_trap_vector_invalid_probe(
    uint64_t value) {
  uint64_t rax = 0;
  uint64_t rdx = value;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00058513\n" // mv a0,a1 (candidate vector)
    ".long 0x3000700b\n" // riscv trap vector set
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(rax), "+d"(rdx)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDX, "memory");
  return rax;
}

static inline uint64_t riscv_foreign_monitor_packet_invalid_probe(
    uint64_t value) {
  uint64_t rax = 0;
  uint64_t rdx = value;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00058513\n" // mv a0,a1 (candidate packet)
    ".long 0x3800700b\n" // riscv monitor packet set
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(rax), "+d"(rdx)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDX, "memory");
  return rax;
}

static inline uint64_t riscv_foreign_trap_vector_mode_invalid_probe(
    uint64_t value) {
  uint64_t rax = value;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3400700b\n" // riscv trap vector mode set
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t riscv_trap_to_aarch64_monitor_probe(void) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    POLY_OP_TRAP_VECTOR_SET
    "movq $1, %%rax\n"
    POLY_OP_TRAP_VECTOR_MODE_SET
    POLY_OP_ENTER_RV64
    ".long 0x04d00513\n" // addi a0,zero,77
    ".long 0x00100893\n" // addi a7,zero,1
    ".long 0x00100073\n" // ebreak
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x910190a0\n" // add x0,x5,#100 (x5 carries trap arg0)
    ".long 0xd5032edf\n" // aarch64 trap return
    "2:\n"
    : "=a"(result)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "r10", "r11", "r12", "r13", "r14",
      "memory");
  return result;
}

static inline uint64_t aarch64_trap_to_riscv_monitor_probe(void) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    POLY_OP_TRAP_VECTOR_SET
    "movq $2, %%rax\n"
    POLY_OP_TRAP_VECTOR_MODE_SET
    POLY_OP_ENTER_A64
    ".long 0xd28009a0\n" // movz x0,#77
    ".long 0xd4200020\n" // brk #1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x0c878513\n" // addi a0,a5,200 (a5 carries trap arg0)
    ".long 0x0c00700b\n" // riscv trap return
    "2:\n"
    : "=a"(result)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "r10", "r11", "r12", "r13", "r14",
      "memory");
  return result;
}

static inline void aarch64_generic_switch_riscv_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x10000070\n" // adr x16, target
    ".long 0xd2800051\n" // movz x17,#2 (RISC-V frontend)
    ".long 0xd5032f1f\n" // aarch64 generic poly switch
    ".long 0x02d00513\n" // target: addi a0,zero,45
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void aarch64_generic_switch_x86_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x10000070\n" // adr x16, target
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0xd5032f1f\n" // aarch64 generic poly switch
    "movq $57, %%rax\n" // target: ordinary x86 after frontend switch
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void riscv_generic_switch_aarch64_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00000297\n" // auipc x5,0
    ".long 0x01028293\n" // addi x5,x5,16
    ".long 0x00100313\n" // addi x6,zero,1 (AArch64 frontend)
    ".long 0x1000700b\n" // riscv generic poly switch
    ".long 0xd28005a0\n" // target: movz x0,#45
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void riscv_generic_switch_x86_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00000297\n" // auipc x5,0
    ".long 0x01028293\n" // addi x5,x5,16
    ".long 0x00000313\n" // addi x6,zero,0 (x86 frontend)
    ".long 0x1000700b\n" // riscv generic poly switch
    "movq $58, %%rax\n" // target: ordinary x86 after frontend switch
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void aarch64_generic_call_riscv_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800500\n" // movz x0,#40
    ".long 0x10000090\n" // adr x16,target
    ".long 0xd2800051\n" // movz x17,#2 (RISC-V frontend)
    ".long 0x10000092\n" // adr x18,return
    ".long 0xd5032f3f\n" // aarch64 generic poly call
    ".long 0x00550513\n" // target: addi a0,a0,5
    ".long 0x00008067\n" // riscv ret through return cookie
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void riscv_generic_call_aarch64_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x02800513\n" // addi a0,zero,40
    ".long 0x00000297\n" // auipc x5,0
    ".long 0x01828293\n" // addi x5,x5,24
    ".long 0x00100313\n" // addi x6,zero,1 (AArch64 frontend)
    ".long 0x00000397\n" // auipc x7,0
    ".long 0x01438393\n" // addi x7,x7,20
    ".long 0x1200700b\n" // riscv generic poly call
    ".long 0x91001400\n" // target: add x0,x0,#5
    ".long 0xd65f03c0\n" // aarch64 ret through return cookie
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void export_live_cross_return_state_probe(
    struct poly_xsave_state *state) {
  asm volatile(
    "leaq 1f(%%rip), %%rdx\n" // AArch64 -> RISC-V target.
    "leaq 3f(%%rip), %%rcx\n" // AArch64 return site.
    "leaq 4f(%%rip), %%rdi\n" // RISC-V -> x86 helper target.
    "leaq 2f(%%rip), %%rsi\n" // RISC-V return site after helper.
    "movq %0, %%rax\n"        // AArch64 x0 / RISC-V a0: state buffer.
    POLY_OP_ENTER_A64
    ".long 0xaa0103f0\n" // mov x16,x1 (target)
    ".long 0xaa0203f2\n" // mov x18,x2 (return)
    ".long 0xd2800051\n" // movz x17,#2 (RISC-V frontend)
    ".long 0xd5032f3f\n" // AArch64 generic PCALL
    "1:\n"
    ".long 0x00068293\n" // mv t0,a3 (x86 helper target)
    ".long 0x00070393\n" // mv t2,a4 (RISC-V helper return)
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x4600700b\n" // RISC-V PCALL_SIG_IMM slot 3
    "2:\n"
    ".long 0x04d00513\n" // addi a0,zero,77
    ".long 0x00008067\n" // ret through AArch64 cross-return cookie
    "3:\n"
    ".long 0xd5032e1f\n" // AArch64 x86 escape
    "jmp 5f\n"
    "4:\n"
    "movq %%rdi, %%rax\n"
    POLY_OP_STATE_EXPORT
    "retq\n"
    "5:\n"
    :
    : "r"(state)
    : POLY_ABI_GPR_CLOBBERS, POLY_ABI_FP_CLOBBERS, "r10", "r11", "memory");
}

static inline void export_live_reverse_cross_return_state_probe(
    struct poly_xsave_state *state) {
  asm volatile(
    "leaq 1f(%%rip), %%rdx\n" // RISC-V -> AArch64 target.
    "leaq 3f(%%rip), %%rcx\n" // RISC-V return site.
    "leaq 4f(%%rip), %%rdi\n" // AArch64 -> x86 helper target.
    "leaq 2f(%%rip), %%rsi\n" // AArch64 return site after helper.
    "movq %0, %%rax\n"        // RISC-V a0 / AArch64 x0: state buffer.
    POLY_OP_ENTER_RV64
    ".long 0x00058293\n" // mv t0,a1 (target)
    ".long 0x00060393\n" // mv t2,a2 (return)
    ".long 0x00100313\n" // addi t1,zero,1 (AArch64 frontend)
    ".long 0x1200700b\n" // RISC-V generic PCALL
    "1:\n"
    ".long 0xaa0303f0\n" // mov x16,x3 (x86 helper target)
    ".long 0xaa0403f2\n" // mov x18,x4 (AArch64 helper return)
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0xd5032a7f\n" // AArch64 PCALL_SIG_IMM slot 3
    "2:\n"
    ".long 0xd2800c60\n" // movz x0,#99
    ".long 0xd65f03c0\n" // ret through RISC-V cross-return cookie
    "3:\n"
    ".long 0x0000700b\n" // RISC-V x86 escape
    "jmp 5f\n"
    "4:\n"
    "movq %%rdi, %%rax\n"
    POLY_OP_STATE_EXPORT
    "retq\n"
    "5:\n"
    :
    : "r"(state)
    : POLY_ABI_GPR_CLOBBERS, POLY_ABI_FP_CLOBBERS, "r10", "r11", "memory");
}

static uint64_t import_live_cross_return_state_probe(
    struct poly_xsave_state *state) {
  uint64_t return_sp = read_rsp();
  uint64_t rax = (uint64_t) (uintptr_t) state;
  state->header.current_mode = POLY_MODE_RAW_RISCV;
  state->transition.active.return_pc = 0;
  state->transition.active.caller_mode = POLY_MODE_RAW_AARCH64;
  state->transition.active.target_mode = POLY_MODE_RAW_RISCV;
  state->transition.active.abi_kind = POLY_CROSS_BRIDGE_DEFAULT;
  state->transition.active.flags = 0;
  state->transition.active.cookie = return_sp;
  state->cross_return.top = 1;
  state->cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
  state->cross_return.frames[0].return_pc = 0;
  state->cross_return.frames[0].return_sp = return_sp;
  state->cross_return.frames[0].caller_mode = POLY_MODE_RAW_AARCH64;
  state->cross_return.frames[0].target_mode = POLY_MODE_RAW_RISCV;
  state->cross_return.frames[0].abi_kind = POLY_CROSS_BRIDGE_DEFAULT;
  state->cross_return.frames[0].flags = 0;
  state->riscv_gpr[1] = POLY_CROSS_RETURN_COOKIE_VALUE;
  state->riscv_gpr[2] = return_sp;
  asm volatile(
    "leaq 1f(%%rip), %%rcx\n"
    "movq %%rcx, %0\n"
    "movq %%rcx, %1\n"
    POLY_OP_STATE_IMPORT
    POLY_OP_ENTER_RV64
    ".long 0x05800513\n" // addi a0,zero,88
    ".long 0x00008067\n" // ret through imported cross-return cookie
    "1:\n"
    ".long 0xd5032e1f\n" // AArch64 x86 escape.
    : "=m"(state->transition.active.return_pc),
      "=m"(state->cross_return.frames[0].return_pc),
      "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, POLY_ABI_FP_CLOBBERS, "r10", "r11",
      "memory");
  return rax;
}

static uint64_t import_live_reverse_cross_return_state_probe(
    struct poly_xsave_state *state) {
  uint64_t return_sp = read_rsp();
  uint64_t rax = (uint64_t) (uintptr_t) state;
  state->header.current_mode = POLY_MODE_RAW_AARCH64;
  state->transition.active.return_pc = 0;
  state->transition.active.caller_mode = POLY_MODE_RAW_RISCV;
  state->transition.active.target_mode = POLY_MODE_RAW_AARCH64;
  state->transition.active.abi_kind = POLY_CROSS_BRIDGE_DEFAULT;
  state->transition.active.flags = 0;
  state->transition.active.cookie = return_sp;
  state->cross_return.top = 1;
  state->cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
  state->cross_return.frames[0].return_pc = 0;
  state->cross_return.frames[0].return_sp = return_sp;
  state->cross_return.frames[0].caller_mode = POLY_MODE_RAW_RISCV;
  state->cross_return.frames[0].target_mode = POLY_MODE_RAW_AARCH64;
  state->cross_return.frames[0].abi_kind = POLY_CROSS_BRIDGE_DEFAULT;
  state->cross_return.frames[0].flags = 0;
  state->aarch64_gpr[30] = POLY_CROSS_RETURN_COOKIE_VALUE;
  state->aarch64_gpr[31] = return_sp;
  asm volatile(
    "leaq 1f(%%rip), %%rcx\n"
    "movq %%rcx, %0\n"
    "movq %%rcx, %1\n"
    POLY_OP_STATE_IMPORT
    POLY_OP_ENTER_A64
    ".long 0xd2800c60\n" // movz x0,#99
    ".long 0xd65f03c0\n" // ret through imported cross-return cookie
    "1:\n"
    ".long 0x0000700b\n" // RISC-V x86 escape.
    : "=m"(state->transition.active.return_pc),
      "=m"(state->cross_return.frames[0].return_pc),
      "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, POLY_ABI_FP_CLOBBERS, "r10", "r11",
      "memory");
  return rax;
}

static inline void raw_aarch64_abi_args_probe(void) {
  asm volatile(
    "movq $1, %%rax\n"
    "movq $2, %%rdx\n"
    "movq $3, %%rcx\n"
    "movq $4, %%rdi\n"
    "movq $5, %%rsi\n"
    "movq $6, %%r8\n"
    "movq $7, %%r9\n"
    "movq $8, %%r10\n"
    POLY_OP_ENTER_A64
    ".long 0x8b010000\n"
    ".long 0x8b020000\n"
    ".long 0x8b030000\n"
    ".long 0x8b040000\n"
    ".long 0x8b050000\n"
    ".long 0x8b060000\n"
    ".long 0x8b070000\n"
    ".long 0xd5032e1f\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
        "r15", "memory");
}

static inline void raw_riscv_abi_args_probe(void) {
  asm volatile(
    "movq $1, %%rax\n"
    "movq $2, %%rdx\n"
    "movq $3, %%rcx\n"
    "movq $4, %%rdi\n"
    "movq $5, %%rsi\n"
    "movq $6, %%r8\n"
    "movq $7, %%r9\n"
    "movq $8, %%r10\n"
    POLY_OP_ENTER_RV64
    ".long 0x00b50533\n"
    ".long 0x00c50533\n"
    ".long 0x00d50533\n"
    ".long 0x00e50533\n"
    ".long 0x00f50533\n"
    ".long 0x01050533\n"
    ".long 0x01150533\n"
    ".long 0x0000700b\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
        "r15", "memory");
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
    POLY_OP_PCALL_A64
    "1:\n"
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0x8b020000\n" // add x0,x0,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
        "r11", "r15", "memory");
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
    POLY_OP_PCALL_RV64
    "1:\n"
    ".long 0x00b50533\n" // add a0,a0,a1
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x00008067\n" // ret
    "2:\n"
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
        "r11", "r15", "memory");
}

static inline void poly_opcode_pcall_aarch64_sysv_args_probe(void) {
  asm volatile(
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_A64
    "1:\n"
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0x8b020000\n" // add x0,x0,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
        "r11", "r15", "memory");
}

static inline void poly_opcode_pcall_riscv_sysv_args_probe(void) {
  asm volatile(
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_RV64
    "1:\n"
    ".long 0x00b50533\n" // add a0,a0,a1
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x00008067\n" // ret
    "2:\n"
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
        "r11", "r15", "memory");
}

static inline void pcall_signature_aarch64_sysv_args_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "movq %0, %%r12\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_A64
    "1:\n"
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0x8b020000\n" // add x0,x0,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    :
    : "r"((uint64_t) polyprobe_native_signature_slot)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r15", "memory");
}

static inline void pcall_signature_riscv_sysv_args_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "movq %0, %%r12\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_RV64
    "1:\n"
    ".long 0x00b50533\n" // add a0,a0,a1
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x00008067\n" // ret
    "2:\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    :
    : "r"((uint64_t) polyprobe_native_signature_slot)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r15", "memory");
}

static inline void pcall_signature_aarch64_exchange_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "movq $1, %%rax\n"
    "movq $2, %%rdx\n"
    "movq $3, %%rcx\n"
    "movq $4, %%rdi\n"
    "movq $5, %%rsi\n"
    "movq $6, %%r8\n"
    "movq $7, %%r9\n"
    "movq $8, %%r10\n"
    "movq $4, %%r12\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_A64
    "1:\n"
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0x8b020000\n" // add x0,x0,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0x8b060000\n" // add x0,x0,x6
    ".long 0x8b070000\n" // add x0,x0,x7
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r15", "memory");
}

static inline void pcall_signature_riscv_exchange_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "movq $1, %%rax\n"
    "movq $2, %%rdx\n"
    "movq $3, %%rcx\n"
    "movq $4, %%rdi\n"
    "movq $5, %%rsi\n"
    "movq $6, %%r8\n"
    "movq $7, %%r9\n"
    "movq $8, %%r10\n"
    "movq $4, %%r12\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_RV64
    "1:\n"
    ".long 0x00b50533\n" // add a0,a0,a1
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x01050533\n" // add a0,a0,a6
    ".long 0x01150533\n" // add a0,a0,a7
    ".long 0x00008067\n" // ret
    "2:\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r15", "memory");
}

static inline void pcall_signature_mode_aarch64_sysv_args_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "pushq %%r15\n"
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "movq %1, %%r12\n"
    "movq %0, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_MODE
    "1:\n"
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0x8b020000\n" // add x0,x0,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "popq %%r15\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_AARCH64),
      "r"((uint64_t) polyprobe_native_signature_slot)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
}

static inline void pcall_signature_mode_riscv_sysv_args_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "pushq %%r15\n"
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "movq %1, %%r12\n"
    "movq %0, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_MODE
    "1:\n"
    ".long 0x00b50533\n" // add a0,a0,a1
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x00008067\n" // ret
    "2:\n"
    "popq %%r15\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_RISCV),
      "r"((uint64_t) polyprobe_native_signature_slot)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
}

static inline void pcall_signature_imm_mode_aarch64_sysv_args_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "movq %0, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_IMM_SLOT3
    "1:\n"
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0x8b020000\n" // add x0,x0,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_AARCH64)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
}

static inline void pcall_signature_imm_mode_riscv_sysv_args_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "movq %0, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_IMM_SLOT3
    "1:\n"
    ".long 0x00b50533\n" // add a0,a0,a1
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x00008067\n" // ret
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_RISCV)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
}

static inline void pcall_signature_imm_mode_x86_sysv_args_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    "movq %0, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_IMM_SLOT3
    "1:\n"
    "movq %%rdi, %%rax\n"
    "addq %%rsi, %%rax\n"
    "addq %%rdx, %%rax\n"
    "addq %%rcx, %%rax\n"
    "addq %%r8, %%rax\n"
    "addq %%r9, %%rax\n"
    "retq\n"
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_X86)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
}

static inline void aarch64_hfa32_sentinel_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x52824680\n" // movz w0,#0x1234
    ".long 0x1e270003\n" // fmov s3,w0
    ".long 0x528acf00\n" // movz w0,#0x5678
    ".long 0x1e270004\n" // fmov s4,w0
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void pcall_signature_aarch64_hfa3_next_fp_probe(uint64_t slot) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "pushq %%r15\n"
    "movq %0, %%r12\n"
    "movl $1, %%r15d\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_MODE
    "1:\n"
    ".long 0x1e204060\n" // fmov s0,s3
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "popq %%r15\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    :
    : "r"(slot)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "memory");
}

static inline void pcall_signature_aarch64_hfa4_next_fp_probe(uint64_t slot) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "pushq %%r15\n"
    "movq %0, %%r12\n"
    "movl $1, %%r15d\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_MODE
    "1:\n"
    ".long 0x1e204080\n" // fmov s0,s4
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "popq %%r15\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    :
    : "r"(slot)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "memory");
}

static inline void aarch64_signature_imm_call_x86_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0003f0\n" // mov x16,x0 (target)
    ".long 0xaa0103f2\n" // mov x18,x1 (return)
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd5032a7f\n" // aarch64 PCALL_SIG_IMM slot 3
    "1:\n"
    "movq %%rdi, %%rax\n"
    "addq %%rsi, %%rax\n"
    "addq %%rdx, %%rax\n"
    "addq %%rcx, %%rax\n"
    "addq %%r8, %%rax\n"
    "addq %%r9, %%rax\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "memory");
}

static inline void riscv_signature_imm_call_x86_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_RV64
    ".long 0x00050293\n" // mv x5,a0 (target)
    ".long 0x00058393\n" // mv x7,a1 (return)
    ".long 0x00000313\n" // addi x6,zero,0 (x86 frontend)
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x4600700b\n" // riscv PCALL_SIG_IMM slot 3
    "1:\n"
    "movq %%rdi, %%rax\n"
    "addq %%rsi, %%rax\n"
    "addq %%rdx, %%rax\n"
    "addq %%rcx, %%rax\n"
    "addq %%r8, %%rax\n"
    "addq %%r9, %%rax\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "memory");
}

static inline void aarch64_landing_policy_call_x86_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0003f0\n" // mov x16,x0 (target)
    ".long 0xaa0103f2\n" // mov x18,x1 (return)
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0xd5032a7f\n" // aarch64 PCALL_SIG_IMM slot 3
    "1:\n"
    POLY_OP_LANDING
    "movq $61, %%rax\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "memory");
}

static inline void riscv_landing_policy_call_x86_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_RV64
    ".long 0x00050293\n" // mv x5,a0 (target)
    ".long 0x00058393\n" // mv x7,a1 (return)
    ".long 0x00000313\n" // addi x6,zero,0 (x86 frontend)
    ".long 0x4600700b\n" // riscv PCALL_SIG_IMM slot 3
    "1:\n"
    POLY_OP_LANDING
    "movq $62, %%rax\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "memory");
}

static inline void aarch64_signature_imm_call_x86_stack_probe(void) {
  asm volatile(
    "movabsq $0x5141524348535431, %%rcx\n"
    "pushq %%rcx\n"
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0003f0\n" // mov x16,x0 (target)
    ".long 0xaa0103f2\n" // mov x18,x1 (return)
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0xd5032a7f\n" // aarch64 PCALL_SIG_IMM slot 3
    "1:\n"
    "movq (%%r11), %%rax\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    "addq $8, %%rsp\n"
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "memory");
}

static inline void riscv_signature_imm_call_x86_stack_probe(void) {
  asm volatile(
    "movabsq $0x5152564353544b31, %%rcx\n"
    "pushq %%rcx\n"
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_RV64
    ".long 0x00050293\n" // mv x5,a0 (target)
    ".long 0x00058393\n" // mv x7,a1 (return)
    ".long 0x00000313\n" // addi x6,zero,0 (x86 frontend)
    ".long 0x4600700b\n" // riscv PCALL_SIG_IMM slot 3
    "1:\n"
    "movq (%%r11), %%rax\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    "addq $8, %%rsp\n"
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "memory");
}

static inline void aarch64_signature_imm_call_x86_fp64_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0003f0\n" // mov x16,x0 (target)
    ".long 0xaa0103f2\n" // mov x18,x1 (return)
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0xd5032a7f\n" // aarch64 PCALL_SIG_IMM slot 3
    "1:\n"
    "mulsd %%xmm1, %%xmm0\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "xmm0", "memory");
}

static inline void riscv_signature_imm_call_x86_fp64_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_RV64
    ".long 0x00050293\n" // mv x5,a0 (target)
    ".long 0x00058393\n" // mv x7,a1 (return)
    ".long 0x00000313\n" // addi x6,zero,0 (x86 frontend)
    ".long 0x4600700b\n" // riscv PCALL_SIG_IMM slot 3
    "1:\n"
    "mulsd %%xmm1, %%xmm0\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "xmm0", "memory");
}

static inline void aarch64_signature_imm_call_x86_vec128_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0003f0\n" // mov x16,x0 (target)
    ".long 0xaa0103f2\n" // mov x18,x1 (return)
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0xd5032a7f\n" // aarch64 PCALL_SIG_IMM slot 3
    "1:\n"
    "paddq %%xmm1, %%xmm0\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "xmm0", "memory");
}

static inline void riscv_signature_imm_call_x86_vec128_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_RV64
    ".long 0x00050293\n" // mv x5,a0 (target)
    ".long 0x00058393\n" // mv x7,a1 (return)
    ".long 0x00000313\n" // addi x6,zero,0 (x86 frontend)
    ".long 0x4600700b\n" // riscv PCALL_SIG_IMM slot 3
    "1:\n"
    "paddq %%xmm1, %%xmm0\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "xmm0", "memory");
}

static inline void raw_fp64_aarch64_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x1e612800\n"
    ".long 0x1e613800\n"
    ".long 0x1e610800\n"
    ".long 0xd5032e1f\n"
    ::: "r15", "xmm0", "memory");
}

static inline void raw_fp64_riscv_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x02b50553\n"
    ".long 0x0ab50553\n"
    ".long 0x12b50553\n"
    ".long 0x0000700b\n"
    ::: "r15", "xmm0", "memory");
}

static inline void pcall_fp64_aarch64_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_A64
    "1:\n"
    ".long 0x1e612800\n"
    ".long 0x1e613800\n"
    ".long 0x1e610800\n"
    ".long 0xd65f03c0\n"
    "2:\n"
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
        "r11", "r15", "xmm0", "memory");
}

static inline void pcall_fp64_riscv_probe(void) {
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_RV64
    "1:\n"
    ".long 0x02b50553\n"
    ".long 0x0ab50553\n"
    ".long 0x12b50553\n"
    ".long 0x00008067\n"
    "2:\n"
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
        "r11", "r15", "xmm0", "memory");
}

static inline void pcall_signature_fp64_aarch64_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "pushq %%r15\n"
    "movq %0, %%r15\n"
    "movq %1, %%r12\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_MODE
    "1:\n"
    ".long 0x1e612800\n"
    ".long 0x1e613800\n"
    ".long 0x1e610800\n"
    ".long 0xd65f03c0\n"
    "2:\n"
    "popq %%r15\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_AARCH64),
      "r"((uint64_t) polyprobe_fp64_signature_slot)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "memory");
}

static inline void pcall_signature_fp64_riscv_probe(void) {
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r12\n"
    "pushq %%r15\n"
    "movq %0, %%r15\n"
    "movq %1, %%r12\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_MODE
    "1:\n"
    ".long 0x02b50553\n"
    ".long 0x0ab50553\n"
    ".long 0x12b50553\n"
    ".long 0x00008067\n"
    "2:\n"
    "popq %%r15\n"
    "popq %%r12\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_RISCV),
      "r"((uint64_t) polyprobe_fp64_signature_slot)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "memory");
}

static inline void raw_barrier_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800120\n"
    ".long 0xd5033fbf\n"
    ".long 0xd5033f9f\n"
    ".long 0xd5033fdf\n"
    ".long 0x91002000\n"
    ".long 0xd5032e1f\n"
    POLY_OP_ENTER_RV64
    ".long 0x01400513\n"
    ".long 0x0ff0000f\n"
    ".long 0x0000100f\n"
    ".long 0x00250513\n"
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline uint64_t raw_mixed_probe(uint64_t value) {
  register uint64_t rax __asm__("rax") = value;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x91000400\n"
    ".long 0x10000070\n"
    ".long 0xd2800051\n"
    ".long 0xd5032f1f\n"
    ".long 0x00550513\n"
    ".long 0x00000297\n"
    ".long 0x01028293\n"
    ".long 0x00100313\n"
    ".long 0x1000700b\n"
    ".long 0x91000400\n"
    ".long 0xd5032e1f\n"
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline uint64_t raw_switch_stress_step(uint64_t value) {
  register uint64_t rax __asm__("rax") = value;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x91000400\n"
    ".long 0x10000070\n"
    ".long 0xd2800051\n"
    ".long 0xd5032f1f\n"
    ".long 0x00550513\n"
    ".long 0x00000297\n"
    ".long 0x01028293\n"
    ".long 0x00100313\n"
    ".long 0x1000700b\n"
    ".long 0x91000400\n"
    ".long 0xd5032e1f\n"
    : "+a"(rax)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return rax;
}

static inline void raw_aarch64_break_probe(uint64_t arg0) {
  register uint64_t rax __asm__("rax") = arg0;
  register uint64_t rdi __asm__("rdi") = arg0;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd4200020\n"
    ".long 0xd5032e1f\n"
    : "+a"(rax), "+D"(rdi)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDI, "memory");
}

static inline void raw_riscv_break_probe(uint64_t arg0) {
  register uint64_t rax __asm__("rax") = arg0;
  register uint64_t rdi __asm__("rdi") = arg0;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00100893\n"
    ".long 0x00100073\n"
    ".long 0x0000700b\n"
    : "+a"(rax), "+D"(rdi)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDI, "memory");
}

static inline uint64_t raw_aarch64_trap_restore_probe(void) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800233\n" // movz x19,#17
    ".long 0xd2800454\n" // movz x20,#34
    ".long 0xd4200040\n" // brk #2
    ".long 0x8b140260\n" // add x0,x19,x20
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return result;
}

static inline uint64_t raw_riscv_trap_restore_probe(void) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01100913\n" // addi s2,zero,17
    ".long 0x02200993\n" // addi s3,zero,34
    ".long 0x00200893\n" // addi a7,zero,2
    ".long 0x00100073\n" // ebreak
    ".long 0x01390533\n" // add a0,s2,s3
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "=a"(result)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
  return result;
}

static inline uint64_t raw_aarch64_trap_fp_restore_probe(void) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2824693\n" // movz x19,#0x1234
    ".long 0x9e670274\n" // fmov d20,x19
    ".long 0xd4200060\n" // brk #3
    ".long 0x9e660280\n" // fmov x0,d20
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "xmm4", "memory");
  return result;
}

static inline void raw_riscv_trap_fp_restore_probe(void) {
  write_xmm0_u64(0x1234);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x22a50a53\n" // fsgnj.d f20,fa0,fa0
    ".long 0x00300893\n" // addi a7,zero,3
    ".long 0x00100073\n" // ebreak
    ".long 0x234a0553\n" // fsgnj.d fa0,f20,f20
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "xmm0", "memory");
}

static inline void raw_aarch64_import_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd29c1010\n" // movz x16,#0xe080 (import id 8)
    ".long 0xf2bffff0\n" // movk x16,#0xffff,lsl #16
    ".long 0xf2dffff0\n" // movk x16,#0xffff,lsl #32
    ".long 0xf2fffff0\n" // movk x16,#0xffff,lsl #48
    ".long 0xd28009a0\n" // movz x0,#77
    ".long 0xd28009c1\n" // movz x1,#78
    ".long 0xd28009e2\n" // movz x2,#79
    ".long 0xd2800a03\n" // movz x3,#80
    ".long 0xd2800a24\n" // movz x4,#81
    ".long 0xd2800a45\n" // movz x5,#82
    ".long 0xd2800b06\n" // movz x6,#88
    ".long 0xd2800c67\n" // movz x7,#99
    ".long 0xd63f0200\n" // blr x16, reserved import must trap
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "r12", "r13", "r14",
        "memory");
}

static inline void raw_riscv_import_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> import id 8
    ".long 0x0b100513\n" // addi a0,zero,177
    ".long 0x0b200593\n" // addi a1,zero,178
    ".long 0x0b300613\n" // addi a2,zero,179
    ".long 0x0b400693\n" // addi a3,zero,180
    ".long 0x0b500713\n" // addi a4,zero,181
    ".long 0x0b600793\n" // addi a5,zero,182
    ".long 0x0b700813\n" // addi a6,zero,183
    ".long 0x0b800893\n" // addi a7,zero,184
    ".long 0x000280e7\n" // jalr ra,0(t0), reserved import must trap
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "r12", "r13", "r14",
        "memory");
}

static inline void raw_aarch64_getpid_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28009a0\n" // movz x0,#77
    ".long 0xd28009c1\n" // movz x1,#78
    ".long 0xd28009e2\n" // movz x2,#79
    ".long 0xd2800a03\n" // movz x3,#80
    ".long 0xd2800a24\n" // movz x4,#81
    ".long 0xd2800a45\n" // movz x5,#82
    ".long 0xd2800b06\n" // movz x6,#88
    ".long 0xd2800c67\n" // movz x7,#99
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd4000001\n"
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "memory");
}

static inline void raw_riscv_getpid_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x04d00513\n" // addi a0,zero,77
    ".long 0x04e00593\n" // addi a1,zero,78
    ".long 0x04f00613\n" // addi a2,zero,79
    ".long 0x05000693\n" // addi a3,zero,80
    ".long 0x05100713\n" // addi a4,zero,81
    ".long 0x05200793\n" // addi a5,zero,82
    ".long 0x05800813\n" // addi a6,zero,88
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n"
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "r10", "r11", "memory");
}

static inline void raw_aarch64_illegal_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xffffffff\n"
    ".long 0xd5032e1f\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_illegal_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0xffffffff\n"
    ".long 0x0000700b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_invalid_branch_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800050\n" // movz x16,#2: canonical but not AArch64 aligned.
    ".long 0xd61f0200\n" // br x16: invalid target must trap.
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape after trap resume.
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_invalid_branch_probe(void) {
  uint64_t target = POLYPROBE_INVALID_RISCV_BRANCH_TARGET;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00050067\n" // jalr zero,0(a0): non-canonical target must trap.
    ".long 0x0000700b\n" // riscv polyctrl x86 escape after trap resume.
    : "+a"(target)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX, "memory");
}

static inline void raw_aarch64_invalid_x86_call_return_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800010\n" // movz x16,#0: target is unused after return rejection.
    ".long 0xd2800011\n" // movz x17,#0: x86 frontend.
    ".long 0xd2800052\n" // movz x18,#2: canonical but not AArch64 aligned.
    ".long 0xd5032a7f\n" // AArch64 PCALL_SIG_IMM slot 3 must trap.
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape after trap resume.
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_invalid_x86_call_return_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00000293\n" // addi t0,zero,0: target is unused after return rejection.
    ".long 0x00000313\n" // addi t1,zero,0: x86 frontend.
    ".long 0x00100393\n" // addi t2,zero,1: not RISC-V halfword aligned.
    ".long 0x4600700b\n" // RISC-V PCALL_SIG_IMM slot 3 must trap.
    ".long 0x0000700b\n" // riscv polyctrl x86 escape after trap resume.
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_invalid_cross_call_return_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800010\n" // movz x16,#0: target is unused after return rejection.
    ".long 0xd2800051\n" // movz x17,#2: RISC-V frontend.
    ".long 0xd2800052\n" // movz x18,#2: canonical but not AArch64 aligned.
    ".long 0xd5032f3f\n" // AArch64 generic PCALL must trap.
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape after trap resume.
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_invalid_cross_call_return_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00000293\n" // addi t0,zero,0: target is unused after return rejection.
    ".long 0x00100313\n" // addi t1,zero,1: AArch64 frontend.
    ".long 0x00100393\n" // addi t2,zero,1: not RISC-V halfword aligned.
    ".long 0x1200700b\n" // RISC-V generic PCALL must trap.
    ".long 0x0000700b\n" // riscv polyctrl x86 escape after trap resume.
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

int main(void) {
  stage("POLY_PROBE: start");
  install_polyprobe_trap_vector();

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
  const uint32_t forbidden_features = poly_cpuid_forbidden_feature_mask();
  if ((poly_features.ecx & forbidden_features) != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: forbidden poly CPUID features advertised ecx=0x%x forbidden=0x%x\n",
      poly_features.ecx, forbidden_features);
    return 1;
  }
  if (poly_features.eax != POLY_CPUID_ABI_VERSION ||
      poly_features.ebx != poly_cpuid_expected_mode_mask() ||
      poly_features.ecx != poly_cpuid_expected_feature_mask() ||
      poly_features.edx != POLY_STATE_XSAVE_COMPONENT_ARCH) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID feature mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_features.eax, poly_features.ebx, poly_features.ecx, poly_features.edx);
    return 1;
  }
  struct poly_cpuid_regs expected_escapes =
    poly_cpuid_expected_escape_leaf0();
  struct poly_cpuid_regs poly_escapes =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 0);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID escape leaf0 mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf1();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 1);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID escape leaf1 mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf2();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 2);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID escape leaf2 mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf3();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 3);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID escape leaf3 mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf4();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 4);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID escape leaf4 mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf5();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 5);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID x86 control leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf6();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 6);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID generic switch manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf7();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 7);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID immediate pcall manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  polyprobe_native_signature_slot = (poly_escapes.ecx >> 24) & 0xffU;
  if (polyprobe_native_signature_slot >= poly_escapes.ebx ||
      ((poly_escapes.edx >> 24) & 0xffU) !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: native signature slot manifest mismatch slot=%u kind=0x%x count=%u\n",
      polyprobe_native_signature_slot, (poly_escapes.edx >> 24) & 0xffU,
      poly_escapes.ebx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf8();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 8);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID foreign signature pcall manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf9();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 9);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID landing pad manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf10();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 10);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID foreign ABI signature control manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf11();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 11);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID foreign immediate signature pcall manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf12();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 12);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID foreign trap control manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf13();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 13);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID aarch64 trap detail manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf14();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 14);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID riscv trap detail manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf15();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 15);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID landing policy manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf16();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 16);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID landing policy details mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf17();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 17);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID extended ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf18();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 18);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID ABI register map manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf19();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 19);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID state-key manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf20();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 20);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID compact ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf21();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 21);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID state-key detail manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf22();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 22);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID FP64 ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  polyprobe_fp64_signature_slot = poly_escapes.edx;
  if (polyprobe_fp64_signature_slot >= POLY_ABI_SIGNATURE_SLOT_COUNT ||
      poly_abi_signature_get(polyprobe_fp64_signature_slot) !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: default FP64 ABI signature slot mismatch slot=%u count=%u kind=%llu\n",
      polyprobe_fp64_signature_slot, POLY_ABI_SIGNATURE_SLOT_COUNT,
      (unsigned long long) poly_abi_signature_get(
        polyprobe_fp64_signature_slot));
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf23();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 23);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID FP32 ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  if (poly_escapes.edx >= POLY_ABI_SIGNATURE_SLOT_COUNT ||
      poly_abi_signature_get(poly_escapes.edx) !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP32) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: default FP32 ABI signature slot mismatch slot=%u count=%u kind=%llu\n",
      poly_escapes.edx, POLY_ABI_SIGNATURE_SLOT_COUNT,
      (unsigned long long) poly_abi_signature_get(poly_escapes.edx));
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf24();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 24);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID SRET ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  if (poly_escapes.eax >= POLY_ABI_SIGNATURE_SLOT_COUNT ||
      poly_abi_signature_get(poly_escapes.eax) !=
        POLY_ABI_SIGNATURE_KIND_SRET_X86_SYSV_REGS) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: default SRET ABI signature slot mismatch slot=%u count=%u kind=%llu\n",
      poly_escapes.eax, POLY_ABI_SIGNATURE_SLOT_COUNT,
      (unsigned long long) poly_abi_signature_get(poly_escapes.eax));
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf25();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 25);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID x86 SysV FP128 return ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  if (poly_escapes.eax >= POLY_ABI_SIGNATURE_SLOT_COUNT ||
      poly_abi_signature_get(poly_escapes.eax) !=
        POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FP128_RET) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: default x86 SysV FP128 return ABI signature slot mismatch slot=%u count=%u kind=%llu\n",
      poly_escapes.eax, POLY_ABI_SIGNATURE_SLOT_COUNT,
      (unsigned long long) poly_abi_signature_get(poly_escapes.eax));
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf26();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 26);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID AArch64 HFA32 return ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf27();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 27);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID AArch64 HFA32 argument ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf28();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 28);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID native SRET ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  if (poly_escapes.eax >= POLY_ABI_SIGNATURE_SLOT_COUNT ||
      poly_abi_signature_get(poly_escapes.eax) !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: default native SRET ABI signature slot mismatch slot=%u count=%u kind=%llu\n",
      poly_escapes.eax, POLY_ABI_SIGNATURE_SLOT_COUNT,
      (unsigned long long) poly_abi_signature_get(poly_escapes.eax));
    return 1;
  }
  expected_escapes = poly_cpuid_expected_escape_leaf29();
  poly_escapes = poly_read_cpuid(POLY_CPUID_BASE + 2, 29);
  if (poly_escapes.eax != expected_escapes.eax ||
      poly_escapes.ebx != expected_escapes.ebx ||
      poly_escapes.ecx != expected_escapes.ecx ||
      poly_escapes.edx != expected_escapes.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID AArch64 HFA64 return ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  struct poly_cpuid_contract_failure state_failure;
  if (!poly_cpuid_verify_arch_state_contract(&state_failure)) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: %s mismatch leaf=0x%x subleaf=%u got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      state_failure.name, state_failure.leaf, state_failure.subleaf,
      state_failure.actual.eax, state_failure.actual.ebx,
      state_failure.actual.ecx, state_failure.actual.edx,
      state_failure.expected.eax, state_failure.expected.ebx,
      state_failure.expected.ecx, state_failure.expected.edx);
    return 1;
  }
  struct poly_cpuid_regs expected_trap =
    poly_cpuid_expected_trap_leaf();
  struct poly_cpuid_regs poly_trap =
    poly_read_cpuid(POLY_CPUID_BASE + 5, 0);
  if (poly_trap.eax != expected_trap.eax ||
      poly_trap.ebx != expected_trap.ebx ||
      poly_trap.ecx != expected_trap.ecx ||
      poly_trap.edx != expected_trap.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID trap leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_trap.eax, poly_trap.ebx, poly_trap.ecx, poly_trap.edx);
    return 1;
  }
  struct poly_cpuid_regs expected_frontends =
    poly_cpuid_expected_frontend_leaf();
  struct poly_cpuid_regs poly_frontends =
    poly_read_cpuid(POLY_CPUID_BASE + 8, 1);
  if (poly_frontends.eax != expected_frontends.eax ||
      poly_frontends.ebx != expected_frontends.ebx ||
      poly_frontends.ecx != expected_frontends.ecx ||
      poly_frontends.edx != expected_frontends.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID frontend leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_frontends.eax, poly_frontends.ebx, poly_frontends.ecx,
      poly_frontends.edx);
    return 1;
  }
  struct poly_cpuid_regs expected_abi_bridge =
    poly_cpuid_expected_abi_bridge_leaf();
  struct poly_cpuid_regs poly_abi_bridge =
    poly_read_cpuid(POLY_CPUID_BASE + 9, 0);
  const uint32_t forbidden_abi_bridge =
    poly_cpuid_forbidden_abi_bridge_mask();
  if ((poly_abi_bridge.ebx & forbidden_abi_bridge) != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: forbidden ABI bridge hardware advertised ebx=0x%x forbidden=0x%x\n",
      poly_abi_bridge.ebx, forbidden_abi_bridge);
    return 1;
  }
  if ((poly_abi_bridge.ebx &
        POLY_ABI_BRIDGE_FLAG_REGISTER_MAP_SIGNATURES) == 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: ABI register-map signature support not advertised ebx=0x%x\n",
      poly_abi_bridge.ebx);
    return 1;
  }
  if (poly_abi_bridge.eax != expected_abi_bridge.eax ||
      poly_abi_bridge.ebx != expected_abi_bridge.ebx ||
      poly_abi_bridge.ecx != expected_abi_bridge.ecx ||
      poly_abi_bridge.edx != expected_abi_bridge.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID ABI bridge leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_abi_bridge.eax, poly_abi_bridge.ebx, poly_abi_bridge.ecx,
      poly_abi_bridge.edx);
    return 1;
  }

  stage("POLY_STAGE: x86-status");
  poly_mode_x86();
  memset(&polyprobe_state, 0, sizeof(polyprobe_state));
  poly_state_export(&polyprobe_state);
  if (polyprobe_state.header.current_mode != POLY_MODE_X86) {
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

  stage("POLY_STAGE: x86-poly-opcodes");
  poly_opcode_aarch64_arith_probe();
  if (read_rax() != 43) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 poly opcode aarch64 stream mismatch\n");
    return 1;
  }
  poly_opcode_riscv_arith_probe();
  if (read_rax() != 22) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 poly opcode riscv stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: foreign-sp");
  uint64_t rsp_before = read_rsp();
  raw_aarch64_sp_probe();
  uint64_t rsp_after = read_rsp();
  if (read_rax() != 42 || rsp_after != rsp_before) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 SP frame mismatch result=%llu rsp_before=0x%llx rsp_after=0x%llx\n",
      (unsigned long long) read_rax(), (unsigned long long) rsp_before,
      (unsigned long long) rsp_after);
    return 1;
  }
  rsp_before = read_rsp();
  raw_aarch64_frame_pair_probe();
  rsp_after = read_rsp();
  if (read_rax() != 42 || rsp_after != rsp_before) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 frame pair mismatch result=%llu rsp_before=0x%llx rsp_after=0x%llx\n",
      (unsigned long long) read_rax(), (unsigned long long) rsp_before,
      (unsigned long long) rsp_after);
    return 1;
  }
  rsp_before = read_rsp();
  raw_riscv_sp_probe();
  rsp_after = read_rsp();
  if (read_rax() != 42 || rsp_after != rsp_before) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv SP frame mismatch result=%llu rsp_before=0x%llx rsp_after=0x%llx\n",
      (unsigned long long) read_rax(), (unsigned long long) rsp_before,
      (unsigned long long) rsp_after);
    return 1;
  }
  rsp_before = read_rsp();
  raw_riscv_compressed_frame_probe();
  rsp_after = read_rsp();
  if (read_rax() != 42 || rsp_after != rsp_before) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv compressed frame mismatch result=%llu rsp_before=0x%llx rsp_after=0x%llx\n",
      (unsigned long long) read_rax(), (unsigned long long) rsp_before,
      (unsigned long long) rsp_after);
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

  stage("POLY_STAGE: state-export-import");
  memset(&polyprobe_state, 0xa5, sizeof(polyprobe_state));
  const uint64_t seeded_aarch64_tls = 0x123456789abc0001ULL;
  const uint64_t seeded_riscv_tls = 0x223456789abc0002ULL;
  const uint64_t seeded_aarch64_fpcr = 0x00c00000ULL;
  const uint64_t seeded_aarch64_fpsr = 0x12ULL;
  const uint64_t seeded_riscv_fcsr = 0x75ULL;
  raw_aarch64_state_seed_probe();
  raw_riscv_state_seed_probe();
  raw_aarch64_status_seed_probe();
  raw_riscv_status_seed_probe();
  if (penter_aarch64_tls_probe(seeded_aarch64_tls) !=
        seeded_aarch64_tls ||
      penter_riscv_tls_probe(seeded_riscv_tls) != seeded_riscv_tls) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: frontend TLS seed mismatch aarch64=0x%llx riscv=0x%llx\n",
      (unsigned long long) penter_aarch64_tls_probe(seeded_aarch64_tls),
      (unsigned long long) penter_riscv_tls_probe(seeded_riscv_tls));
    return 1;
  }
  poly_state_export(&polyprobe_state);
  const uint64_t expected_xsave_flags =
    poly_cpuid_expected_arch_state_leaf().edx;
  if (polyprobe_state.header.magic != POLY_STATE_XSAVE_MAGIC ||
      polyprobe_state.header.layout_version != POLY_STATE_XSAVE_LAYOUT_VERSION ||
      polyprobe_state.header.header_bytes != POLY_STATE_XSAVE_HEADER_BYTES ||
      polyprobe_state.header.total_bytes != sizeof(polyprobe_state) ||
      polyprobe_state.header.flags != expected_xsave_flags) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly state export header mismatch magic=0x%x version=%u header=%u bytes=%u flags=0x%llx expected_flags=0x%llx\n",
      polyprobe_state.header.magic, polyprobe_state.header.layout_version,
      polyprobe_state.header.header_bytes, polyprobe_state.header.total_bytes,
      (unsigned long long) polyprobe_state.header.flags,
      (unsigned long long) expected_xsave_flags);
    return 1;
  }
  if (polyprobe_state.aarch64_gpr[19] != 0x1234 ||
      polyprobe_state.aarch64_fp[8].lo != 0x1234 ||
      polyprobe_state.aarch64_fp[8].hi != 0 ||
      polyprobe_state.riscv_gpr[19] != 0x321 ||
      polyprobe_state.riscv_fp[18].lo != 0x321 ||
      polyprobe_state.riscv_fp[18].hi != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly state export register mismatch a19=0x%llx d8=0x%llx:%llx r19=0x%llx f18=0x%llx:%llx\n",
      (unsigned long long) polyprobe_state.aarch64_gpr[19],
      (unsigned long long) polyprobe_state.aarch64_fp[8].hi,
      (unsigned long long) polyprobe_state.aarch64_fp[8].lo,
      (unsigned long long) polyprobe_state.riscv_gpr[19],
      (unsigned long long) polyprobe_state.riscv_fp[18].hi,
      (unsigned long long) polyprobe_state.riscv_fp[18].lo);
    return 1;
  }
  if (polyprobe_state.aarch64_status.fpcr != seeded_aarch64_fpcr ||
      polyprobe_state.aarch64_status.fpsr != seeded_aarch64_fpsr ||
      polyprobe_state.riscv_status.fcsr != seeded_riscv_fcsr) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: poly state export status mismatch fpcr=0x%llx fpsr=0x%llx fcsr=0x%llx\n",
      (unsigned long long) polyprobe_state.aarch64_status.fpcr,
      (unsigned long long) polyprobe_state.aarch64_status.fpsr,
      (unsigned long long) polyprobe_state.riscv_status.fcsr);
    return 1;
  }
  if (polyprobe_state.frontend_tls.flags != 1 ||
      polyprobe_state.frontend_tls.active_mode != POLY_MODE_X86 ||
      polyprobe_state.frontend_tls.aarch64_tls_base != seeded_aarch64_tls ||
      polyprobe_state.frontend_tls.riscv_tls_base != seeded_riscv_tls) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: poly state export frontend TLS mismatch flags=0x%llx active=%llu aarch64=0x%llx riscv=0x%llx\n",
      (unsigned long long) polyprobe_state.frontend_tls.flags,
      (unsigned long long) polyprobe_state.frontend_tls.active_mode,
      (unsigned long long) polyprobe_state.frontend_tls.aarch64_tls_base,
      (unsigned long long) polyprobe_state.frontend_tls.riscv_tls_base);
    return 1;
  }
  polyprobe_state.aarch64_gpr[19] = 0x2468;
  polyprobe_state.aarch64_fp[8].lo = 0x3579;
  polyprobe_state.riscv_gpr[19] = 0x432;
  polyprobe_state.riscv_fp[18].lo = 0x543;
  const uint64_t imported_aarch64_fpcr = 0x00400000ULL;
  const uint64_t imported_aarch64_fpsr = 0x8ULL;
  const uint64_t imported_riscv_fcsr = 0x43ULL;
  polyprobe_state.aarch64_status.fpcr = imported_aarch64_fpcr;
  polyprobe_state.aarch64_status.fpsr = imported_aarch64_fpsr;
  polyprobe_state.riscv_status.fcsr = imported_riscv_fcsr;
  const uint64_t imported_aarch64_tls = 0x323456789abc0003ULL;
  const uint64_t imported_riscv_tls = 0x423456789abc0004ULL;
  polyprobe_state.frontend_tls.aarch64_tls_base = imported_aarch64_tls;
  polyprobe_state.frontend_tls.riscv_tls_base = imported_riscv_tls;
  poly_state_import(&polyprobe_state);
  memset(&polyprobe_state, 0xa5, sizeof(polyprobe_state));
  poly_state_export(&polyprobe_state);
  if (polyprobe_state.frontend_tls.aarch64_tls_base !=
        imported_aarch64_tls ||
      polyprobe_state.frontend_tls.riscv_tls_base != imported_riscv_tls) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: poly state import frontend TLS mismatch aarch64=0x%llx riscv=0x%llx\n",
      (unsigned long long) polyprobe_state.frontend_tls.aarch64_tls_base,
      (unsigned long long) polyprobe_state.frontend_tls.riscv_tls_base);
    return 1;
  }
  if (polyprobe_state.aarch64_status.fpcr != imported_aarch64_fpcr ||
      polyprobe_state.aarch64_status.fpsr != imported_aarch64_fpsr ||
      polyprobe_state.riscv_status.fcsr != imported_riscv_fcsr) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: poly state import status export mismatch fpcr=0x%llx fpsr=0x%llx fcsr=0x%llx\n",
      (unsigned long long) polyprobe_state.aarch64_status.fpcr,
      (unsigned long long) polyprobe_state.aarch64_status.fpsr,
      (unsigned long long) polyprobe_state.riscv_status.fcsr);
    return 1;
  }
  raw_aarch64_state_gpr_probe();
  if (read_rax() != 0x2468) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly state import aarch64 gpr mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  raw_aarch64_state_fp_probe();
  if (read_rax() != 0x3579) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly state import aarch64 fp mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  raw_riscv_state_gpr_probe();
  if (read_rax() != 0x432) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly state import riscv gpr mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  raw_riscv_state_fp_probe();
  if (read_rax() != 0x543) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly state import riscv fp mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  raw_aarch64_status_sum_probe();
  if (read_rax() != imported_aarch64_fpcr + imported_aarch64_fpsr) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: poly state import aarch64 status mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) read_rax(),
      (unsigned long long) (imported_aarch64_fpcr + imported_aarch64_fpsr));
    return 1;
  }
  raw_riscv_status_probe();
  if (read_rax() != imported_riscv_fcsr) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: poly state import riscv status mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) read_rax(),
      (unsigned long long) imported_riscv_fcsr);
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
  generic_enter_aarch64_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: generic aarch64 frontend enter mismatch\n");
    return 1;
  }
  generic_enter_riscv_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: generic riscv frontend enter mismatch\n");
    return 1;
  }
  generic_switch_aarch64_probe();
  if (read_rax() != 33) {
    fprintf(stderr, "POLY_PROBE_FAIL: generic aarch64 frontend switch mismatch\n");
    return 1;
  }
  generic_switch_riscv_probe();
  if (read_rax() != 33) {
    fprintf(stderr, "POLY_PROBE_FAIL: generic riscv frontend switch mismatch\n");
    return 1;
  }
  landing_pad_probe();
  if (read_rax() != 19) {
    fprintf(stderr, "POLY_PROBE_FAIL: landing pad marker mismatch\n");
    return 1;
  }
  CHECK_POLYPROBE_SWITCH_DELTA("aarch64 generic switch to riscv",
    aarch64_generic_switch_riscv_probe, 45, POLYPROBE_NEUTRAL_SWITCH_DELTA);
  aarch64_generic_switch_x86_probe();
  if (read_rax() != 57) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 generic switch to x86 mismatch\n");
    return 1;
  }
  CHECK_POLYPROBE_SWITCH_DELTA("riscv generic switch to aarch64",
    riscv_generic_switch_aarch64_probe, 45, POLYPROBE_NEUTRAL_SWITCH_DELTA);
  riscv_generic_switch_x86_probe();
  if (read_rax() != 58) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv generic switch to x86 mismatch\n");
    return 1;
  }
  CHECK_POLYPROBE_SWITCH_DELTA("aarch64 generic call to riscv",
    aarch64_generic_call_riscv_probe, 45, POLYPROBE_NEUTRAL_CALL_DELTA);
  CHECK_POLYPROBE_SWITCH_DELTA("riscv generic call to aarch64",
    riscv_generic_call_aarch64_probe, 45, POLYPROBE_NEUTRAL_CALL_DELTA);

  stage("POLY_STAGE: abi-args");
  raw_aarch64_abi_args_probe();
  if (read_rax() != 36) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 ABI argument bridge mismatch\n");
    return 1;
  }
  raw_riscv_abi_args_probe();
  if (read_rax() != 36) {
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
  poly_opcode_pcall_aarch64_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 poly opcode pcall aarch64 SysV argument bridge mismatch\n");
    return 1;
  }
  poly_opcode_pcall_riscv_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 poly opcode pcall riscv SysV argument bridge mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: abi-signatures");
  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0 ||
      poly_abi_signature_get(3) != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS ||
      poly_abi_signature_set(4, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0 ||
      poly_abi_signature_get(4) != POLY_ABI_SIGNATURE_KIND_EXCHANGE ||
      poly_abi_signature_set(7,
        POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_I128) != 0 ||
      poly_abi_signature_get(7) !=
        POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_I128) {
    fprintf(stderr, "POLY_PROBE_FAIL: ABI signature slot programming mismatch\n");
    return 1;
  }
  if (poly_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_COUNT,
        POLY_ABI_SIGNATURE_KIND_EXCHANGE) != POLY_ERR_INVAL ||
      poly_abi_signature_get(POLY_ABI_SIGNATURE_SLOT_COUNT) !=
        POLY_ERR_INVAL ||
      poly_abi_signature_set(3,
        POLY_ABI_SIGNATURE_KIND_INVALID_TEST) !=
        POLY_ERR_INVAL ||
      poly_abi_signature_get(3) !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 ABI signature invalid control mismatch\n");
    return 1;
  }
  if (poly_abi_signature_set_raw(3,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS |
        ((uint64_t) POLY_ABI_REGISTER_MAP_X86_SYSV_TO_NATIVE << 32)) !=
        POLY_ERR_INVAL ||
      poly_abi_signature_get(3) !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 ABI signature mismatched register map accepted\n");
    return 1;
  }
  aarch64_abi_signature_control_probe();
  if (read_rax() != POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 ABI signature control mismatch\n");
    return 1;
  }
  aarch64_abi_signature_invalid_slot_probe();
  if (read_rax() != POLY_ERR_INVAL) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 ABI signature invalid slot mismatch\n");
    return 1;
  }
  aarch64_abi_signature_invalid_kind_probe();
  if (read_rax() != POLY_ERR_INVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 ABI signature invalid kind mismatch\n");
    return 1;
  }
  if (poly_abi_signature_set(5, POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0 ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr, "POLY_PROBE_FAIL: native ABI signature slot programming mismatch\n");
    return 1;
  }
  riscv_abi_signature_control_probe();
  if (read_rax() != POLY_ABI_SIGNATURE_KIND_EXCHANGE ||
      poly_abi_signature_get(6) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv ABI signature control mismatch\n");
    return 1;
  }
  riscv_abi_signature_invalid_slot_probe();
  if (read_rax() != POLY_ERR_INVAL) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv ABI signature invalid slot mismatch\n");
    return 1;
  }
  riscv_abi_signature_invalid_kind_probe();
  if (read_rax() != POLY_ERR_INVAL ||
      poly_abi_signature_get(6) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv ABI signature invalid kind mismatch\n");
    return 1;
  }
  if (poly_abi_signature_set(polyprobe_native_signature_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0 ||
      poly_abi_signature_get(polyprobe_native_signature_slot) !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: discovered native ABI signature slot setup mismatch slot=%u\n",
      polyprobe_native_signature_slot);
    return 1;
  }
  if (poly_abi_signature_set(7,
        POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA3_F32_ARG) != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: AArch64 HFA3 FP32 signature slot setup failed\n");
    return 1;
  }
  aarch64_hfa32_sentinel_probe();
  write_xmm0_u64(0xaaaabbbbccccddddULL);
  write_xmm1_u64(0x1111222233334444ULL);
  write_xmm2_u64(0x0000000099aabbccULL);
  pcall_signature_aarch64_hfa3_next_fp_probe(7);
  if ((uint32_t) read_xmm0_u64() != 0x99aabbccU) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: AArch64 HFA3 FP32 signature did not map next FP arg got=0x%x\n",
      (uint32_t) read_xmm0_u64());
    return 1;
  }
  if (poly_abi_signature_set(7,
        POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA4_F32_ARG) != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: AArch64 HFA4 FP32 signature slot setup failed\n");
    return 1;
  }
  aarch64_hfa32_sentinel_probe();
  write_xmm0_u64(0xaaaabbbbccccddddULL);
  write_xmm1_u64(0x1111222233334444ULL);
  write_xmm2_u64(0x0000000099aabbccULL);
  pcall_signature_aarch64_hfa4_next_fp_probe(7);
  if ((uint32_t) read_xmm0_u64() != 0x99aabbccU) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: AArch64 HFA4 FP32 signature did not map next FP arg got=0x%x\n",
      (uint32_t) read_xmm0_u64());
    return 1;
  }
  if (poly_abi_signature_set(7,
        POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_I128) != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: ABI signature slot restore failed\n");
    return 1;
  }

  stage("POLY_STAGE: cross-return-xsave");
  memset(&polyprobe_state, 0xa5, sizeof(polyprobe_state));
  export_live_cross_return_state_probe(&polyprobe_state);
  if (read_rax() != 77) {
    fprintf(stderr, "POLY_PROBE_FAIL: live cross-return export result mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (polyprobe_state.cross_return.top != 1 ||
      polyprobe_state.cross_return.depth != POLY_STATE_XSAVE_CROSS_RETURN_DEPTH ||
      polyprobe_state.cross_return.frames[0].caller_mode !=
        POLY_MODE_RAW_AARCH64 ||
      polyprobe_state.cross_return.frames[0].target_mode !=
        POLY_MODE_RAW_RISCV ||
      polyprobe_state.cross_return.frames[0].abi_kind !=
        POLY_CROSS_BRIDGE_DEFAULT ||
      polyprobe_state.cross_return.frames[0].return_pc == 0 ||
      polyprobe_state.cross_return.frames[0].return_sp == 0) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: live cross-return XSAVE mismatch top=%llu depth=%llu caller=%u target=%u abi=%u pc=0x%llx sp=0x%llx\n",
      (unsigned long long) polyprobe_state.cross_return.top,
      (unsigned long long) polyprobe_state.cross_return.depth,
      polyprobe_state.cross_return.frames[0].caller_mode,
      polyprobe_state.cross_return.frames[0].target_mode,
      polyprobe_state.cross_return.frames[0].abi_kind,
      (unsigned long long) polyprobe_state.cross_return.frames[0].return_pc,
      (unsigned long long) polyprobe_state.cross_return.frames[0].return_sp);
    return 1;
  }
  if (polyprobe_state.transition.active.return_pc !=
        polyprobe_state.cross_return.frames[0].return_pc ||
      polyprobe_state.transition.active.caller_mode !=
        POLY_MODE_RAW_AARCH64 ||
      polyprobe_state.transition.active.target_mode !=
        POLY_MODE_RAW_RISCV ||
      polyprobe_state.transition.active.abi_kind !=
        POLY_CROSS_BRIDGE_DEFAULT ||
      polyprobe_state.transition.active.cookie !=
        polyprobe_state.cross_return.frames[0].return_sp) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: live cross-return transition mismatch pc=0x%llx caller=%u target=%u abi=%u cookie=0x%llx\n",
      (unsigned long long) polyprobe_state.transition.active.return_pc,
      polyprobe_state.transition.active.caller_mode,
      polyprobe_state.transition.active.target_mode,
      polyprobe_state.transition.active.abi_kind,
      (unsigned long long) polyprobe_state.transition.active.cookie);
    return 1;
  }
  uint64_t imported_cross_return_result =
    import_live_cross_return_state_probe(&polyprobe_state);
  if (imported_cross_return_result != 88) {
    fprintf(stderr, "POLY_PROBE_FAIL: imported cross-return resume mismatch got=%llu\n",
      (unsigned long long) imported_cross_return_result);
    return 1;
  }
  memset(&polyprobe_state, 0xa5, sizeof(polyprobe_state));
  export_live_reverse_cross_return_state_probe(&polyprobe_state);
  if (read_rax() != 99) {
    fprintf(stderr, "POLY_PROBE_FAIL: reverse live cross-return export result mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (polyprobe_state.cross_return.top != 1 ||
      polyprobe_state.cross_return.depth != POLY_STATE_XSAVE_CROSS_RETURN_DEPTH ||
      polyprobe_state.cross_return.frames[0].caller_mode !=
        POLY_MODE_RAW_RISCV ||
      polyprobe_state.cross_return.frames[0].target_mode !=
        POLY_MODE_RAW_AARCH64 ||
      polyprobe_state.cross_return.frames[0].abi_kind !=
        POLY_CROSS_BRIDGE_DEFAULT ||
      polyprobe_state.cross_return.frames[0].return_pc == 0 ||
      polyprobe_state.cross_return.frames[0].return_sp == 0) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: reverse live cross-return XSAVE mismatch top=%llu depth=%llu caller=%u target=%u abi=%u pc=0x%llx sp=0x%llx\n",
      (unsigned long long) polyprobe_state.cross_return.top,
      (unsigned long long) polyprobe_state.cross_return.depth,
      polyprobe_state.cross_return.frames[0].caller_mode,
      polyprobe_state.cross_return.frames[0].target_mode,
      polyprobe_state.cross_return.frames[0].abi_kind,
      (unsigned long long) polyprobe_state.cross_return.frames[0].return_pc,
      (unsigned long long) polyprobe_state.cross_return.frames[0].return_sp);
    return 1;
  }
  if (polyprobe_state.transition.active.return_pc !=
        polyprobe_state.cross_return.frames[0].return_pc ||
      polyprobe_state.transition.active.caller_mode !=
        POLY_MODE_RAW_RISCV ||
      polyprobe_state.transition.active.target_mode !=
        POLY_MODE_RAW_AARCH64 ||
      polyprobe_state.transition.active.abi_kind !=
        POLY_CROSS_BRIDGE_DEFAULT ||
      polyprobe_state.transition.active.cookie !=
        polyprobe_state.cross_return.frames[0].return_sp) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: reverse live cross-return transition mismatch pc=0x%llx caller=%u target=%u abi=%u cookie=0x%llx\n",
      (unsigned long long) polyprobe_state.transition.active.return_pc,
      polyprobe_state.transition.active.caller_mode,
      polyprobe_state.transition.active.target_mode,
      polyprobe_state.transition.active.abi_kind,
      (unsigned long long) polyprobe_state.transition.active.cookie);
    return 1;
  }
  imported_cross_return_result =
    import_live_reverse_cross_return_state_probe(&polyprobe_state);
  if (imported_cross_return_result != 99) {
    fprintf(stderr, "POLY_PROBE_FAIL: reverse imported cross-return resume mismatch got=%llu\n",
      (unsigned long long) imported_cross_return_result);
    return 1;
  }
  puts("POLY_PROBE_CROSS_RETURN_XSAVE_OK");

  stage("POLY_STAGE: state-key-controls");
  if (poly_state_key_set_status(0) != 0 || poly_state_key_get() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: state key reset mismatch got=0x%llx\n",
      (unsigned long long) poly_state_key_get());
    return 1;
  }
  uint64_t aarch64_state_key = (uint64_t) (uintptr_t) &polyprobe_state;
  uint64_t riscv_state_key = aarch64_state_key + 0x80;
  if (aarch64_state_key_control_probe(aarch64_state_key) !=
        aarch64_state_key ||
      poly_state_key_get() != aarch64_state_key) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: aarch64 state key control mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) poly_state_key_get(),
      (unsigned long long) aarch64_state_key);
    poly_state_key_set_status(0);
    return 1;
  }
  memset(&polyprobe_state, 0xa5, sizeof(polyprobe_state));
  poly_state_export(&polyprobe_state);
  if (polyprobe_state.state_key.flags != POLY_STATE_KEY_FLAG_EXPLICIT ||
      polyprobe_state.state_key.explicit_key != aarch64_state_key ||
      polyprobe_state.state_key.supported_flags !=
        POLY_STATE_KEY_FLAG_EXPLICIT) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: state key export mismatch flags=0x%llx key=0x%llx supported=0x%llx expected=0x%llx\n",
      (unsigned long long) polyprobe_state.state_key.flags,
      (unsigned long long) polyprobe_state.state_key.explicit_key,
      (unsigned long long) polyprobe_state.state_key.supported_flags,
      (unsigned long long) aarch64_state_key);
    poly_state_key_set_status(0);
    return 1;
  }
  if (poly_state_key_set_status(0) != 0 || poly_state_key_get() != 0) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: state key reset after aarch64 mismatch got=0x%llx\n",
      (unsigned long long) poly_state_key_get());
    return 1;
  }
  poly_state_import(&polyprobe_state);
  if (poly_state_key_get() != aarch64_state_key) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: state key import mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) poly_state_key_get(),
      (unsigned long long) aarch64_state_key);
    poly_state_key_set_status(0);
    return 1;
  }
  if (poly_state_key_set_status(0) != 0 || poly_state_key_get() != 0) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: state key reset after import mismatch got=0x%llx\n",
      (unsigned long long) poly_state_key_get());
    return 1;
  }
  if (riscv_state_key_control_probe(riscv_state_key) != riscv_state_key ||
      poly_state_key_get() != riscv_state_key) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: riscv state key control mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) poly_state_key_get(),
      (unsigned long long) riscv_state_key);
    poly_state_key_set_status(0);
    return 1;
  }
  if (poly_state_key_set_status(0) != 0 || poly_state_key_get() != 0) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: state key final reset mismatch got=0x%llx\n",
      (unsigned long long) poly_state_key_get());
    return 1;
  }

  stage("POLY_STAGE: landing-policy");
  if (poly_landing_policy_set(0) != 0 ||
      poly_landing_policy_get() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: landing policy reset mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    return 1;
  }
  if (aarch64_landing_policy_control_probe(
        POLY_LANDING_POLICY_REQUIRE_SWITCH) !=
        POLY_LANDING_POLICY_REQUIRE_SWITCH ||
      poly_landing_policy_get() != POLY_LANDING_POLICY_REQUIRE_SWITCH) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 landing policy control mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    poly_landing_policy_set(0);
    return 1;
  }
  if (aarch64_landing_policy_invalid_probe(
        POLY_LANDING_POLICY_SUPPORTED << 1) != POLY_ERR_INVAL ||
      poly_landing_policy_get() != POLY_LANDING_POLICY_REQUIRE_SWITCH) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 landing policy invalid control mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    poly_landing_policy_set(0);
    return 1;
  }
  if (poly_landing_policy_set(0) != 0)
    return 1;
  if (riscv_landing_policy_control_probe(POLY_LANDING_POLICY_REQUIRE_CALL) !=
        POLY_LANDING_POLICY_REQUIRE_CALL ||
      poly_landing_policy_get() != POLY_LANDING_POLICY_REQUIRE_CALL) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv landing policy control mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    poly_landing_policy_set(0);
    return 1;
  }
  memset(&polyprobe_state, 0xa5, sizeof(polyprobe_state));
  poly_state_export(&polyprobe_state);
  if (polyprobe_state.landing_policy.flags !=
        POLY_LANDING_POLICY_REQUIRE_CALL ||
      polyprobe_state.landing_policy.supported_flags !=
        POLY_LANDING_POLICY_SUPPORTED) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: landing policy export mismatch flags=0x%llx supported=0x%llx\n",
      (unsigned long long) polyprobe_state.landing_policy.flags,
      (unsigned long long) polyprobe_state.landing_policy.supported_flags);
    poly_landing_policy_set(0);
    return 1;
  }
  if (poly_landing_policy_set(0) != 0 ||
      poly_landing_policy_get() != 0) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: landing policy reset before import mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    return 1;
  }
  poly_state_import(&polyprobe_state);
  if (poly_landing_policy_get() != POLY_LANDING_POLICY_REQUIRE_CALL) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: landing policy import mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    poly_landing_policy_set(0);
    return 1;
  }
  if (riscv_landing_policy_invalid_probe(
        POLY_LANDING_POLICY_SUPPORTED << 1) != POLY_ERR_INVAL ||
      poly_landing_policy_get() != POLY_LANDING_POLICY_REQUIRE_CALL) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv landing policy invalid control mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    poly_landing_policy_set(0);
    return 1;
  }
  aarch64_landing_policy_call_x86_probe();
  if (read_rax() != 61) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 landing policy x86 call mismatch got=%llu\n",
      (unsigned long long) read_rax());
    poly_landing_policy_set(0);
    return 1;
  }
  riscv_landing_policy_call_x86_probe();
  if (read_rax() != 62) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv landing policy x86 call mismatch got=%llu\n",
      (unsigned long long) read_rax());
    poly_landing_policy_set(0);
    return 1;
  }
  if (poly_landing_policy_set(0) != 0 ||
      poly_landing_policy_get() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: landing policy final reset mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    return 1;
  }

  const uint64_t invalid_control_address = 0x0100000000000000ULL;
  uint64_t x86_vector =
    (uint64_t) (uintptr_t) polyprobe_trap_vector_handler;
  uint64_t x86_packet = (uint64_t) (uintptr_t) &polyprobe_state;
  if (poly_trap_vector_set_status(x86_vector) != x86_vector ||
      poly_trap_vector_get() != x86_vector) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 trap-vector control mismatch\n");
    return 1;
  }
  if (poly_trap_vector_set_status(invalid_control_address) !=
        POLY_ERR_INVAL ||
      poly_trap_vector_get() != x86_vector) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: x86 invalid trap-vector control mismatch\n");
    return 1;
  }
  if (poly_trap_vector_mode_set_status(POLY_MODE_RAW_AARCH64) !=
        POLY_MODE_RAW_AARCH64 ||
      poly_trap_vector_mode_get() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 trap-vector mode control mismatch\n");
    return 1;
  }
  if (poly_trap_vector_mode_set_status(POLY_FRONTEND_COUNT) !=
        POLY_ERR_INVAL ||
      poly_trap_vector_mode_get() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: x86 invalid trap-vector mode control mismatch\n");
    return 1;
  }
  if (poly_monitor_packet_set_status(x86_packet) != x86_packet ||
      poly_monitor_packet_get() != x86_packet) {
    fprintf(stderr, "POLY_PROBE_FAIL: x86 monitor-packet control mismatch\n");
    return 1;
  }
  if (poly_monitor_packet_set_status(invalid_control_address) !=
        POLY_ERR_INVAL ||
      poly_monitor_packet_get() != x86_packet) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: x86 invalid monitor-packet control mismatch\n");
    return 1;
  }

  uint64_t aarch64_vector = x86_vector;
  uint64_t aarch64_packet = (uint64_t) (uintptr_t) &polyprobe_state;
  if (aarch64_foreign_control_plane_probe(aarch64_vector, aarch64_packet) !=
      aarch64_vector ||
      poly_trap_vector_get() != aarch64_vector ||
      poly_trap_vector_mode_get() != POLY_MODE_RAW_RISCV ||
      poly_monitor_packet_get() != aarch64_packet) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 foreign control-plane mismatch\n");
    return 1;
  }

  uint64_t riscv_vector = aarch64_vector + 16;
  uint64_t riscv_packet = aarch64_packet + 64;
  if (riscv_foreign_control_plane_probe(riscv_vector, riscv_packet) !=
      riscv_vector ||
      poly_trap_vector_get() != riscv_vector ||
      poly_trap_vector_mode_get() != POLY_MODE_RAW_AARCH64 ||
      poly_monitor_packet_get() != riscv_packet) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv foreign control-plane mismatch\n");
    return 1;
  }
  if (aarch64_foreign_trap_vector_invalid_probe(
        invalid_control_address) != POLY_ERR_INVAL ||
      poly_trap_vector_get() != riscv_vector) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: aarch64 invalid trap-vector control mismatch\n");
    return 1;
  }
  if (aarch64_foreign_trap_vector_mode_invalid_probe(
        POLY_FRONTEND_COUNT) != POLY_ERR_INVAL ||
      poly_trap_vector_mode_get() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: aarch64 invalid trap-vector mode control mismatch\n");
    return 1;
  }
  if (aarch64_foreign_monitor_packet_invalid_probe(
        invalid_control_address) != POLY_ERR_INVAL ||
      poly_monitor_packet_get() != riscv_packet) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: aarch64 invalid monitor-packet control mismatch\n");
    return 1;
  }
  if (riscv_foreign_trap_vector_invalid_probe(
        invalid_control_address) != POLY_ERR_INVAL ||
      poly_trap_vector_get() != riscv_vector) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: riscv invalid trap-vector control mismatch\n");
    return 1;
  }
  if (riscv_foreign_trap_vector_mode_invalid_probe(POLY_FRONTEND_COUNT) !=
        POLY_ERR_INVAL ||
      poly_trap_vector_mode_get() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: riscv invalid trap-vector mode control mismatch\n");
    return 1;
  }
  if (riscv_foreign_monitor_packet_invalid_probe(
        invalid_control_address) != POLY_ERR_INVAL ||
      poly_monitor_packet_get() != riscv_packet) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: riscv invalid monitor-packet control mismatch\n");
    return 1;
  }
  memset(&polyprobe_state, 0xa5, sizeof(polyprobe_state));
  poly_state_export(&polyprobe_state);
  if (polyprobe_state.header.trap_vector_pc != riscv_vector ||
      polyprobe_state.header.trap_vector_mode != POLY_MODE_RAW_AARCH64 ||
      polyprobe_state.header.monitor_packet_addr != riscv_packet) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: control-plane XSAVE export mismatch vector=0x%llx mode=%u packet=0x%llx\n",
      (unsigned long long) polyprobe_state.header.trap_vector_pc,
      polyprobe_state.header.trap_vector_mode,
      (unsigned long long) polyprobe_state.header.monitor_packet_addr);
    return 1;
  }
  polyprobe_state.header.trap_vector_pc = x86_vector;
  polyprobe_state.header.trap_vector_mode = POLY_MODE_X86;
  polyprobe_state.header.monitor_packet_addr = x86_packet;
  poly_state_import(&polyprobe_state);
  if (poly_trap_vector_get() != x86_vector ||
      poly_trap_vector_mode_get() != POLY_MODE_X86 ||
      poly_monitor_packet_get() != x86_packet) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: control-plane XSAVE import mismatch vector=0x%llx mode=%llu packet=0x%llx\n",
      (unsigned long long) poly_trap_vector_get(),
      (unsigned long long) poly_trap_vector_mode_get(),
      (unsigned long long) poly_monitor_packet_get());
    return 1;
  }
  poly_monitor_packet_set_value(0);
  install_polyprobe_trap_vector();

  stage("POLY_STAGE: foreign-trap-vectors");
  if (riscv_trap_to_aarch64_monitor_probe() != 177) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv trap to aarch64 monitor mismatch got=%llu\n",
      (unsigned long long) read_rax());
    install_polyprobe_trap_vector();
    return 1;
  }
  if (aarch64_trap_to_riscv_monitor_probe() != 277) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 trap to riscv monitor mismatch got=%llu\n",
      (unsigned long long) read_rax());
    install_polyprobe_trap_vector();
    return 1;
  }
  install_polyprobe_trap_vector();

  pcall_signature_aarch64_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall signature aarch64 SysV bridge mismatch\n");
    return 1;
  }
  pcall_signature_riscv_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall signature riscv SysV bridge mismatch\n");
    return 1;
  }
  pcall_signature_aarch64_exchange_probe();
  if (read_rax() != 36) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall signature aarch64 exchange bridge mismatch\n");
    return 1;
  }
  pcall_signature_riscv_exchange_probe();
  if (read_rax() != 36) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall signature riscv exchange bridge mismatch\n");
    return 1;
  }
  pcall_signature_mode_aarch64_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: generic pcall signature aarch64 bridge mismatch\n");
    return 1;
  }
  pcall_signature_mode_riscv_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: generic pcall signature riscv bridge mismatch\n");
    return 1;
  }
  pcall_signature_imm_mode_aarch64_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: immediate pcall signature aarch64 bridge mismatch\n");
    return 1;
  }
  pcall_signature_imm_mode_riscv_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: immediate pcall signature riscv bridge mismatch\n");
    return 1;
  }
  pcall_signature_imm_mode_x86_sysv_args_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: immediate pcall signature x86 bridge mismatch\n");
    return 1;
  }
  aarch64_signature_imm_call_x86_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 immediate signature pcall x86 bridge mismatch\n");
    return 1;
  }
  riscv_signature_imm_call_x86_probe();
  if (read_rax() != 21) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv immediate signature pcall x86 bridge mismatch\n");
    return 1;
  }
  aarch64_signature_imm_call_x86_stack_probe();
  if (read_rax() != 0x5141524348535431ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 signature pcall x86 stack handoff mismatch\n");
    return 1;
  }
  riscv_signature_imm_call_x86_stack_probe();
  if (read_rax() != 0x5152564353544b31ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv signature pcall x86 stack handoff mismatch\n");
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
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  pcall_signature_fp64_aarch64_probe();
  uint64_t pcall_signature_aarch64_fp64 = read_xmm0_u64();
  if (pcall_signature_aarch64_fp64 != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall signature aarch64 FP64 bridge mismatch got=0x%llx\n",
            (unsigned long long) pcall_signature_aarch64_fp64);
    return 1;
  }
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  pcall_signature_fp64_riscv_probe();
  uint64_t pcall_signature_riscv_fp64 = read_xmm0_u64();
  if (pcall_signature_riscv_fp64 != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: pcall signature riscv FP64 bridge mismatch got=0x%llx\n",
            (unsigned long long) pcall_signature_riscv_fp64);
    return 1;
  }
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  aarch64_signature_imm_call_x86_fp64_probe();
  uint64_t aarch64_call_x86_fp64 = read_xmm0_u64();
  if (aarch64_call_x86_fp64 != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 signature pcall x86 FP64 mismatch got=0x%llx\n",
            (unsigned long long) aarch64_call_x86_fp64);
    return 1;
  }
  write_xmm0_u64(0x3ff8000000000000ULL);
  write_xmm1_u64(0x4002000000000000ULL);
  riscv_signature_imm_call_x86_fp64_probe();
  uint64_t riscv_call_x86_fp64 = read_xmm0_u64();
  if (riscv_call_x86_fp64 != 0x400b000000000000ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv signature pcall x86 FP64 mismatch got=0x%llx\n",
            (unsigned long long) riscv_call_x86_fp64);
    return 1;
  }
  write_xmm0_u128(1, 2);
  write_xmm1_u128(3, 4);
  aarch64_signature_imm_call_x86_vec128_probe();
  struct polyprobe_u128 aarch64_call_x86_vec128 = read_xmm0_u128();
  if (aarch64_call_x86_vec128.lo != 4 || aarch64_call_x86_vec128.hi != 6) {
    fprintf(stderr, "POLY_PROBE_FAIL: aarch64 signature pcall x86 vec128 mismatch got=0x%llx:0x%llx\n",
            (unsigned long long) aarch64_call_x86_vec128.hi,
            (unsigned long long) aarch64_call_x86_vec128.lo);
    return 1;
  }
  write_xmm0_u128(1, 2);
  write_xmm1_u128(3, 4);
  riscv_signature_imm_call_x86_vec128_probe();
  struct polyprobe_u128 riscv_call_x86_vec128 = read_xmm0_u128();
  if (riscv_call_x86_vec128.lo != 4 || riscv_call_x86_vec128.hi != 6) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv signature pcall x86 vec128 mismatch got=0x%llx:0x%llx\n",
            (unsigned long long) riscv_call_x86_vec128.hi,
            (unsigned long long) riscv_call_x86_vec128.lo);
    return 1;
  }

  stage("POLY_STAGE: raw-barrier");
  raw_barrier_probe();
  if (read_rax() != 22) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw barrier stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: mixed-raw");
  if (raw_mixed_probe(40) != 47) {
    fprintf(stderr, "POLY_PROBE_FAIL: mixed raw instruction stream mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: switch-stress");
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t switch_accum = 0;
  for (unsigned n = 0; n < 8; n++)
    switch_accum = raw_switch_stress_step(switch_accum);
  if (switch_accum != 56) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw switch stress result mismatch\n");
    return 1;
  }
  poly_switch_count_status();
  if (read_rax() != switches_before + 32) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw switch count mismatch\n");
    return 1;
  }

  stage("POLY_STAGE: raw-break");
  const char break_string[] = "polyglot";
  volatile uint64_t break_arg = (uint64_t) (uintptr_t) break_string;
  struct polyprobe_monitor_packet monitor_packet __attribute__((aligned(64)));
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  polyprobe_current_monitor_packet = &monitor_packet;
  poly_monitor_packet_set_value((uint64_t) (uintptr_t) &monitor_packet);
  raw_aarch64_break_probe(break_arg);
  if (read_rax() != (0x4c000001ULL | ((uint64_t) POLY_MODE_RAW_AARCH64 << 8))) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 break trap vector mismatch\n");
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 break", &monitor_packet,
        POLY_TRAP_BREAK, POLY_MODE_RAW_AARCH64, 1, 1, 1) != 0)
    return 1;
  if (monitor_packet.args[0] != break_arg) {
    fprintf(stderr, "POLY_PROBE_FAIL: monitor packet aarch64 break arg0 mismatch got=%llu expected=%llu\n",
      (unsigned long long) monitor_packet.args[0],
      (unsigned long long) break_arg);
    return 1;
  }
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_riscv_break_probe(break_arg);
  if (read_rax() != (0x4c000001ULL | ((uint64_t) POLY_MODE_RAW_RISCV << 8))) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv break trap vector mismatch\n");
    return 1;
  }
  if (expect_monitor_packet_header("riscv break", &monitor_packet,
        POLY_TRAP_BREAK, POLY_MODE_RAW_RISCV, 1, 0, 1) != 0)
    return 1;
  if (monitor_packet.args[0] != break_arg) {
    fprintf(stderr, "POLY_PROBE_FAIL: monitor packet riscv break arg0 mismatch got=%llu expected=%llu\n",
      (unsigned long long) monitor_packet.args[0],
      (unsigned long long) break_arg);
    return 1;
  }
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  if (raw_aarch64_trap_restore_probe() != 51) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: aarch64 trap return state restore mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 restore break", &monitor_packet,
        POLY_TRAP_BREAK, POLY_MODE_RAW_AARCH64, 2, 2, 1) != 0)
    return 1;
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  if (raw_riscv_trap_restore_probe() != 51) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: riscv trap return state restore mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("riscv restore break", &monitor_packet,
        POLY_TRAP_BREAK, POLY_MODE_RAW_RISCV, 2, 0, 1) != 0)
    return 1;
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  if (raw_aarch64_trap_fp_restore_probe() != 0x1234) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: aarch64 trap return FP restore mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 restore fp break",
        &monitor_packet, POLY_TRAP_BREAK, POLY_MODE_RAW_AARCH64, 3, 3, 1) != 0)
    return 1;
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_riscv_trap_fp_restore_probe();
  if (read_xmm0_u64() != 0x1234) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: riscv trap return FP restore mismatch got=0x%llx\n",
      (unsigned long long) read_xmm0_u64());
    return 1;
  }
  if (expect_monitor_packet_header("riscv restore fp break", &monitor_packet,
        POLY_TRAP_BREAK, POLY_MODE_RAW_RISCV, 3, 0, 1) != 0)
    return 1;

  stage("POLY_STAGE: raw-syscall");
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_aarch64_getpid_probe();
  if (read_rax() != 4242) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 syscall mismatch\n");
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 syscall", &monitor_packet,
        POLY_TRAP_SYSCALL, POLY_MODE_RAW_AARCH64, 172, 0, 1) != 0)
    return 1;
  if (expect_monitor_packet_args("aarch64 syscall", &monitor_packet,
        polyprobe_aarch64_trap_args) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_riscv_getpid_probe();
  if (read_rax() != 4242) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv syscall mismatch\n");
    return 1;
  }
  if (expect_monitor_packet_header("riscv syscall", &monitor_packet,
        POLY_TRAP_SYSCALL, POLY_MODE_RAW_RISCV, 172, 0, 1) != 0)
    return 1;
  if (expect_monitor_packet_args("riscv syscall", &monitor_packet,
        polyprobe_riscv_syscall_args) != 0)
    return 1;

  stage("POLY_STAGE: raw-import");
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_aarch64_import_probe();
  if (read_rax() != 5555) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 import trap vector mismatch\n");
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 import", &monitor_packet,
        POLY_TRAP_IMPORT, POLY_MODE_RAW_AARCH64, 8, 0, 0) != 0)
    return 1;
  if (expect_monitor_packet_args("aarch64 import", &monitor_packet,
        polyprobe_aarch64_trap_args) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_riscv_import_probe();
  if (read_rax() != 5555) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv import trap vector mismatch\n");
    return 1;
  }
  if (expect_monitor_packet_header("riscv import", &monitor_packet,
        POLY_TRAP_IMPORT, POLY_MODE_RAW_RISCV, 8, 0, 0) != 0)
    return 1;
  if (expect_monitor_packet_args("riscv import", &monitor_packet,
        polyprobe_riscv_import_trap_args) != 0)
    return 1;

  stage("POLY_STAGE: raw-illegal");
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_aarch64_illegal_probe();
  if (read_rax() != 6666) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 illegal trap vector mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 illegal", &monitor_packet,
        POLY_TRAP_ILLEGAL, POLY_MODE_RAW_AARCH64, 0xffffffffULL, 4, 1) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_riscv_illegal_probe();
  if (read_rax() != 6666) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv illegal trap vector mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("riscv illegal", &monitor_packet,
        POLY_TRAP_ILLEGAL, POLY_MODE_RAW_RISCV, 0xffffffffULL, 4, 1) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_aarch64_invalid_branch_probe();
  if (read_rax() != 7777) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 invalid branch trap mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 invalid branch", &monitor_packet,
        POLY_TRAP_ILLEGAL, POLY_MODE_RAW_AARCH64, 0xd61f0200ULL, 4, 1) != 0)
    return 1;
  if (monitor_packet.args[0] != POLYPROBE_INVALID_AARCH64_BRANCH_TARGET) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: monitor packet aarch64 invalid branch target mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) monitor_packet.args[0],
      (unsigned long long) POLYPROBE_INVALID_AARCH64_BRANCH_TARGET);
    return 1;
  }

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_riscv_invalid_branch_probe();
  if (read_rax() != 7777) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv invalid branch trap mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("riscv invalid branch", &monitor_packet,
        POLY_TRAP_ILLEGAL, POLY_MODE_RAW_RISCV, 0x00050067ULL, 4, 1) != 0)
    return 1;
  if (monitor_packet.args[0] != POLYPROBE_INVALID_RISCV_BRANCH_TARGET) {
    fprintf(stderr,
      "POLY_PROBE_FAIL: monitor packet riscv invalid branch target mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) monitor_packet.args[0],
      (unsigned long long) POLYPROBE_INVALID_RISCV_BRANCH_TARGET);
    return 1;
  }

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_aarch64_invalid_x86_call_return_probe();
  if (read_rax() != 8888) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 invalid x86 call return trap mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 invalid x86 call return",
        &monitor_packet, POLY_TRAP_ILLEGAL, POLY_MODE_RAW_AARCH64,
        POLYPROBE_AARCH64_PCALL_SLOT3_INSN, 4, 1) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_riscv_invalid_x86_call_return_probe();
  if (read_rax() != 8888) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv invalid x86 call return trap mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("riscv invalid x86 call return",
        &monitor_packet, POLY_TRAP_ILLEGAL, POLY_MODE_RAW_RISCV,
        POLYPROBE_RISCV_PCALL_SLOT3_INSN, 4, 1) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_aarch64_invalid_cross_call_return_probe();
  if (read_rax() != 8888) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 invalid cross call return trap mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("aarch64 invalid cross call return",
        &monitor_packet, POLY_TRAP_ILLEGAL, POLY_MODE_RAW_AARCH64,
        POLYPROBE_AARCH64_PCALL_GENERIC_INSN, 4, 1) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  raw_riscv_invalid_cross_call_return_probe();
  if (read_rax() != 8888) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv invalid cross call return trap mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (expect_monitor_packet_header("riscv invalid cross call return",
        &monitor_packet, POLY_TRAP_ILLEGAL, POLY_MODE_RAW_RISCV,
        POLYPROBE_RISCV_PCALL_GENERIC_INSN, 4, 1) != 0)
    return 1;
  puts("POLY_PROBE_MONITOR_PACKETS_OK");
  poly_monitor_packet_set_value(0);
  polyprobe_current_monitor_packet = 0;

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

  poly_foreign_break_count_status();
  uint64_t breaks_before = read_rax();
  raw_aarch64_break_probe(break_arg);
  poly_foreign_break_count_status();
  if (read_rax() != breaks_before + 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw foreign break count mismatch\n");
    return 1;
  }

  puts("POLY_PROBE_OK");
  return 0;
}
