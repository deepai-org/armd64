#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>

#include "../include/polycpuid.h"

#define POLY_OP_ENTER_A64 ".byte 0x0f,0x3a,0xfc,0x01\n"
#define POLY_OP_ENTER_RV64 ".byte 0x0f,0x3a,0xfc,0x02\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x3a,0xfc,0x62\n"
#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x3a,0xfc,0x60\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x3a,0xfc,0x63\n"
#define POLY_OP_TRAP_STATUS_REASON ".byte 0x0f,0x3a,0xfc,0x50\n"
#define POLY_OP_TRAP_STATUS_MODE ".byte 0x0f,0x3a,0xfc,0x51\n"
#define POLY_OP_TRAP_STATUS_NUMBER ".byte 0x0f,0x3a,0xfc,0x52\n"
#define POLY_OP_TRAP_STATUS_SELECTOR ".byte 0x0f,0x3a,0xfc,0x5a\n"
#define POLY_OP_TRAP_STATUS_ARG6 ".byte 0x0f,0x3a,0xfc,0x5c\n"
#define POLY_OP_TRAP_STATUS_ARG7 ".byte 0x0f,0x3a,0xfc,0x5d\n"
#define POLY_OP_STATE_EXPORT ".byte 0x0f,0x3a,0xfc,0x67\n"
#define POLY_OP_ABI_SIGNATURE_SET ".byte 0x0f,0x3a,0xfc,0x69\n"
#define POLY_OP_ABI_SIGNATURE_GET ".byte 0x0f,0x3a,0xfc,0x6a\n"
#define POLY_OP_PCALL_SIG_IMM_MODE_SLOT3 ".byte 0x0f,0x3a,0xfc,0x2e,0x03\n"

enum {
  POLYTHREAD_THREADS = 4,
  POLYTHREAD_ROUNDS = 12,
  POLYTHREAD_BUSY = 20000,
  POLYTHREAD_ATOMIC_ITERS = 16,
  POLYTHREAD_YIELDS = 8
};

static pthread_barrier_t start_barrier;
static uint64_t mixed_atomic_counter __attribute__((aligned(8)));

static inline void poly_state_key_set(uint64_t value) {
  asm volatile(
    ".byte 0x0f,0x3a,0xfc,0x65\n"
    : "+a"(value)
    :
    : "memory");
}

static inline uint64_t poly_state_key_get(void) {
  uint64_t value;
  asm volatile(
    ".byte 0x0f,0x3a,0xfc,0x66\n"
    : "=a"(value)
    :
    : "memory");
  return value;
}

static inline uint64_t poly_abi_signature_set(uint64_t slot, uint64_t kind) {
  uint64_t rax = slot;
  uint64_t rdx = kind;
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

static inline void poly_state_export(struct poly_xsave_state *state) {
  asm volatile(POLY_OP_STATE_EXPORT :: "a"(state) : "memory");
}

static inline void poly_trap_vector_set(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline void poly_trap_vector_mode_set(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
}

static inline uint64_t poly_trap_status_reason(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_STATUS_REASON : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_status_mode(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_STATUS_MODE : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_status_number(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_STATUS_NUMBER : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_status_selector(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_STATUS_SELECTOR : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_status_arg6(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_STATUS_ARG6 : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_trap_status_arg7(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_STATUS_ARG7 : "=a"(value) :: "memory");
  return value;
}

__attribute__((naked, noinline, used))
static void polythread_trap_vector_handler(void) {
  __asm__(
    "cmpq $1, %rax\n"
    "je 1f\n"
    "cmpq $3, %rax\n"
    "jne 2f\n"
    "1:\n"
    "movq %rcx, %rax\n"
    "addq %r13, %rax\n"
    "addq %r14, %rax\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "2:\n"
    "movq $0xffffffffffffffff, %rax\n"
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

static __attribute__((noinline)) int check_state_key_after_stack_growth(
  uintptr_t worker_id, uint64_t expected) {
  volatile unsigned char stack_pad[12288];
  for (unsigned n = 0; n < sizeof(stack_pad); n += 4096)
    stack_pad[n] = (unsigned char) n;
  asm volatile("" : : "m"(stack_pad) : "memory");
  uint64_t got = poly_state_key_get();
  if (got != expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu stack-growth explicit-state-key got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) got,
      (unsigned long long) expected);
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
    ".byte 0x0f,0x3a,0xfc,0x10\n"
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
    ".byte 0x0f,0x3a,0xfc,0x11\n"
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
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT3
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
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT3
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
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT3
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
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT3
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
    ".byte 0x0f,0x3a,0xfc,0x10\n"
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
    ".byte 0x0f,0x3a,0xfc,0x11\n"
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
    ".byte 0x0f,0x3a,0xfc,0x10\n"
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
    ".byte 0x0f,0x3a,0xfc,0x11\n"
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
    ".byte 0x0f,0x3a,0xfc,0x10\n"
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
    ".byte 0x0f,0x3a,0xfc,0x10\n"
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
    ".byte 0x0f,0x3a,0xfc,0x11\n"
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
    ".byte 0x0f,0x3a,0xfc,0x11\n"
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
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(result), "+d"(arg6_lane), "+c"(arg7_lane)
    :
    : "rbx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r13", "r14",
      "memory");
  return result;
}

static uint64_t trap_riscv_syscall(uint64_t number, uint64_t arg6) {
  uint64_t result = number;
  uint64_t arg6_lane = arg6;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00058813\n" // addi a6,a1,0
    ".long 0x00050893\n" // addi a7,a0,0
    ".long 0x00000073\n" // ecall
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(result), "+d"(arg6_lane)
    :
    : "rbx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r13",
      "r14", "memory");
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
    ".long 0xd503201f\n" // nop, x0 already carries arg0
    ".long 0xd63f0200\n" // blr x16, unresolved strlen descriptor
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(result), "+d"(arg6_lane), "+c"(arg7_lane)
    :
    : "rbx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13",
      "r14", "memory");
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
    ".long 0x00050513\n" // addi a0,a0,0
    ".long 0x000280e7\n" // jalr ra,0(t0)
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(result), "+d"(arg6_lane), "+c"(arg7_lane)
    :
    : "rbx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13",
      "r14", "memory");
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
    ".long 0xd5032f3f\n" // generic pcall frontend=x17 target=x16
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(a0), "+d"(a1), "+c"(a2), "+D"(a3), "+S"(a4),
      "+r"(r8_arg), "+r"(target)
    :
    : "rbx", "r9", "r11", "r12", "r13", "r14", "memory");
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
    ".long 0x1200700b\n" // generic pcall frontend=t1 target=t0 return=t2
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(a0), "+d"(a1), "+c"(a2), "+D"(a3), "+S"(a4),
      "+r"(r8_arg), "+r"(target)
    :
    : "rbx", "r9", "r11", "r12", "r13", "r14", "memory");
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
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(a0), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
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
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(a0), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
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
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    "1:\n"
    "mulsd %%xmm1, %%xmm0\n"
    "retq\n"
    "2:\n"
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "xmm0", "xmm1", "memory");
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
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    "1:\n"
    "mulsd %%xmm1, %%xmm0\n"
    "retq\n"
    "2:\n"
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "xmm0", "xmm1", "memory");
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
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
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
      "r13", "r14", "memory");
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
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
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
      "r13", "r14", "memory");
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
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x02b50553\n" // fadd.d fa0,fa0,fa1
    ".long 0x0ab50553\n" // fsub.d fa0,fa0,fa1
    ".long 0x12b50553\n" // fmul.d fa0,fa0,fa1
    ".long 0x00008067\n" // ret through hardware return cookie
    "2:\n"
    ::: "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "xmm0", "xmm1", "memory");
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
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x1e612800\n" // fadd d0,d0,d1
    ".long 0x1e613800\n" // fsub d0,d0,d1
    ".long 0x1e610800\n" // fmul d0,d0,d1
    ".long 0xd65f03c0\n" // ret x30 through hardware return cookie
    "2:\n"
    ::: "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_aarch64_hidden_set(uint64_t value) {
  uint64_t result;
  uint64_t arg0 = value;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x3a,0xfc,0x10\n"
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
    ".byte 0x0f,0x3a,0xfc,0x10\n"
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
    ".byte 0x0f,0x3a,0xfc,0x11\n"
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
    ".byte 0x0f,0x3a,0xfc,0x11\n"
    "1:\n"
    ".long 0x00aa0533\n" // add a0,s4,a0
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(result), "+D"(arg0)
    :
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static int check_exported_thread_banks(uintptr_t worker_id,
    uint64_t expected_aarch64_gpr, uint64_t expected_riscv_gpr,
    uint64_t expected_aarch64_fp, uint64_t expected_riscv_fp) {
  struct poly_xsave_state snapshot __attribute__((aligned(64)));
  poly_state_export(&snapshot);
  if (snapshot.aarch64_gpr[20] != expected_aarch64_gpr ||
      snapshot.riscv_gpr[20] != expected_riscv_gpr ||
      snapshot.aarch64_fp[20].lo != expected_aarch64_fp ||
      snapshot.riscv_fp[20].lo != expected_riscv_fp) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu exported banks a64x20=0x%llx/0x%llx rvx20=0x%llx/0x%llx a64d20=0x%llx/0x%llx rvf20=0x%llx/0x%llx\n",
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
    ".byte 0x0f,0x3a,0xfc,0x10\n"
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
    ".byte 0x0f,0x3a,0xfc,0x11\n"
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
  uint64_t state_key = 0x504f4c5954480000ULL + worker_id + 1;

  if (wait_for_workers(worker_id, "start") != 0)
    return (void *) 1;

  if (poly_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS,
      POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu native ABI signature setup failed\n",
      (unsigned long) worker_id);
    return (void *) 1;
  }
  uint64_t sig_imm_seed = base + 0x12000ULL;
  uint64_t sig_imm_aarch64_result =
    pcall_sig_imm_aarch64_add1(sig_imm_seed);
  if (sig_imm_aarch64_result != sig_imm_seed + 1) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu sig-imm aarch64 got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) sig_imm_aarch64_result,
      (unsigned long long) (sig_imm_seed + 1));
    return (void *) 1;
  }
  uint64_t sig_imm_riscv_result =
    pcall_sig_imm_riscv_add1(sig_imm_seed + 1);
  if (sig_imm_riscv_result != sig_imm_seed + 2) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu sig-imm riscv got=%llu expected=%llu\n",
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
      "POLYTHREAD_FAIL: worker=%lu sig-imm aarch64 fp got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) sig_fp_aarch64_result,
      (unsigned long long) sig_fp_expected);
    return (void *) 1;
  }
  uint64_t sig_fp_riscv_result =
    pcall_sig_imm_riscv_fp64_mix(sig_fp_left, sig_fp_right);
  if (sig_fp_riscv_result != sig_fp_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu sig-imm riscv fp got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) sig_fp_riscv_result,
      (unsigned long long) sig_fp_expected);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "native-sig-imm-pcall") != 0)
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
      "POLYTHREAD_FAIL: worker=%lu default aarch64 bank got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) default_aarch64_result,
      (unsigned long long) (default_aarch64_seed + 9));
    return (void *) 1;
  }
  uint64_t default_riscv_result = pcall_riscv_hidden_get(11);
  if (default_riscv_result != default_riscv_seed + 11) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv bank got=%llu expected=%llu\n",
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
      "POLYTHREAD_FAIL: worker=%lu default aarch64 fp bank got=0x%llx expected=0x%llx\n",
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
      "POLYTHREAD_FAIL: worker=%lu default riscv fp bank got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) default_riscv_fp_result,
      (unsigned long long) default_riscv_fp_expected);
    return (void *) 1;
  }
  if (check_exported_thread_banks(worker_id, default_aarch64_seed,
      default_riscv_seed, double_to_bits((double) default_aarch64_fp_seed),
      double_to_bits((double) default_riscv_fp_seed)) != 0)
    return (void *) 1;

  poly_trap_vector_mode_set(POLY_MODE_X86);
  poly_trap_vector_set(
    (uint64_t) (uintptr_t) polythread_trap_vector_handler);

  uint64_t aarch64_trap_number = 200 + worker_id;
  uint64_t aarch64_trap_arg6 = base + 0x40000ULL;
  uint64_t aarch64_trap_arg7 = base + 0x50000ULL;
  uint64_t aarch64_trap_result = trap_aarch64_syscall(aarch64_trap_number,
    aarch64_trap_arg6, aarch64_trap_arg7);
  if (aarch64_trap_result !=
      aarch64_trap_number + aarch64_trap_arg6 + aarch64_trap_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 trap result got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) aarch64_trap_result,
      (unsigned long long) (aarch64_trap_number + aarch64_trap_arg6 +
        aarch64_trap_arg7));
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-aarch64-trap") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  if (poly_trap_status_reason() != POLY_TRAP_SYSCALL ||
      poly_trap_status_mode() != POLY_MODE_RAW_AARCH64 ||
      poly_trap_status_number() != aarch64_trap_number ||
      poly_trap_status_selector() != 7 ||
      poly_trap_status_arg6() != aarch64_trap_arg6 ||
      poly_trap_status_arg7() != aarch64_trap_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 trap packet reason=%llu mode=%llu number=%llu selector=%llu arg6=%llu arg7=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
      (unsigned long long) poly_trap_status_selector(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return (void *) 1;
  }

  uint64_t riscv_trap_number = 300 + worker_id;
  uint64_t riscv_trap_arg6 = base + 0x60000ULL;
  uint64_t riscv_trap_result =
    trap_riscv_syscall(riscv_trap_number, riscv_trap_arg6);
  if (riscv_trap_result !=
      riscv_trap_number + riscv_trap_arg6 + riscv_trap_number) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv trap result got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) riscv_trap_result,
      (unsigned long long) (riscv_trap_number + riscv_trap_arg6 +
        riscv_trap_number));
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-riscv-trap") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  if (poly_trap_status_reason() != POLY_TRAP_SYSCALL ||
      poly_trap_status_mode() != POLY_MODE_RAW_RISCV ||
      poly_trap_status_number() != riscv_trap_number ||
      poly_trap_status_selector() != 0 ||
      poly_trap_status_arg6() != riscv_trap_arg6 ||
      poly_trap_status_arg7() != riscv_trap_number) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv trap packet reason=%llu mode=%llu number=%llu selector=%llu arg6=%llu arg7=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
      (unsigned long long) poly_trap_status_selector(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return (void *) 1;
  }

  uint64_t import_id = 8;
  uint64_t aarch64_import_arg6 = base + 0x70000ULL;
  uint64_t aarch64_import_arg7 = base + 0x80000ULL;
  uint64_t aarch64_import_arg0 = base + 0x90000ULL;
  uint64_t aarch64_import_result = trap_aarch64_import(
    aarch64_import_arg0, aarch64_import_arg6, aarch64_import_arg7);
  if (aarch64_import_result !=
      import_id + aarch64_import_arg6 + aarch64_import_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 import result got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) aarch64_import_result,
      (unsigned long long) (import_id + aarch64_import_arg6 +
        aarch64_import_arg7));
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-aarch64-import") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  if (poly_trap_status_reason() != POLY_TRAP_IMPORT ||
      poly_trap_status_mode() != POLY_MODE_RAW_AARCH64 ||
      poly_trap_status_number() != import_id ||
      poly_trap_status_selector() != 0 ||
      poly_trap_status_arg6() != aarch64_import_arg6 ||
      poly_trap_status_arg7() != aarch64_import_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 import packet reason=%llu mode=%llu number=%llu selector=%llu arg6=%llu arg7=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
      (unsigned long long) poly_trap_status_selector(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return (void *) 1;
  }

  uint64_t riscv_import_arg6 = base + 0xa0000ULL;
  uint64_t riscv_import_arg7 = base + 0xb0000ULL;
  uint64_t riscv_import_arg0 = base + 0xc0000ULL;
  uint64_t riscv_import_result = trap_riscv_import(riscv_import_arg0,
    riscv_import_arg6, riscv_import_arg7);
  if (riscv_import_result !=
      import_id + riscv_import_arg6 + riscv_import_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv import result got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) riscv_import_result,
      (unsigned long long) (import_id + riscv_import_arg6 +
        riscv_import_arg7));
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "default-riscv-import") != 0)
    return (void *) 1;
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  if (poly_trap_status_reason() != POLY_TRAP_IMPORT ||
      poly_trap_status_mode() != POLY_MODE_RAW_RISCV ||
      poly_trap_status_number() != import_id ||
      poly_trap_status_selector() != 0 ||
      poly_trap_status_arg6() != riscv_import_arg6 ||
      poly_trap_status_arg7() != riscv_import_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv import packet reason=%llu mode=%llu number=%llu selector=%llu arg6=%llu arg7=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
      (unsigned long long) poly_trap_status_selector(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return (void *) 1;
  }

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

  poly_state_key_set(state_key);
  if (wait_for_workers(worker_id, "state-key-set") != 0)
    return (void *) 1;
  uint64_t current_state_key = poly_state_key_get();
  if (current_state_key != state_key) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu explicit-state-key got=0x%llx expected=0x%llx\n",
      (unsigned long) worker_id,
      (unsigned long long) current_state_key,
      (unsigned long long) state_key);
    return (void *) 1;
  }
  if (check_state_key_after_stack_growth(worker_id, state_key) != 0)
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
    current_state_key = poly_state_key_get();
    if (current_state_key != state_key) {
      fprintf(stderr,
        "POLYTHREAD_FAIL: worker=%lu round=%u explicit-state-key got=0x%llx expected=0x%llx\n",
        (unsigned long) worker_id, round,
        (unsigned long long) current_state_key,
        (unsigned long long) state_key);
      return (void *) 1;
    }
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

  poly_state_key_set(0);
  return 0;
}

int main(void) {
  pthread_t threads[POLYTHREAD_THREADS];

  printf("POLYTHREAD_START\n");
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
