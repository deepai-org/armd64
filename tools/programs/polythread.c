#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>

#include "../include/polycpuid.h"

#define POLY_OP_ENTER_A64_WITH_TLS \
  "movl $1, %%r15d\n" \
  ".balign 4, 0x90\n" \
  POLY_X86_CTRL_PENTER_MODE_ASM
#define POLY_OP_ENTER_RV64_WITH_TLS \
  "movl $2, %%r15d\n" \
  ".balign 4, 0x90\n" \
  POLY_X86_CTRL_PENTER_MODE_ASM
#define POLY_OP_ENTER_A64 \
  "xorq %%r13, %%r13\n" \
  POLY_OP_ENTER_A64_WITH_TLS
#define POLY_OP_ENTER_RV64 \
  "xorq %%r13, %%r13\n" \
  POLY_OP_ENTER_RV64_WITH_TLS
#define POLY_OP_TRAP_RETURN POLY_X86_CTRL_TRAP_RETURN_ASM
#define POLY_OP_TRAP_VECTOR_SET POLY_X86_CTRL_TRAP_VECTOR_SET_ASM
#define POLY_OP_TRAP_VECTOR_MODE_SET POLY_X86_CTRL_TRAP_VECTOR_MODE_SET_ASM
#define POLY_OP_STATE_KEY_SET POLY_X86_CTRL_STATE_KEY_SET_ASM
#define POLY_OP_STATE_KEY_GET POLY_X86_CTRL_STATE_KEY_GET_ASM
#define POLY_OP_STATE_EXPORT POLY_X86_CTRL_STATE_EXPORT_ASM
#define POLY_OP_ABI_SIGNATURE_SET POLY_X86_CTRL_ABI_SIGNATURE_SET_ASM
#define POLY_OP_ABI_SIGNATURE_GET POLY_X86_CTRL_ABI_SIGNATURE_GET_ASM
#define POLY_OP_MONITOR_PACKET_SET POLY_X86_CTRL_MONITOR_PACKET_SET_ASM
#define POLY_OP_PCALL_SIG_IMM_NATIVE \
  POLY_X86_CTRL_PCALL_SIG_IMM_NATIVE_REGS_ASM \
  ".balign 4, 0x90\n"
#define POLY_OP_PCALL_SIG_IMM_FP64 \
  POLY_X86_CTRL_PCALL_SIG_IMM_NATIVE_REGS_FP64_ASM \
  ".balign 4, 0x90\n"
#define POLY_OP_PCALL_SYSV_A64 \
  "pushq %%rbx\n" \
  "pushq %%r15\n" \
  "pushq %%r11\n" \
  "movq %%r10, %%rbx\n" \
  "movl $1, %%r15d\n" \
  "leaq 9f(%%rip), %%r11\n" \
  POLY_X86_CTRL_PCALL_SIG_IMM_X86_SYSV_REGS_ASM \
  "9:\n" \
  "popq %%r11\n" \
  "popq %%r15\n" \
  "popq %%rbx\n" \
  "jmp *%%r11\n" \
  ".balign 4, 0x90\n"
#define POLY_OP_PCALL_SYSV_RV64 \
  "pushq %%rbx\n" \
  "pushq %%r15\n" \
  "pushq %%r11\n" \
  "movq %%r10, %%rbx\n" \
  "movl $2, %%r15d\n" \
  "leaq 9f(%%rip), %%r11\n" \
  POLY_X86_CTRL_PCALL_SIG_IMM_X86_SYSV_REGS_ASM \
  "9:\n" \
  "popq %%r11\n" \
  "popq %%r15\n" \
  "popq %%rbx\n" \
  "jmp *%%r11\n" \
  ".balign 4, 0x90\n"
#define POLY_AARCH64_PCALL_SIG_IMM_NATIVE ".long 0xd5032a7f\n"
#define POLY_AARCH64_PCALL_SIG_IMM_FP64 ".long 0xd5032b1f\n"
#define POLY_RISCV_PCALL_SIG_IMM_NATIVE ".long 0x4600700b\n"
#define POLY_RISCV_PCALL_SIG_IMM_FP64 ".long 0x5000700b\n"

enum {
  POLYTHREAD_THREADS = 4,
  POLYTHREAD_ROUNDS = 12,
  POLYTHREAD_BUSY = 20000,
  POLYTHREAD_ATOMIC_ITERS = 16,
  POLYTHREAD_YIELDS = 8
};

static pthread_barrier_t start_barrier;
static uint64_t mixed_atomic_counter __attribute__((aligned(8)));
static uint64_t explicit_state_key_counter __attribute__((aligned(8)));
static uint64_t real_xsave_context_counter __attribute__((aligned(8)));
static uint64_t real_xsave_no_key_counter __attribute__((aligned(8)));
struct polythread_monitor_packet {
  struct poly_trap_packet trap;
  uint64_t args[POLY_TRAP_PACKET_ARG_COUNT];
};

static __thread const struct polythread_monitor_packet
  *polythread_current_monitor_packet;
static __thread uint8_t polythread_state_key_anchor;

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

static inline uint64_t poly_state_key_set(uint64_t value) {
  asm volatile(POLY_OP_STATE_KEY_SET : "+a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_state_key_get(void) {
  uint64_t value;
  asm volatile(POLY_OP_STATE_KEY_GET : "=a"(value) :: "memory");
  return value;
}

static uint64_t polythread_state_key_value(void) {
  return (uint64_t) (uintptr_t) &polythread_state_key_anchor;
}

static inline uint64_t polythread_read_xcr0(void) {
  uint32_t eax;
  uint32_t edx;
  asm volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0) : "memory");
  return ((uint64_t) edx << 32) | eax;
}

static int polythread_poly_xsave_enabled(void) {
  return (polythread_read_xcr0() &
    (1ULL << POLY_STATE_XSAVE_COMPONENT_ARCH)) != 0;
}

static int polythread_select_state_key(uintptr_t worker_id,
    uint64_t expected_key, const char *phase) {
  if (expected_key == 0 ||
      poly_state_key_set(expected_key) != 0 ||
      poly_state_key_get() != expected_key) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu state-key %s set failed key=0x%llx got=0x%llx\n",
      (unsigned long) worker_id, phase,
      (unsigned long long) expected_key,
      (unsigned long long) poly_state_key_get());
    return -1;
  }
  return 0;
}

static int polythread_clear_state_key(uintptr_t worker_id) {
  if (poly_state_key_set(0) != 0 || poly_state_key_get() != 0) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu state-key clear failed got=0x%llx\n",
      (unsigned long) worker_id, (unsigned long long) poly_state_key_get());
    return -1;
  }
  return 0;
}

static int check_polythread_arch_state_contract(void) {
  struct poly_cpuid_contract_failure failure;
  if (!poly_cpuid_verify_arch_state_contract(&failure)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: %s mismatch leaf=0x%x subleaf=%u got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      failure.name, failure.leaf, failure.subleaf,
      failure.actual.eax, failure.actual.ebx,
      failure.actual.ecx, failure.actual.edx,
      failure.expected.eax, failure.expected.ebx,
      failure.expected.ecx, failure.expected.edx);
    return -1;
  }
  return 0;
}

static int check_polythread_contract(void) {
  const struct poly_cpuid_regs base = poly_read_cpuid(POLY_CPUID_BASE, 0);
  if (base.eax < POLY_CPUID_MAX || !poly_cpuid_vendor_matches(&base)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: poly CPUID missing base=(0x%x,0x%x,0x%x,0x%x)\n",
      base.eax, base.ebx, base.ecx, base.edx);
    return -1;
  }

  const struct poly_cpuid_regs features =
    poly_read_cpuid(POLY_CPUID_BASE + 1, 0);
  const uint32_t forbidden_features = poly_cpuid_forbidden_feature_mask();
  if (features.eax != POLY_CPUID_ABI_VERSION ||
      features.ebx != poly_cpuid_expected_mode_mask() ||
      features.ecx != poly_cpuid_expected_feature_mask() ||
      features.edx != POLY_STATE_XSAVE_COMPONENT_ARCH ||
      (features.ecx & forbidden_features) != 0) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: poly CPUID feature mismatch features=(%u,0x%x,0x%x,0x%x) forbidden=0x%x\n",
      features.eax, features.ebx, features.ecx, features.edx,
      forbidden_features);
    return -1;
  }

  if (check_polythread_arch_state_contract() < 0)
    return -1;

  const struct poly_cpuid_regs expected_abi_bridge =
    poly_cpuid_expected_abi_bridge_leaf();
  const struct poly_cpuid_regs abi_bridge =
    poly_read_cpuid(POLY_CPUID_BASE + 9, 0);
  const uint32_t forbidden_abi_bridge =
    poly_cpuid_forbidden_abi_bridge_mask();
  if (abi_bridge.eax != expected_abi_bridge.eax ||
      abi_bridge.ebx != expected_abi_bridge.ebx ||
      abi_bridge.ecx != expected_abi_bridge.ecx ||
      abi_bridge.edx != expected_abi_bridge.edx ||
      (abi_bridge.ebx & forbidden_abi_bridge) != 0) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: CPU ABI bridge mismatch abi=(%u,0x%x,0x%x,0x%x) forbidden=0x%x\n",
      abi_bridge.eax, abi_bridge.ebx, abi_bridge.ecx, abi_bridge.edx,
      forbidden_abi_bridge);
    return -1;
  }

  return 0;
}

static int setup_polythread_native_signature_slot(void) {
  const struct poly_cpuid_regs expected_x86_controls =
    poly_cpuid_expected_escape_leaf5();
  const struct poly_cpuid_regs expected_x86_geometry =
    poly_cpuid_expected_escape_leaf32();
  const struct poly_cpuid_regs expected_fp64_signature =
    poly_cpuid_expected_escape_leaf22();
  const struct poly_cpuid_regs expected_hfa32_ret_signature =
    poly_cpuid_expected_escape_leaf26();
  const struct poly_cpuid_regs expected_hfa32_arg_signature =
    poly_cpuid_expected_escape_leaf27();
  const struct poly_cpuid_regs expected_native_sret_signature =
    poly_cpuid_expected_escape_leaf28();
  const struct poly_cpuid_regs expected_hfa64_ret_signature =
    poly_cpuid_expected_escape_leaf29();
  const struct poly_cpuid_regs x86_controls =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 5);
  const struct poly_cpuid_regs x86_geometry =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 32);
  const struct poly_cpuid_regs signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 7);
  const struct poly_cpuid_regs fp64_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 22);
  const struct poly_cpuid_regs hfa32_ret_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 26);
  const struct poly_cpuid_regs hfa32_arg_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 27);
  const struct poly_cpuid_regs native_sret_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 28);
  const struct poly_cpuid_regs hfa64_ret_signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 29);
  const uint32_t native_slot = (signature.ecx >> 24) & 0xffU;
  const uint32_t native_kind = (signature.edx >> 24) & 0xffU;
  const uint32_t fp64_slot = fp64_signature.edx;
  const uint32_t native_sret_slot = native_sret_signature.eax;
  if (!poly_cpuid_regs_match(&x86_controls, &expected_x86_controls)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: x86 control manifest mismatch leaf5=(0x%x,0x%x,0x%x,0x%x)\n",
      x86_controls.eax, x86_controls.ebx, x86_controls.ecx,
      x86_controls.edx);
    return -1;
  }
  if (!poly_cpuid_regs_match(&x86_geometry, &expected_x86_geometry)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: x86 opcode geometry mismatch leaf32=(0x%x,0x%x,0x%x,0x%x)\n",
      x86_geometry.eax, x86_geometry.ebx, x86_geometry.ecx,
      x86_geometry.edx);
    return -1;
  }
  if (signature.eax != POLY_X86_CTRL_PCALL_SIG_IMM_BASE ||
      signature.ebx != POLY_ABI_SIGNATURE_SLOT_COUNT ||
      native_slot >= signature.ebx ||
      native_kind != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS ||
      native_slot != POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: native signature manifest mismatch sig=(0x%x,%u,0x%x,0x%x)\n",
      signature.eax, signature.ebx, signature.ecx, signature.edx);
    return -1;
  }
  if (fp64_signature.eax != expected_fp64_signature.eax ||
      fp64_signature.ebx != expected_fp64_signature.ebx ||
      fp64_signature.ecx != expected_fp64_signature.ecx ||
      fp64_signature.edx != expected_fp64_signature.edx ||
      fp64_slot >= signature.ebx ||
      fp64_slot != POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_FP64) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: FP64 signature manifest mismatch fp64=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      fp64_signature.eax, fp64_signature.ebx, fp64_signature.ecx,
      fp64_signature.edx, expected_fp64_signature.eax,
      expected_fp64_signature.ebx, expected_fp64_signature.ecx,
      expected_fp64_signature.edx);
    return -1;
  }
  if (native_sret_signature.eax != expected_native_sret_signature.eax ||
      native_sret_signature.ebx != expected_native_sret_signature.ebx ||
      native_sret_signature.ecx != expected_native_sret_signature.ecx ||
      native_sret_signature.edx != expected_native_sret_signature.edx ||
      native_sret_slot >= signature.ebx ||
      native_sret_slot != POLY_ABI_SIGNATURE_SLOT_NATIVE_SRET_REGS) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: native SRET signature manifest mismatch nsret=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      native_sret_signature.eax, native_sret_signature.ebx,
      native_sret_signature.ecx, native_sret_signature.edx,
      expected_native_sret_signature.eax,
      expected_native_sret_signature.ebx,
      expected_native_sret_signature.ecx,
      expected_native_sret_signature.edx);
    return -1;
  }
  if (hfa32_ret_signature.eax != expected_hfa32_ret_signature.eax ||
      hfa32_ret_signature.ebx != expected_hfa32_ret_signature.ebx ||
      hfa32_ret_signature.ecx != expected_hfa32_ret_signature.ecx ||
      hfa32_ret_signature.edx != expected_hfa32_ret_signature.edx) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: AArch64 HFA32 return signature manifest mismatch hfa32ret=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
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
      "POLYTHREAD_FAIL: AArch64 HFA32 argument signature manifest mismatch hfa32arg=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      hfa32_arg_signature.eax, hfa32_arg_signature.ebx,
      hfa32_arg_signature.ecx, hfa32_arg_signature.edx,
      expected_hfa32_arg_signature.eax, expected_hfa32_arg_signature.ebx,
      expected_hfa32_arg_signature.ecx, expected_hfa32_arg_signature.edx);
    return -1;
  }
  if (hfa64_ret_signature.eax != expected_hfa64_ret_signature.eax ||
      hfa64_ret_signature.ebx != expected_hfa64_ret_signature.ebx ||
      hfa64_ret_signature.ecx != expected_hfa64_ret_signature.ecx ||
      hfa64_ret_signature.edx != expected_hfa64_ret_signature.edx) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: AArch64 HFA64 return signature manifest mismatch hfa64ret=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      hfa64_ret_signature.eax, hfa64_ret_signature.ebx,
      hfa64_ret_signature.ecx, hfa64_ret_signature.edx,
      expected_hfa64_ret_signature.eax, expected_hfa64_ret_signature.ebx,
      expected_hfa64_ret_signature.ecx, expected_hfa64_ret_signature.edx);
    return -1;
  }
  uint64_t default_native_sret_kind =
    poly_abi_signature_get(native_sret_slot);
  if (default_native_sret_kind != POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: native SRET signature default got=%llu expected=%u slot=%u\n",
      (unsigned long long) default_native_sret_kind,
      POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS,
      native_sret_slot);
    return -1;
  }
  if (poly_abi_signature_set(native_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0 ||
      poly_abi_signature_set(fp64_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64) != 0 ||
      poly_abi_signature_set(native_sret_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS) != 0) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: native signature slot setup failed native=%u fp64=%u nsret=%u\n",
      native_slot, fp64_slot, native_sret_slot);
    return -1;
  }
  return 0;
}

static inline void poly_state_export(struct poly_xsave_state *state) {
  asm volatile(POLY_OP_STATE_EXPORT :: "a"(state) : "memory");
}

static inline void poly_trap_vector_set(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline void poly_trap_vector_mode_set(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
}

static inline void poly_monitor_packet_set(uint64_t value) {
  asm volatile(POLY_OP_MONITOR_PACKET_SET :: "a"(value) : "memory");
}

static int expect_monitor_packet(uintptr_t worker_id, const char *label,
    const struct polythread_monitor_packet *packet, uint32_t reason,
    uint32_t mode, uint64_t number, uint64_t selector,
    const uint64_t expected_args[POLY_TRAP_PACKET_ARG_COUNT]) {
  int args_match = 1;
  for (unsigned n = 0; n < POLY_TRAP_PACKET_ARG_COUNT; n++) {
    if (packet->args[n] != expected_args[n])
      args_match = 0;
  }
  if (packet->trap.reason != reason ||
      packet->trap.source_mode != mode ||
      packet->trap.number != number ||
      packet->trap.selector != selector ||
      packet->trap.resume_pc == 0 ||
      packet->trap.reserved[0] != 0 ||
      packet->trap.reserved[1] != 0 ||
      (packet->trap.flags & POLY_TRAP_PACKET_REQUIRED_FLAGS) !=
        POLY_TRAP_PACKET_REQUIRED_FLAGS ||
      !args_match) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu %s monitor packet reason=%u mode=%u number=%llu selector=%llu args=[%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu] expected=[%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu] resume=0x%llx flags=0x%llx\n",
      (unsigned long) worker_id, label,
      packet->trap.reason, packet->trap.source_mode,
      (unsigned long long) packet->trap.number,
      (unsigned long long) packet->trap.selector,
      (unsigned long long) packet->args[0],
      (unsigned long long) packet->args[1],
      (unsigned long long) packet->args[2],
      (unsigned long long) packet->args[3],
      (unsigned long long) packet->args[4],
      (unsigned long long) packet->args[5],
      (unsigned long long) packet->args[6],
      (unsigned long long) packet->args[7],
      (unsigned long long) expected_args[0],
      (unsigned long long) expected_args[1],
      (unsigned long long) expected_args[2],
      (unsigned long long) expected_args[3],
      (unsigned long long) expected_args[4],
      (unsigned long long) expected_args[5],
      (unsigned long long) expected_args[6],
      (unsigned long long) expected_args[7],
      (unsigned long long) packet->trap.resume_pc,
      (unsigned long long) packet->trap.flags);
    return -1;
  }

  return 0;
}

static int polythread_monitor_packet_contract_valid(
    const struct polythread_monitor_packet *packet) {
  return packet->trap.resume_pc != 0 &&
    packet->trap.reserved[0] == 0 &&
    packet->trap.reserved[1] == 0 &&
    (packet->trap.flags & POLY_TRAP_PACKET_REQUIRED_FLAGS) ==
      POLY_TRAP_PACKET_REQUIRED_FLAGS;
}

static uint64_t polythread_trap_vector_result(uint64_t number,
    const uint64_t args[POLY_TRAP_PACKET_ARG_COUNT]) {
  uint64_t result = number;
  for (unsigned n = 0; n < POLY_TRAP_PACKET_ARG_COUNT; n++)
    result += args[n];
  return result;
}

__attribute__((noinline, used))
uint64_t polythread_trap_vector_dispatch(void) {
  const struct polythread_monitor_packet *packet =
    polythread_current_monitor_packet;
  if (packet == 0)
    return (uint64_t) -1;
  if (!polythread_monitor_packet_contract_valid(packet))
    return (uint64_t) -1;
  if (packet->trap.reason != POLY_TRAP_SYSCALL &&
      packet->trap.reason != POLY_TRAP_IMPORT)
    return (uint64_t) -1;
  return polythread_trap_vector_result(packet->trap.number, packet->args);
}

__attribute__((naked, noinline, used))
static void polythread_trap_vector_handler(void) {
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
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call polythread_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
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

__attribute__((noinline, used))
static uint64_t polythread_x86_import_sum6(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  return a0 + a1 + a2 + a3 + a4 + a5;
}

static int wait_for_workers(uintptr_t worker_id, const char *phase) {
  int barrier_status = pthread_barrier_wait(&start_barrier);
  if (barrier_status != 0 && barrier_status != PTHREAD_BARRIER_SERIAL_THREAD) {
    fprintf(stderr, "POLYTHREAD_FAIL: barrier worker=%lu phase=%s status=%d\n",
      (unsigned long) worker_id, phase, barrier_status);
    return -1;
  }
  return 0;
}

static uint64_t double_to_bits(double value) {
  union {
    double d;
    uint64_t u;
  } bits;
  bits.d = value;
  return bits.u;
}

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

static uint64_t pcall_aarch64_busy(uint64_t seed) {
  uint64_t result;
  uint64_t arg0 = seed;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_A64
    "1:\n"
    ".long 0xd289c409\n" // movz x9,#20000
    ".long 0xf1000529\n" // subs x9,x9,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result), "+D"(arg0)
    :
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_riscv_busy(uint64_t seed) {
  uint64_t result;
  uint64_t arg0 = seed;
  uint64_t arg1 = POLYTHREAD_BUSY;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_RV64
    "1:\n"
    ".long 0xfff58593\n" // addi a1,a1,-1
    ".long 0xfe059ee3\n" // bnez a1,-4
    ".long 0x00150513\n" // addi a0,a0,1
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(result), "+D"(arg0), "+S"(arg1)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_sig_imm_aarch64_add1(uint64_t seed) {
  uint64_t result;
  uint64_t arg0 = seed;
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "movq %2, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_IMM_NATIVE
    "1:\n"
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    : "=a"(result), "+D"(arg0)
    : "i"(POLY_FRONTEND_AARCH64)
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_sig_imm_riscv_add1(uint64_t seed) {
  uint64_t result;
  uint64_t arg0 = seed;
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "movq %2, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_IMM_NATIVE
    "1:\n"
    ".long 0x00150513\n" // addi a0,a0,1
    ".long 0x00008067\n" // ret
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    : "=a"(result), "+D"(arg0)
    : "i"(POLY_FRONTEND_RISCV)
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_sig_imm_aarch64_fp64_mix(uint64_t left_bits,
    uint64_t right_bits) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "movq %0, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_IMM_FP64
    "1:\n"
    ".long 0x1e612800\n" // fadd d0,d0,d1
    ".long 0x1e613800\n" // fsub d0,d0,d1
    ".long 0x1e610800\n" // fmul d0,d0,d1
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_AARCH64)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_sig_imm_riscv_fp64_mix(uint64_t left_bits,
    uint64_t right_bits) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%r15\n"
    "movq %0, %%r15\n"
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SIG_IMM_FP64
    "1:\n"
    ".long 0x02b50553\n" // fadd.d fa0,fa0,fa1
    ".long 0x0ab50553\n" // fsub.d fa0,fa0,fa1
    ".long 0x12b50553\n" // fmul.d fa0,fa0,fa1
    ".long 0x00008067\n" // ret
    "2:\n"
    "popq %%r15\n"
    "popq %%rbx\n"
    :
    : "i"(POLY_FRONTEND_RISCV)
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_aarch64_hidden_busy(uint64_t seed) {
  uint64_t result;
  uint64_t arg0 = seed;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_A64
    "1:\n"
    ".long 0x91000014\n" // add x20,x0,#0
    ".long 0xd289c409\n" // movz x9,#20000
    ".long 0xf1000529\n" // subs x9,x9,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x91001e80\n" // add x0,x20,#7
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result), "+D"(arg0)
    :
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_riscv_hidden_busy(uint64_t seed) {
  uint64_t result;
  uint64_t arg0 = seed;
  uint64_t arg1 = POLYTHREAD_BUSY;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_RV64
    "1:\n"
    ".long 0x00050a13\n" // addi s4,a0,0
    ".long 0xfff58593\n" // addi a1,a1,-1
    ".long 0xfe059ee3\n" // bnez a1,-4
    ".long 0x007a0513\n" // addi a0,s4,7
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(result), "+D"(arg0), "+S"(arg1)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_aarch64_hidden_fp_busy(uint64_t left_bits,
    uint64_t right_bits) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_A64
    "1:\n"
    ".long 0x1e604014\n" // fmov d20,d0
    ".long 0xd289c409\n" // movz x9,#20000
    ".long 0xf1000529\n" // subs x9,x9,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x1e612a80\n" // fadd d0,d20,d1
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    :::
    "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
    "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_riscv_hidden_fp_busy(uint64_t left_bits,
    uint64_t right_bits) {
  uint64_t arg1 = POLYTHREAD_BUSY;
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_RV64
    "1:\n"
    ".long 0x22a50a53\n" // fsgnj.d f20,fa0,fa0
    ".long 0xfff58593\n" // addi a1,a1,-1
    ".long 0xfe059ee3\n" // bnez a1,-4
    ".long 0x02ba7553\n" // fadd.d fa0,f20,fa1
    ".long 0x00008067\n" // ret
    "2:\n"
    : "+S"(arg1)
    :
    : "rax", "rcx", "rdx", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_aarch64_hidden_fp_set(uint64_t value_bits) {
  write_xmm0_u64(value_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_A64
    "1:\n"
    ".long 0x1e604014\n" // fmov d20,d0
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    :::
    "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
    "xmm0", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_aarch64_hidden_fp_get(uint64_t addend_bits) {
  write_xmm0_u64(addend_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_A64
    "1:\n"
    ".long 0x1e602a80\n" // fadd d0,d20,d0
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    :::
    "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
    "xmm0", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_riscv_hidden_fp_set(uint64_t value_bits) {
  write_xmm0_u64(value_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_RV64
    "1:\n"
    ".long 0x22a50a53\n" // fsgnj.d f20,fa0,fa0
    ".long 0x00008067\n" // ret
    "2:\n"
    :::
    "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
    "xmm0", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_riscv_hidden_fp_get(uint64_t addend_bits) {
  write_xmm0_u64(addend_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_RV64
    "1:\n"
    ".long 0x02aa7553\n" // fadd.d fa0,f20,fa0
    ".long 0x00008067\n" // ret
    "2:\n"
    :::
    "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
    "xmm0", "memory");
  return read_xmm0_u64();
}

static uint64_t trap_aarch64_syscall(uint64_t number, uint64_t arg6,
    uint64_t arg7) {
  uint64_t result = number;
  uint64_t arg6_lane = arg6;
  uint64_t arg7_lane = arg7;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x91000008\n" // add x8,x0,#0
    ".long 0x91000026\n" // add x6,x1,#0
    ".long 0x91000047\n" // add x7,x2,#0
    ".long 0x91000501\n" // add x1,x8,#1
    ".long 0x91000902\n" // add x2,x8,#2
    ".long 0x91000d03\n" // add x3,x8,#3
    ".long 0x91001104\n" // add x4,x8,#4
    ".long 0x91001505\n" // add x5,x8,#5
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(result), "+d"(arg6_lane), "+c"(arg7_lane)
    :
    : "rbx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r13", "r14",
      "r15", "memory");
  return result;
}

static uint64_t trap_riscv_syscall(uint64_t number, uint64_t arg6) {
  uint64_t result = number;
  uint64_t arg6_lane = arg6;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00058813\n" // addi a6,a1,0
    ".long 0x00050893\n" // addi a7,a0,0
    ".long 0x00188593\n" // addi a1,a7,1
    ".long 0x00288613\n" // addi a2,a7,2
    ".long 0x00388693\n" // addi a3,a7,3
    ".long 0x00488713\n" // addi a4,a7,4
    ".long 0x00588793\n" // addi a5,a7,5
    ".long 0x00000073\n" // ecall
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(result), "+d"(arg6_lane)
    :
    : "rbx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r13",
      "r14", "r15", "memory");
  return result;
}

static uint64_t trap_aarch64_import(uint64_t arg0, uint64_t arg6,
    uint64_t arg7) {
  uint64_t result = arg0;
  uint64_t arg6_lane = arg6;
  uint64_t arg7_lane = arg7;
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_A64
    ".long 0xd29c1010\n" // movz x16,#0xe080
    ".long 0xf2bffff0\n" // movk x16,#0xffff,lsl #16
    ".long 0xf2dffff0\n" // movk x16,#0xffff,lsl #32
    ".long 0xf2fffff0\n" // movk x16,#0xffff,lsl #48
    ".long 0x91000026\n" // add x6,x1,#0
    ".long 0x91000047\n" // add x7,x2,#0
    ".long 0x91000401\n" // add x1,x0,#1
    ".long 0x91000802\n" // add x2,x0,#2
    ".long 0x91000c03\n" // add x3,x0,#3
    ".long 0x91001004\n" // add x4,x0,#4
    ".long 0x91001405\n" // add x5,x0,#5
    ".long 0xd63f0200\n" // blr x16, unresolved strlen descriptor
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(result), "+d"(arg6_lane), "+c"(arg7_lane)
    :
    : "rbx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13",
      "r14", "r15", "memory");
  return result;
}

static uint64_t trap_riscv_import(uint64_t arg0, uint64_t arg6,
    uint64_t arg7) {
  uint64_t result = arg0;
  uint64_t arg6_lane = arg6;
  uint64_t arg7_lane = arg7;
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> unresolved strlen descriptor
    ".long 0x00058813\n" // addi a6,a1,0
    ".long 0x00060893\n" // addi a7,a2,0
    ".long 0x00150593\n" // addi a1,a0,1
    ".long 0x00250613\n" // addi a2,a0,2
    ".long 0x00350693\n" // addi a3,a0,3
    ".long 0x00450713\n" // addi a4,a0,4
    ".long 0x00550793\n" // addi a5,a0,5
    ".long 0x000280e7\n" // jalr ra,0(t0)
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(result), "+d"(arg6_lane), "+c"(arg7_lane)
    :
    : "rbx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13",
      "r14", "r15", "memory");
  return result;
}

static uint64_t direct_aarch64_x86_sum6(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  register uint64_t r8_arg asm("r8") = a5;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) polythread_x86_import_sum6;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    POLY_AARCH64_PCALL_SIG_IMM_NATIVE // signature slot 3
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(a0), "+d"(a1), "+c"(a2), "+D"(a3), "+S"(a4),
      "+r"(r8_arg), "+r"(target)
    :
    : "rbx", "r9", "r11", "r12", "r13", "r14", "r15", "memory");
  return a0;
}

static uint64_t direct_riscv_x86_sum6(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  register uint64_t r8_arg asm("r8") = a5;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) polythread_x86_import_sum6;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    POLY_RISCV_PCALL_SIG_IMM_NATIVE // signature slot 3
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(a0), "+d"(a1), "+c"(a2), "+D"(a3), "+S"(a4),
      "+r"(r8_arg), "+r"(target)
    :
    : "rbx", "r9", "r11", "r12", "r13", "r14", "r15", "memory");
  return a0;
}

static uint64_t direct_aarch64_x86_signature_sum6(uint64_t a0) {
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) polythread_x86_import_sum6;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0x91000401\n" // add x1,x0,#1
    ".long 0x91000802\n" // add x2,x0,#2
    ".long 0x91000c03\n" // add x3,x0,#3
    ".long 0x91001004\n" // add x4,x0,#4
    ".long 0x91001405\n" // add x5,x0,#5
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    POLY_AARCH64_PCALL_SIG_IMM_NATIVE // signature slot 3
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(a0), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "r15", "memory");
  return a0;
}

static uint64_t direct_riscv_x86_signature_sum6(uint64_t a0) {
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) polythread_x86_import_sum6;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00150593\n" // addi a1,a0,1
    ".long 0x00250613\n" // addi a2,a0,2
    ".long 0x00350693\n" // addi a3,a0,3
    ".long 0x00450713\n" // addi a4,a0,4
    ".long 0x00550793\n" // addi a5,a0,5
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    POLY_RISCV_PCALL_SIG_IMM_NATIVE // signature slot 3
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(a0), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "r15", "memory");
  return a0;
}

static uint64_t direct_aarch64_x86_signature_fp64_mul(uint64_t left_bits,
    uint64_t right_bits) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0003f0\n" // mov x16,x0 (target)
    ".long 0xaa0103f2\n" // mov x18,x1 (return)
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    POLY_AARCH64_PCALL_SIG_IMM_FP64 // signature slot 8
    "1:\n"
    "mulsd %%xmm1, %%xmm0\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    :
    :
    : "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "r15", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t direct_riscv_x86_signature_fp64_mul(uint64_t left_bits,
    uint64_t right_bits) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%rax\n"
    "leaq 2f(%%rip), %%rdx\n"
    POLY_OP_ENTER_RV64
    ".long 0x00050293\n" // mv x5,a0 (target)
    ".long 0x00058393\n" // mv x7,a1 (return)
    ".long 0x00000313\n" // addi x6,zero,0 (x86 frontend)
    POLY_RISCV_PCALL_SIG_IMM_FP64 // signature slot 8
    "1:\n"
    "mulsd %%xmm1, %%xmm0\n"
    "retq\n"
    ".balign 4, 0x90\n"
    "2:\n"
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    :
    :
    : "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "r15", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t direct_aarch64_riscv_signature_sum6(uint64_t a0) {
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, RISC-V target from R10/P7
    ".long 0x91000401\n" // add x1,x0,#1
    ".long 0x91000802\n" // add x2,x0,#2
    ".long 0x91000c03\n" // add x3,x0,#3
    ".long 0x91001004\n" // add x4,x0,#4
    ".long 0x91001405\n" // add x5,x0,#5
    ".long 0xd2800051\n" // movz x17,#2 (RISC-V frontend)
    ".long 0x10000052\n" // adr x18,return
    POLY_AARCH64_PCALL_SIG_IMM_NATIVE // signature slot 3
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x00b50533\n" // add a0,a0,a1
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x00008067\n" // ret through hardware return cookie
    "2:\n"
    : "+a"(a0)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "r15", "memory");
  return a0;
}

static uint64_t direct_riscv_aarch64_signature_sum6(uint64_t a0) {
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, AArch64 target from R10/P7
    ".long 0x00150593\n" // addi a1,a0,1
    ".long 0x00250613\n" // addi a2,a0,2
    ".long 0x00350693\n" // addi a3,a0,3
    ".long 0x00450713\n" // addi a4,a0,4
    ".long 0x00550793\n" // addi a5,a0,5
    ".long 0x00100313\n" // addi t1,zero,1 (AArch64 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    POLY_RISCV_PCALL_SIG_IMM_NATIVE // signature slot 3
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0x8b020000\n" // add x0,x0,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0xd65f03c0\n" // ret x30 through hardware return cookie
    "2:\n"
    : "+a"(a0)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "r15", "memory");
  return a0;
}

static uint64_t direct_aarch64_riscv_signature_fp64_mix(uint64_t left_bits,
    uint64_t right_bits) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, RISC-V target from R10/P7
    ".long 0xd2800051\n" // movz x17,#2 (RISC-V frontend)
    ".long 0x10000052\n" // adr x18,return
    POLY_AARCH64_PCALL_SIG_IMM_FP64 // signature slot 8
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x02b50553\n" // fadd.d fa0,fa0,fa1
    ".long 0x0ab50553\n" // fsub.d fa0,fa0,fa1
    ".long 0x12b50553\n" // fmul.d fa0,fa0,fa1
    ".long 0x00008067\n" // ret through hardware return cookie
    "2:\n"
    :
    :
    : "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "r15", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t direct_riscv_aarch64_signature_fp64_mix(uint64_t left_bits,
    uint64_t right_bits) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, AArch64 target from R10/P7
    ".long 0x00100313\n" // addi t1,zero,1 (AArch64 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    POLY_RISCV_PCALL_SIG_IMM_FP64 // signature slot 8
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x1e612800\n" // fadd d0,d0,d1
    ".long 0x1e613800\n" // fsub d0,d0,d1
    ".long 0x1e610800\n" // fmul d0,d0,d1
    ".long 0xd65f03c0\n" // ret x30 through hardware return cookie
    "2:\n"
    :
    :
    : "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "r15", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_aarch64_hidden_set(uint64_t value) {
  uint64_t result;
  uint64_t arg0 = value;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_A64
    "1:\n"
    ".long 0x91000014\n" // add x20,x0,#0
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result), "+D"(arg0)
    :
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_aarch64_hidden_get(uint64_t addend) {
  uint64_t result;
  uint64_t arg0 = addend;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_A64
    "1:\n"
    ".long 0x8b000280\n" // add x0,x20,x0
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result), "+D"(arg0)
    :
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_riscv_hidden_set(uint64_t value) {
  uint64_t result;
  uint64_t arg0 = value;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_RV64
    "1:\n"
    ".long 0x00050a13\n" // addi s4,a0,0
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(result), "+D"(arg0)
    :
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_riscv_hidden_get(uint64_t addend) {
  uint64_t result;
  uint64_t arg0 = addend;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_RV64
    "1:\n"
    ".long 0x00aa0533\n" // add a0,s4,a0
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(result), "+D"(arg0)
    :
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

__attribute__((noinline))
static uint64_t pcall_aarch64_hidden_set_deep(uint64_t value) {
  volatile uint64_t pad[257];
  pad[0] = value;
  pad[256] = value ^ 0x5a5a5a5a5a5a5a5aULL;
  return pcall_aarch64_hidden_set(pad[0]);
}

__attribute__((noinline))
static uint64_t pcall_aarch64_hidden_get_deep(uint64_t addend) {
  volatile uint64_t pad[257];
  pad[0] = addend;
  pad[256] = addend ^ 0xa5a5a5a5a5a5a5a5ULL;
  return pcall_aarch64_hidden_get(pad[0]);
}

__attribute__((noinline))
static uint64_t pcall_riscv_hidden_set_deep(uint64_t value) {
  volatile uint64_t pad[257];
  pad[0] = value;
  pad[256] = value ^ 0x6b6b6b6b6b6b6b6bULL;
  return pcall_riscv_hidden_set(pad[0]);
}

__attribute__((noinline))
static uint64_t pcall_riscv_hidden_get_deep(uint64_t addend) {
  volatile uint64_t pad[257];
  pad[0] = addend;
  pad[256] = addend ^ 0xb6b6b6b6b6b6b6b6ULL;
  return pcall_riscv_hidden_get(pad[0]);
}

__attribute__((noinline))
static uint64_t pcall_aarch64_hidden_fp_set_deep(uint64_t value_bits) {
  volatile uint64_t pad[257];
  pad[0] = value_bits;
  pad[256] = value_bits ^ 0x3c3c3c3c3c3c3c3cULL;
  return pcall_aarch64_hidden_fp_set(pad[0]);
}

__attribute__((noinline))
static uint64_t pcall_aarch64_hidden_fp_get_deep(uint64_t addend_bits) {
  volatile uint64_t pad[257];
  pad[0] = addend_bits;
  pad[256] = addend_bits ^ 0xc3c3c3c3c3c3c3c3ULL;
  return pcall_aarch64_hidden_fp_get(pad[0]);
}

__attribute__((noinline))
static uint64_t pcall_riscv_hidden_fp_set_deep(uint64_t value_bits) {
  volatile uint64_t pad[257];
  pad[0] = value_bits;
  pad[256] = value_bits ^ 0x4d4d4d4d4d4d4d4dULL;
  return pcall_riscv_hidden_fp_set(pad[0]);
}

__attribute__((noinline))
static uint64_t pcall_riscv_hidden_fp_get_deep(uint64_t addend_bits) {
  volatile uint64_t pad[257];
  pad[0] = addend_bits;
  pad[256] = addend_bits ^ 0xd4d4d4d4d4d4d4d4ULL;
  return pcall_riscv_hidden_fp_get(pad[0]);
}

static int run_explicit_state_key_probe(uintptr_t worker_id, uint64_t base) {
  const uint64_t key = polythread_state_key_value();
  const uint64_t aarch64_seed = base + 0x5100ULL;
  const uint64_t riscv_seed = base + 0x6200ULL;
  const uint64_t aarch64_fp = double_to_bits((double) worker_id + 17.5);
  const uint64_t riscv_fp = double_to_bits((double) worker_id + 23.5);

  if (polythread_select_state_key(worker_id, key, "select") != 0)
    return -1;
  if (pcall_aarch64_hidden_set(aarch64_seed) != aarch64_seed ||
      pcall_riscv_hidden_set(riscv_seed) != riscv_seed ||
      pcall_aarch64_hidden_fp_set(aarch64_fp) != aarch64_fp ||
      pcall_riscv_hidden_fp_set(riscv_fp) != riscv_fp) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu explicit state-key state setup failed\n",
      (unsigned long) worker_id);
    return -1;
  }

  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();

  if (polythread_select_state_key(worker_id, key, "reselect") != 0)
    return -1;
  if (pcall_aarch64_hidden_get(3) != aarch64_seed + 3 ||
      pcall_riscv_hidden_get(5) != riscv_seed + 5 ||
      pcall_aarch64_hidden_fp_get(double_to_bits(3.0)) !=
        double_to_bits((double) worker_id + 20.5) ||
      pcall_riscv_hidden_fp_get(double_to_bits(5.0)) !=
        double_to_bits((double) worker_id + 28.5)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu explicit state-key state isolation failed\n",
      (unsigned long) worker_id);
    return -1;
  }
  if (polythread_clear_state_key(worker_id) != 0)
    return -1;

  __atomic_add_fetch(&explicit_state_key_counter, 1, __ATOMIC_SEQ_CST);
  return 0;
}

static int run_real_xsave_context_probe(uintptr_t worker_id, uint64_t base) {
  const uint64_t shared_key = 0x7a7b7c7d7e7f8000ULL;
  const uint64_t aarch64_seed = base + 0x125000ULL;
  const uint64_t riscv_seed = base + 0x126000ULL;
  const uint64_t aarch64_fp =
    double_to_bits((double) worker_id + 41.25);
  const uint64_t riscv_fp =
    double_to_bits((double) worker_id + 53.75);

  if (!polythread_poly_xsave_enabled())
    return 0;

  if (polythread_select_state_key(worker_id, shared_key,
        "real-xsave-shared") != 0)
    return -1;
  if (pcall_aarch64_hidden_set(aarch64_seed) != aarch64_seed ||
      pcall_riscv_hidden_set(riscv_seed) != riscv_seed ||
      pcall_aarch64_hidden_fp_set(aarch64_fp) != aarch64_fp ||
      pcall_riscv_hidden_fp_set(riscv_fp) != riscv_fp) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu real XSAVE context setup failed\n",
      (unsigned long) worker_id);
    return -1;
  }

  if (wait_for_workers(worker_id, "real-xsave-context-set") != 0)
    return -1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS * 4; n++)
    sched_yield();

  uint64_t got_aarch64 = pcall_aarch64_hidden_get(29);
  uint64_t got_riscv = pcall_riscv_hidden_get(31);
  uint64_t got_aarch64_fp =
    pcall_aarch64_hidden_fp_get(double_to_bits(29.0));
  uint64_t got_riscv_fp =
    pcall_riscv_hidden_fp_get(double_to_bits(31.0));
  if (got_aarch64 != aarch64_seed + 29 ||
      got_riscv != riscv_seed + 31 ||
      got_aarch64_fp != double_to_bits((double) worker_id + 70.25) ||
      got_riscv_fp != double_to_bits((double) worker_id + 84.75)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu real XSAVE context isolation failed "
      "a64=0x%llx/0x%llx rv=0x%llx/0x%llx "
      "a64fp=0x%llx/0x%llx rvfp=0x%llx/0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) got_aarch64,
      (unsigned long long) (aarch64_seed + 29),
      (unsigned long long) got_riscv,
      (unsigned long long) (riscv_seed + 31),
      (unsigned long long) got_aarch64_fp,
      (unsigned long long) double_to_bits((double) worker_id + 70.25),
      (unsigned long long) got_riscv_fp,
      (unsigned long long) double_to_bits((double) worker_id + 84.75));
    return -1;
  }

  if (wait_for_workers(worker_id, "real-xsave-context-checked") != 0)
    return -1;
  if (polythread_clear_state_key(worker_id) != 0)
    return -1;

  __atomic_add_fetch(&real_xsave_context_counter, 1, __ATOMIC_SEQ_CST);
  return 0;
}

static int run_real_xsave_no_key_probe(uintptr_t worker_id, uint64_t base) {
  const uint64_t aarch64_seed = base + 0x127000ULL;
  const uint64_t riscv_seed = base + 0x128000ULL;
  const uint64_t aarch64_fp =
    double_to_bits((double) worker_id + 89.5);
  const uint64_t riscv_fp =
    double_to_bits((double) worker_id + 97.5);

  if (!polythread_poly_xsave_enabled())
    return 0;

  if (polythread_clear_state_key(worker_id) != 0)
    return -1;
  if (pcall_aarch64_hidden_set(aarch64_seed) != aarch64_seed ||
      pcall_riscv_hidden_set(riscv_seed) != riscv_seed ||
      pcall_aarch64_hidden_fp_set(aarch64_fp) != aarch64_fp ||
      pcall_riscv_hidden_fp_set(riscv_fp) != riscv_fp) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu real XSAVE no-key setup failed\n",
      (unsigned long) worker_id);
    return -1;
  }

  if (wait_for_workers(worker_id, "real-xsave-no-key-set") != 0)
    return -1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS * 4; n++)
    sched_yield();

  uint64_t got_aarch64 = pcall_aarch64_hidden_get(37);
  uint64_t got_riscv = pcall_riscv_hidden_get(41);
  uint64_t got_aarch64_fp =
    pcall_aarch64_hidden_fp_get(double_to_bits(37.0));
  uint64_t got_riscv_fp =
    pcall_riscv_hidden_fp_get(double_to_bits(41.0));
  if (got_aarch64 != aarch64_seed + 37 ||
      got_riscv != riscv_seed + 41 ||
      got_aarch64_fp != double_to_bits((double) worker_id + 126.5) ||
      got_riscv_fp != double_to_bits((double) worker_id + 138.5)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu real XSAVE no-key isolation failed "
      "a64=0x%llx/0x%llx rv=0x%llx/0x%llx "
      "a64fp=0x%llx/0x%llx rvfp=0x%llx/0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) got_aarch64,
      (unsigned long long) (aarch64_seed + 37),
      (unsigned long long) got_riscv,
      (unsigned long long) (riscv_seed + 41),
      (unsigned long long) got_aarch64_fp,
      (unsigned long long) double_to_bits((double) worker_id + 126.5),
      (unsigned long long) got_riscv_fp,
      (unsigned long long) double_to_bits((double) worker_id + 138.5));
    return -1;
  }

  if (wait_for_workers(worker_id, "real-xsave-no-key-checked") != 0)
    return -1;

  __atomic_add_fetch(&real_xsave_no_key_counter, 1, __ATOMIC_SEQ_CST);
  return 0;
}

static int check_exported_thread_state(uintptr_t worker_id,
    uint64_t expected_aarch64_gpr, uint64_t expected_riscv_gpr,
    uint64_t expected_aarch64_fp, uint64_t expected_riscv_fp) {
  struct poly_xsave_state snapshot __attribute__((aligned(64)));
  poly_state_export(&snapshot);
  if (snapshot.aarch64_gpr[20] != expected_aarch64_gpr ||
      snapshot.riscv_gpr[20] != expected_riscv_gpr ||
      snapshot.aarch64_fp[20].lo != expected_aarch64_fp ||
      snapshot.riscv_fp[20].lo != expected_riscv_fp) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu exported state a64x20=0x%llx/0x%llx rvx20=0x%llx/0x%llx a64d20=0x%llx/0x%llx rvf20=0x%llx/0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) snapshot.aarch64_gpr[20],
      (unsigned long long) expected_aarch64_gpr,
      (unsigned long long) snapshot.riscv_gpr[20],
      (unsigned long long) expected_riscv_gpr,
      (unsigned long long) snapshot.aarch64_fp[20].lo,
      (unsigned long long) expected_aarch64_fp,
      (unsigned long long) snapshot.riscv_fp[20].lo,
      (unsigned long long) expected_riscv_fp);
    return -1;
  }
  return 0;
}

static void x86_atomic_add(uint64_t *ptr, uint64_t iterations) {
  for (uint64_t n = 0; n < iterations; n++)
    __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST);
}

static void pcall_aarch64_atomic_add(uint64_t *ptr, uint64_t iterations) {
  uint64_t ignored;
  uint64_t arg0 = (uint64_t) (uintptr_t) ptr;
  uint64_t arg1 = iterations;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_A64
    "1:\n"
    ".long 0xd2800022\n" // mov x2,#1
    ".long 0xd5033fbf\n" // dmb sy
    ".long 0xd5033f9f\n" // dsb sy
    ".long 0xd5033fdf\n" // isb
    ".long 0xf8e2001f\n" // ldaddal x2,xzr,[x0]
    ".long 0xf1000421\n" // subs x1,x1,#1
    ".long 0x54ffffc1\n" // b.ne -8
    ".long 0xd5033fbf\n" // dmb sy
    ".long 0xd5033f9f\n" // dsb sy
    ".long 0xd5033fdf\n" // isb
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(ignored), "+D"(arg0), "+S"(arg1)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
}

static void pcall_riscv_atomic_add(uint64_t *ptr, uint64_t iterations) {
  uint64_t ignored;
  uint64_t arg0 = (uint64_t) (uintptr_t) ptr;
  uint64_t arg1 = iterations;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    POLY_OP_PCALL_SYSV_RV64
    "1:\n"
    ".long 0x00100313\n" // addi t1,zero,1
    ".long 0x0ff0000f\n" // fence
    ".long 0x0000100f\n" // fence.i
    ".long 0x0665302f\n" // amoadd.d.aqrl zero,t1,(a0)
    ".long 0xfff58593\n" // addi a1,a1,-1
    ".long 0xfe059ce3\n" // bnez a1,-8
    ".long 0x0ff0000f\n" // fence
    ".long 0x0000100f\n" // fence.i
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(ignored), "+D"(arg0), "+S"(arg1)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
}

static void *worker_main(void *arg) {
  uintptr_t worker_id = (uintptr_t) arg;
  uint64_t base = 0x10000000ULL + worker_id * 0x10000ULL;

  if (wait_for_workers(worker_id, "start") != 0)
    return (void *) 1;
  if (run_explicit_state_key_probe(worker_id, base) != 0)
    return (void *) 1;
  if (wait_for_workers(worker_id, "explicit-state-key") != 0)
    return (void *) 1;
  if (run_real_xsave_context_probe(worker_id, base) != 0)
    return (void *) 1;
  if (wait_for_workers(worker_id, "real-xsave-context") != 0)
    return (void *) 1;
  if (run_real_xsave_no_key_probe(worker_id, base) != 0)
    return (void *) 1;
  if (wait_for_workers(worker_id, "real-xsave-no-key") != 0)
    return (void *) 1;

  if (setup_polythread_native_signature_slot() != 0) {
    fprintf(stderr, "POLYTHREAD_FAIL: worker=%lu native ABI signature setup failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  uint64_t sig_imm_seed = base + 0x12000ULL;
  uint64_t sig_imm_aarch64_result =
    pcall_sig_imm_aarch64_add1(sig_imm_seed);
  if (sig_imm_aarch64_result != sig_imm_seed + 1) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu sig-slot aarch64 got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) sig_imm_aarch64_result,
      (unsigned long long) (sig_imm_seed + 1));
    return (void *) 1;
  }
  uint64_t sig_imm_riscv_result =
    pcall_sig_imm_riscv_add1(sig_imm_seed + 1);
  if (sig_imm_riscv_result != sig_imm_seed + 2) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu sig-slot riscv got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) sig_imm_riscv_result,
      (unsigned long long) (sig_imm_seed + 2));
    return (void *) 1;
  }
  uint64_t sig_fp_left = double_to_bits(1.5);
  uint64_t sig_fp_right = double_to_bits(2.25);
  uint64_t sig_fp_expected = double_to_bits(3.375);
  uint64_t sig_fp_aarch64_result =
    pcall_sig_imm_aarch64_fp64_mix(sig_fp_left, sig_fp_right);
  if (sig_fp_aarch64_result != sig_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu sig-slot aarch64 fp got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) sig_fp_aarch64_result,
      (unsigned long long) sig_fp_expected);
    return (void *) 1;
  }
  uint64_t sig_fp_riscv_result =
    pcall_sig_imm_riscv_fp64_mix(sig_fp_left, sig_fp_right);
  if (sig_fp_riscv_result != sig_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu sig-slot riscv fp got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) sig_fp_riscv_result,
      (unsigned long long) sig_fp_expected);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "native-sig-slot-pcall") != 0)
    return (void *) 1;

  uint64_t default_aarch64_seed = base + 0x20000ULL;
  uint64_t default_riscv_seed = base + 0x30000ULL;
  uint64_t default_aarch64_fp_seed = base + 0x34000ULL;
  uint64_t default_riscv_fp_seed = base + 0x35000ULL;
  if (pcall_aarch64_hidden_set(default_aarch64_seed) !=
      default_aarch64_seed) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 hidden set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_riscv_hidden_set(default_riscv_seed) != default_riscv_seed) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv hidden set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_aarch64_hidden_fp_set(
      double_to_bits((double) default_aarch64_fp_seed)) !=
      double_to_bits((double) default_aarch64_fp_seed)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 hidden fp set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_riscv_hidden_fp_set(
      double_to_bits((double) default_riscv_fp_seed)) !=
      double_to_bits((double) default_riscv_fp_seed)) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv hidden fp set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-hidden-set") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  uint64_t default_aarch64_result = pcall_aarch64_hidden_get(9);
  if (default_aarch64_result != default_aarch64_seed + 9) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 state got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) default_aarch64_result,
      (unsigned long long) (default_aarch64_seed + 9));
    return (void *) 1;
  }
  uint64_t default_riscv_result = pcall_riscv_hidden_get(11);
  if (default_riscv_result != default_riscv_seed + 11) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv state got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) default_riscv_result,
      (unsigned long long) (default_riscv_seed + 11));
    return (void *) 1;
  }
  uint64_t default_aarch64_fp_result =
    pcall_aarch64_hidden_fp_get(double_to_bits(9.0));
  uint64_t default_aarch64_fp_expected =
    double_to_bits((double) default_aarch64_fp_seed + 9.0);
  if (default_aarch64_fp_result != default_aarch64_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 fp state got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) default_aarch64_fp_result,
      (unsigned long long) default_aarch64_fp_expected);
    return (void *) 1;
  }
  uint64_t default_riscv_fp_result =
    pcall_riscv_hidden_fp_get(double_to_bits(11.0));
  uint64_t default_riscv_fp_expected =
    double_to_bits((double) default_riscv_fp_seed + 11.0);
  if (default_riscv_fp_result != default_riscv_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv fp state got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) default_riscv_fp_result,
      (unsigned long long) default_riscv_fp_expected);
    return (void *) 1;
  }
  if (check_exported_thread_state(worker_id, default_aarch64_seed,
      default_riscv_seed, double_to_bits((double) default_aarch64_fp_seed),
      double_to_bits((double) default_riscv_fp_seed)) != 0)
    return (void *) 1;

  uint64_t stack_aarch64_seed = base + 0x36000ULL;
  uint64_t stack_riscv_seed = base + 0x37000ULL;
  uint64_t stack_aarch64_fp_seed = base + 0x38000ULL;
  uint64_t stack_riscv_fp_seed = base + 0x39000ULL;
  uint64_t stack_aarch64_fp_bits =
    double_to_bits((double) stack_aarch64_fp_seed);
  uint64_t stack_riscv_fp_bits =
    double_to_bits((double) stack_riscv_fp_seed);
  if (pcall_aarch64_hidden_set_deep(stack_aarch64_seed) !=
      stack_aarch64_seed) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu deep aarch64 hidden set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_riscv_hidden_set_deep(stack_riscv_seed) != stack_riscv_seed) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu deep riscv hidden set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_aarch64_hidden_fp_set_deep(stack_aarch64_fp_bits) !=
      stack_aarch64_fp_bits) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu deep aarch64 hidden fp set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_riscv_hidden_fp_set_deep(stack_riscv_fp_bits) !=
      stack_riscv_fp_bits) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu deep riscv hidden fp set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "stack-depth-hidden-set") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  uint64_t stack_aarch64_result = pcall_aarch64_hidden_get(13);
  if (stack_aarch64_result != stack_aarch64_seed + 13) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-depth aarch64 state got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) stack_aarch64_result,
      (unsigned long long) (stack_aarch64_seed + 13));
    return (void *) 1;
  }
  uint64_t stack_riscv_result = pcall_riscv_hidden_get(17);
  if (stack_riscv_result != stack_riscv_seed + 17) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-depth riscv state got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) stack_riscv_result,
      (unsigned long long) (stack_riscv_seed + 17));
    return (void *) 1;
  }
  uint64_t stack_aarch64_fp_result =
    pcall_aarch64_hidden_fp_get(double_to_bits(13.0));
  uint64_t stack_aarch64_fp_expected =
    double_to_bits((double) stack_aarch64_fp_seed + 13.0);
  if (stack_aarch64_fp_result != stack_aarch64_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-depth aarch64 fp state got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) stack_aarch64_fp_result,
      (unsigned long long) stack_aarch64_fp_expected);
    return (void *) 1;
  }
  uint64_t stack_riscv_fp_result =
    pcall_riscv_hidden_fp_get(double_to_bits(17.0));
  uint64_t stack_riscv_fp_expected =
    double_to_bits((double) stack_riscv_fp_seed + 17.0);
  if (stack_riscv_fp_result != stack_riscv_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-depth riscv fp state got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) stack_riscv_fp_result,
      (unsigned long long) stack_riscv_fp_expected);
    return (void *) 1;
  }

  stack_aarch64_seed = base + 0x3a000ULL;
  stack_riscv_seed = base + 0x3b000ULL;
  stack_aarch64_fp_seed = base + 0x3c000ULL;
  stack_riscv_fp_seed = base + 0x3d000ULL;
  stack_aarch64_fp_bits = double_to_bits((double) stack_aarch64_fp_seed);
  stack_riscv_fp_bits = double_to_bits((double) stack_riscv_fp_seed);
  if (pcall_aarch64_hidden_set(stack_aarch64_seed) != stack_aarch64_seed) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack reset aarch64 hidden set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_riscv_hidden_set(stack_riscv_seed) != stack_riscv_seed) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack reset riscv hidden set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_aarch64_hidden_fp_set(stack_aarch64_fp_bits) !=
      stack_aarch64_fp_bits) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack reset aarch64 hidden fp set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (pcall_riscv_hidden_fp_set(stack_riscv_fp_bits) !=
      stack_riscv_fp_bits) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack reset riscv hidden fp set failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "stack-depth-hidden-reset") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  stack_aarch64_result = pcall_aarch64_hidden_get_deep(19);
  if (stack_aarch64_result != stack_aarch64_seed + 19) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-reset aarch64 state got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) stack_aarch64_result,
      (unsigned long long) (stack_aarch64_seed + 19));
    return (void *) 1;
  }
  stack_riscv_result = pcall_riscv_hidden_get_deep(23);
  if (stack_riscv_result != stack_riscv_seed + 23) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-reset riscv state got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) stack_riscv_result,
      (unsigned long long) (stack_riscv_seed + 23));
    return (void *) 1;
  }
  stack_aarch64_fp_result =
    pcall_aarch64_hidden_fp_get_deep(double_to_bits(19.0));
  stack_aarch64_fp_expected =
    double_to_bits((double) stack_aarch64_fp_seed + 19.0);
  if (stack_aarch64_fp_result != stack_aarch64_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-reset aarch64 fp state got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) stack_aarch64_fp_result,
      (unsigned long long) stack_aarch64_fp_expected);
    return (void *) 1;
  }
  stack_riscv_fp_result =
    pcall_riscv_hidden_fp_get_deep(double_to_bits(23.0));
  stack_riscv_fp_expected =
    double_to_bits((double) stack_riscv_fp_seed + 23.0);
  if (stack_riscv_fp_result != stack_riscv_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-reset riscv fp state got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) stack_riscv_fp_result,
      (unsigned long long) stack_riscv_fp_expected);
    return (void *) 1;
  }
  if (check_exported_thread_state(worker_id, stack_aarch64_seed,
      stack_riscv_seed, stack_aarch64_fp_bits, stack_riscv_fp_bits) != 0)
    return (void *) 1;

  poly_trap_vector_mode_set(POLY_MODE_X86);
  poly_trap_vector_set(
    (uint64_t) (uintptr_t) polythread_trap_vector_handler);
  struct polythread_monitor_packet monitor_packet __attribute__((aligned(64)));
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  polythread_current_monitor_packet = &monitor_packet;
  poly_monitor_packet_set((uint64_t) (uintptr_t) &monitor_packet);

  uint64_t aarch64_trap_number = 200 + worker_id;
  uint64_t aarch64_trap_arg6 = base + 0x40000ULL;
  uint64_t aarch64_trap_arg7 = base + 0x50000ULL;
  const uint64_t aarch64_trap_args[POLY_TRAP_PACKET_ARG_COUNT] = {
    aarch64_trap_number, aarch64_trap_number + 1,
    aarch64_trap_number + 2, aarch64_trap_number + 3,
    aarch64_trap_number + 4, aarch64_trap_number + 5,
    aarch64_trap_arg6, aarch64_trap_arg7
  };
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  uint64_t aarch64_trap_result = trap_aarch64_syscall(aarch64_trap_number,
    aarch64_trap_arg6, aarch64_trap_arg7);
  uint64_t aarch64_trap_expected =
    polythread_trap_vector_result(aarch64_trap_number, aarch64_trap_args);
  if (aarch64_trap_result != aarch64_trap_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 trap result got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) aarch64_trap_result,
      (unsigned long long) aarch64_trap_expected);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-aarch64-trap") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  if (expect_monitor_packet(worker_id, "default aarch64 trap",
      &monitor_packet, POLY_TRAP_SYSCALL, POLY_MODE_RAW_AARCH64,
      aarch64_trap_number, 7, aarch64_trap_args) != 0)
    return (void *) 1;

  uint64_t riscv_trap_number = 300 + worker_id;
  uint64_t riscv_trap_arg6 = base + 0x60000ULL;
  const uint64_t riscv_trap_args[POLY_TRAP_PACKET_ARG_COUNT] = {
    riscv_trap_number, riscv_trap_number + 1, riscv_trap_number + 2,
    riscv_trap_number + 3, riscv_trap_number + 4, riscv_trap_number + 5,
    riscv_trap_arg6, riscv_trap_number
  };
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  uint64_t riscv_trap_result =
    trap_riscv_syscall(riscv_trap_number, riscv_trap_arg6);
  uint64_t riscv_trap_expected =
    polythread_trap_vector_result(riscv_trap_number, riscv_trap_args);
  if (riscv_trap_result != riscv_trap_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv trap result got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) riscv_trap_result,
      (unsigned long long) riscv_trap_expected);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-riscv-trap") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  if (expect_monitor_packet(worker_id, "default riscv trap", &monitor_packet,
      POLY_TRAP_SYSCALL, POLY_MODE_RAW_RISCV, riscv_trap_number, 0,
      riscv_trap_args) != 0)
    return (void *) 1;

  uint64_t import_id = 8;
  uint64_t aarch64_import_arg6 = base + 0x70000ULL;
  uint64_t aarch64_import_arg7 = base + 0x80000ULL;
  uint64_t aarch64_import_arg0 = base + 0x90000ULL;
  const uint64_t aarch64_import_args[POLY_TRAP_PACKET_ARG_COUNT] = {
    aarch64_import_arg0, aarch64_import_arg0 + 1, aarch64_import_arg0 + 2,
    aarch64_import_arg0 + 3, aarch64_import_arg0 + 4,
    aarch64_import_arg0 + 5, aarch64_import_arg6, aarch64_import_arg7
  };
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  uint64_t aarch64_import_result = trap_aarch64_import(
    aarch64_import_arg0, aarch64_import_arg6, aarch64_import_arg7);
  uint64_t aarch64_import_expected =
    polythread_trap_vector_result(import_id, aarch64_import_args);
  if (aarch64_import_result != aarch64_import_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 import result got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) aarch64_import_result,
      (unsigned long long) aarch64_import_expected);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-aarch64-import") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  if (expect_monitor_packet(worker_id, "default aarch64 import",
      &monitor_packet, POLY_TRAP_IMPORT, POLY_MODE_RAW_AARCH64, import_id,
      0, aarch64_import_args) != 0)
    return (void *) 1;

  uint64_t riscv_import_arg6 = base + 0xa0000ULL;
  uint64_t riscv_import_arg7 = base + 0xb0000ULL;
  uint64_t riscv_import_arg0 = base + 0xc0000ULL;
  const uint64_t riscv_import_args[POLY_TRAP_PACKET_ARG_COUNT] = {
    riscv_import_arg0, riscv_import_arg0 + 1, riscv_import_arg0 + 2,
    riscv_import_arg0 + 3, riscv_import_arg0 + 4, riscv_import_arg0 + 5,
    riscv_import_arg6, riscv_import_arg7
  };
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  uint64_t riscv_import_result = trap_riscv_import(riscv_import_arg0,
    riscv_import_arg6, riscv_import_arg7);
  uint64_t riscv_import_expected =
    polythread_trap_vector_result(import_id, riscv_import_args);
  if (riscv_import_result != riscv_import_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv import result got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) riscv_import_result,
      (unsigned long long) riscv_import_expected);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-riscv-import") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  if (expect_monitor_packet(worker_id, "default riscv import",
      &monitor_packet, POLY_TRAP_IMPORT, POLY_MODE_RAW_RISCV, import_id, 0,
      riscv_import_args) != 0)
    return (void *) 1;
  poly_monitor_packet_set(0);
  polythread_current_monitor_packet = 0;

  if (wait_for_workers(worker_id, "default-hidden-checked") != 0)
    return (void *) 1;

  if (wait_for_workers(worker_id, "direct-x86-call-start") != 0)
    return (void *) 1;
  uint64_t direct_aarch64_base = base + 0xd0000ULL;
  uint64_t direct_aarch64_result = direct_aarch64_x86_sum6(
    direct_aarch64_base + 1, direct_aarch64_base + 2,
    direct_aarch64_base + 3, direct_aarch64_base + 4,
    direct_aarch64_base + 5, direct_aarch64_base + 6);
  uint64_t direct_aarch64_expected =
    direct_aarch64_base * 6 + 21;
  if (direct_aarch64_result != direct_aarch64_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct aarch64 x86 call got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_aarch64_result,
      (unsigned long long) direct_aarch64_expected);
    return (void *) 1;
  }

  uint64_t direct_riscv_base = base + 0xe0000ULL;
  uint64_t direct_riscv_result = direct_riscv_x86_sum6(
    direct_riscv_base + 1, direct_riscv_base + 2,
    direct_riscv_base + 3, direct_riscv_base + 4,
    direct_riscv_base + 5, direct_riscv_base + 6);
  uint64_t direct_riscv_expected = direct_riscv_base * 6 + 21;
  if (direct_riscv_result != direct_riscv_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct riscv x86 call got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_riscv_result,
      (unsigned long long) direct_riscv_expected);
    return (void *) 1;
  }

  uint64_t direct_sig_aarch64_arg0 = base + 0xf0000ULL + 1;
  uint64_t direct_sig_aarch64_result =
    direct_aarch64_x86_signature_sum6(direct_sig_aarch64_arg0);
  uint64_t direct_sig_aarch64_expected =
    direct_sig_aarch64_arg0 * 6 + 15;
  if (direct_sig_aarch64_result != direct_sig_aarch64_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct sig aarch64 x86 call got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_sig_aarch64_result,
      (unsigned long long) direct_sig_aarch64_expected);
    return (void *) 1;
  }

  uint64_t direct_sig_riscv_arg0 = base + 0x100000ULL + 1;
  uint64_t direct_sig_riscv_result =
    direct_riscv_x86_signature_sum6(direct_sig_riscv_arg0);
  uint64_t direct_sig_riscv_expected = direct_sig_riscv_arg0 * 6 + 15;
  if (direct_sig_riscv_result != direct_sig_riscv_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct sig riscv x86 call got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_sig_riscv_result,
      (unsigned long long) direct_sig_riscv_expected);
    return (void *) 1;
  }
  uint64_t direct_sig_aarch64_fp_result =
    direct_aarch64_x86_signature_fp64_mul(sig_fp_left, sig_fp_right);
  if (direct_sig_aarch64_fp_result != sig_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct sig aarch64 x86 fp got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_sig_aarch64_fp_result,
      (unsigned long long) sig_fp_expected);
    return (void *) 1;
  }

  uint64_t direct_sig_riscv_fp_result =
    direct_riscv_x86_signature_fp64_mul(sig_fp_left, sig_fp_right);
  if (direct_sig_riscv_fp_result != sig_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct sig riscv x86 fp got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_sig_riscv_fp_result,
      (unsigned long long) sig_fp_expected);
    return (void *) 1;
  }

  uint64_t direct_sig_aarch64_riscv_arg0 = base + 0x110000ULL + 1;
  uint64_t direct_sig_aarch64_riscv_result =
    direct_aarch64_riscv_signature_sum6(direct_sig_aarch64_riscv_arg0);
  uint64_t direct_sig_aarch64_riscv_expected =
    direct_sig_aarch64_riscv_arg0 * 6 + 15;
  if (direct_sig_aarch64_riscv_result !=
      direct_sig_aarch64_riscv_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct sig aarch64 riscv call got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_sig_aarch64_riscv_result,
      (unsigned long long) direct_sig_aarch64_riscv_expected);
    return (void *) 1;
  }

  uint64_t direct_sig_riscv_aarch64_arg0 = base + 0x120000ULL + 1;
  uint64_t direct_sig_riscv_aarch64_result =
    direct_riscv_aarch64_signature_sum6(direct_sig_riscv_aarch64_arg0);
  uint64_t direct_sig_riscv_aarch64_expected =
    direct_sig_riscv_aarch64_arg0 * 6 + 15;
  if (direct_sig_riscv_aarch64_result !=
      direct_sig_riscv_aarch64_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct sig riscv aarch64 call got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_sig_riscv_aarch64_result,
      (unsigned long long) direct_sig_riscv_aarch64_expected);
    return (void *) 1;
  }

  uint64_t direct_sig_aarch64_riscv_fp_result =
    direct_aarch64_riscv_signature_fp64_mix(sig_fp_left, sig_fp_right);
  if (direct_sig_aarch64_riscv_fp_result != sig_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct sig aarch64 riscv fp got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_sig_aarch64_riscv_fp_result,
      (unsigned long long) sig_fp_expected);
    return (void *) 1;
  }

  uint64_t direct_sig_riscv_aarch64_fp_result =
    direct_riscv_aarch64_signature_fp64_mix(sig_fp_left, sig_fp_right);
  if (direct_sig_riscv_aarch64_fp_result != sig_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu direct sig riscv aarch64 fp got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) direct_sig_riscv_aarch64_fp_result,
      (unsigned long long) sig_fp_expected);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "direct-x86-call-done") != 0)
    return (void *) 1;

  if (wait_for_workers(worker_id, "thread-state-bound") != 0)
    return (void *) 1;

  const uint64_t signature_slot = 5;
  const uint64_t signature_kind =
    worker_id == 0 ? POLY_ABI_SIGNATURE_KIND_EXCHANGE :
    worker_id == 1 ? POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS :
    worker_id == 2 ? POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_I128 :
    POLY_ABI_SIGNATURE_KIND_NATIVE_REGS;
  if (poly_abi_signature_set(signature_slot, signature_kind) != 0) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu ABI signature set slot=%llu kind=%llu failed\n",
      (unsigned long) worker_id,
      (unsigned long long) signature_slot,
      (unsigned long long) signature_kind);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "abi-signature-set") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  uint64_t got_signature_kind = poly_abi_signature_get(signature_slot);
  if (got_signature_kind != signature_kind) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu ABI signature slot leak got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) got_signature_kind,
      (unsigned long long) signature_kind);
    return (void *) 1;
  }

  for (unsigned round = 0; round < POLYTHREAD_ROUNDS; round++) {
    uint64_t aarch64_seed = base + round * 2;
    uint64_t riscv_seed = base + round * 2 + 1;
    uint64_t hidden_aarch64_seed = base + 0x4000ULL + round * 2;
    uint64_t hidden_riscv_seed = base + 0x4000ULL + round * 2 + 1;
    uint64_t hidden_aarch64_fp_seed = base + 0x8000ULL + round * 2;
    uint64_t hidden_riscv_fp_seed = base + 0x8000ULL + round * 2 + 1;
    uint64_t seven_bits = double_to_bits(7.0);
    uint64_t aarch64_result = pcall_aarch64_busy(aarch64_seed);
    uint64_t riscv_result = pcall_riscv_busy(riscv_seed);
    uint64_t hidden_aarch64_result =
      pcall_aarch64_hidden_busy(hidden_aarch64_seed);
    uint64_t hidden_riscv_result =
      pcall_riscv_hidden_busy(hidden_riscv_seed);
    uint64_t hidden_aarch64_fp_result = pcall_aarch64_hidden_fp_busy(
      double_to_bits((double) hidden_aarch64_fp_seed), seven_bits);
    uint64_t hidden_riscv_fp_result = pcall_riscv_hidden_fp_busy(
      double_to_bits((double) hidden_riscv_fp_seed), seven_bits);

    if (aarch64_result != aarch64_seed + 1) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: worker=%lu arch=aarch64 round=%u got=%llu expected=%llu\n",
        (unsigned long) worker_id, round,
        (unsigned long long) aarch64_result,
        (unsigned long long) (aarch64_seed + 1));
      return (void *) 1;
    }
    if (riscv_result != riscv_seed + 1) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: worker=%lu arch=riscv round=%u got=%llu expected=%llu\n",
        (unsigned long) worker_id, round,
        (unsigned long long) riscv_result,
        (unsigned long long) (riscv_seed + 1));
      return (void *) 1;
    }
    if (hidden_aarch64_result != hidden_aarch64_seed + 7) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: worker=%lu arch=aarch64-hidden round=%u got=%llu expected=%llu\n",
        (unsigned long) worker_id, round,
        (unsigned long long) hidden_aarch64_result,
        (unsigned long long) (hidden_aarch64_seed + 7));
      return (void *) 1;
    }
    if (hidden_riscv_result != hidden_riscv_seed + 7) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: worker=%lu arch=riscv-hidden round=%u got=%llu expected=%llu\n",
        (unsigned long) worker_id, round,
        (unsigned long long) hidden_riscv_result,
        (unsigned long long) (hidden_riscv_seed + 7));
      return (void *) 1;
    }
    uint64_t expected_aarch64_fp =
      double_to_bits((double) hidden_aarch64_fp_seed + 7.0);
    uint64_t expected_riscv_fp =
      double_to_bits((double) hidden_riscv_fp_seed + 7.0);
    got_signature_kind = poly_abi_signature_get(signature_slot);
    if (got_signature_kind != signature_kind) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: worker=%lu round=%u ABI signature slot leak got=%llu expected=%llu\n",
        (unsigned long) worker_id, round,
        (unsigned long long) got_signature_kind,
        (unsigned long long) signature_kind);
      return (void *) 1;
    }
    if (hidden_aarch64_fp_result != expected_aarch64_fp) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: worker=%lu arch=aarch64-hidden-fp round=%u got=0x%llx expected=0x%llx\n",
        (unsigned long) worker_id, round,
        (unsigned long long) hidden_aarch64_fp_result,
        (unsigned long long) expected_aarch64_fp);
      return (void *) 1;
    }
    if (hidden_riscv_fp_result != expected_riscv_fp) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: worker=%lu arch=riscv-hidden-fp round=%u got=0x%llx expected=0x%llx\n",
        (unsigned long) worker_id, round,
        (unsigned long long) hidden_riscv_fp_result,
        (unsigned long long) expected_riscv_fp);
      return (void *) 1;
    }

    x86_atomic_add(&mixed_atomic_counter, POLYTHREAD_ATOMIC_ITERS);
    pcall_aarch64_atomic_add(&mixed_atomic_counter, POLYTHREAD_ATOMIC_ITERS);
    pcall_riscv_atomic_add(&mixed_atomic_counter, POLYTHREAD_ATOMIC_ITERS);

    for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
      sched_yield();
  }

  return 0;
}

int main(void) {
  pthread_t threads[POLYTHREAD_THREADS];

  printf("POLYTHREAD_START\n");
  if (check_polythread_contract() < 0)
    return 1;
  if (pthread_barrier_init(&start_barrier, 0, POLYTHREAD_THREADS) != 0) {
    fprintf(stderr, "POLYTHREAD_FAIL: pthread_barrier_init\n");
    return 1;
  }
  for (uintptr_t n = 0; n < POLYTHREAD_THREADS; n++) {
    if (pthread_create(&threads[n], 0, worker_main, (void *) n) != 0) {
      fprintf(stderr, "POLYTHREAD_FAIL: pthread_create %lu\n", (unsigned long) n);
      pthread_barrier_destroy(&start_barrier);
      return 1;
    }
  }

  for (unsigned n = 0; n < POLYTHREAD_THREADS; n++) {
    void *status = 0;
    if (pthread_join(threads[n], &status) != 0 || status != 0) {
      fprintf(stderr, "POLYTHREAD_FAIL: pthread_join %u\n", n);
      pthread_barrier_destroy(&start_barrier);
      return 1;
    }
  }
  uint64_t state_key_count =
    __atomic_load_n(&explicit_state_key_counter, __ATOMIC_SEQ_CST);
  if (state_key_count != POLYTHREAD_THREADS) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: explicit state-key workers got=%llu expected=%u\n",
      (unsigned long long) state_key_count, POLYTHREAD_THREADS);
    pthread_barrier_destroy(&start_barrier);
    return 1;
  }
  printf("POLYTHREAD_STATE_KEY_OK workers=%u\n", POLYTHREAD_THREADS);
  uint64_t real_xsave_context_count =
    __atomic_load_n(&real_xsave_context_counter, __ATOMIC_SEQ_CST);
  uint64_t real_xsave_no_key_count =
    __atomic_load_n(&real_xsave_no_key_counter, __ATOMIC_SEQ_CST);
  if (polythread_poly_xsave_enabled()) {
    if (real_xsave_context_count != POLYTHREAD_THREADS) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: real XSAVE context workers got=%llu expected=%u\n",
        (unsigned long long) real_xsave_context_count,
        POLYTHREAD_THREADS);
      pthread_barrier_destroy(&start_barrier);
      return 1;
    }
    printf("POLYTHREAD_REAL_XSAVE_CONTEXT_OK workers=%u\n",
      POLYTHREAD_THREADS);
    if (real_xsave_no_key_count != POLYTHREAD_THREADS) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: real XSAVE no-key workers got=%llu expected=%u\n",
        (unsigned long long) real_xsave_no_key_count,
        POLYTHREAD_THREADS);
      pthread_barrier_destroy(&start_barrier);
      return 1;
    }
    printf("POLYTHREAD_REAL_XSAVE_NO_KEY_OK workers=%u\n",
      POLYTHREAD_THREADS);
  }
  else {
    printf("POLYTHREAD_REAL_XSAVE_CONTEXT_SKIPPED\n");
  }
  printf("POLYTHREAD_STATE_ISOLATION_OK workers=%u rounds=%u\n",
    POLYTHREAD_THREADS, POLYTHREAD_ROUNDS);

  uint64_t expected_mixed_counter =
    (uint64_t) POLYTHREAD_THREADS * POLYTHREAD_ROUNDS *
    POLYTHREAD_ATOMIC_ITERS * 3;
  uint64_t got_mixed_counter =
    __atomic_load_n(&mixed_atomic_counter, __ATOMIC_SEQ_CST);
  if (got_mixed_counter != expected_mixed_counter) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: mixed-atomic-counter got=%llu expected=%llu\n",
      (unsigned long long) got_mixed_counter,
      (unsigned long long) expected_mixed_counter);
    pthread_barrier_destroy(&start_barrier);
    return 1;
  }
  printf("POLYTHREAD_MIXED_ATOMIC_OK counter=%llu\n",
    (unsigned long long) got_mixed_counter);

  pthread_barrier_destroy(&start_barrier);
  printf("POLYTHREAD_OK\n");
  return 0;
}
