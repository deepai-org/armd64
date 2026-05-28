#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>

#include "../include/polycpuid.h"

#define POLY_OP_ENTER_A64 ".byte 0x0f,0x24,0x01,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_ENTER_RV64 ".byte 0x0f,0x24,0x02,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_PIRET ".byte 0x0f,0x24,0x20,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x24,0x62,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x24,0x60,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x24,0x63,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_STATUS_REASON ".byte 0x0f,0x24,0x50,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_STATUS_MODE ".byte 0x0f,0x24,0x51,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_STATUS_NUMBER ".byte 0x0f,0x24,0x52,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_STATUS_ARG6 ".byte 0x0f,0x24,0x5c,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_STATUS_ARG7 ".byte 0x0f,0x24,0x5d,0x50,0x4f,0x4c,0x59,0x21\n"

enum {
  POLYTHREAD_THREADS = 4,
  POLYTHREAD_ROUNDS = 12,
  POLYTHREAD_BUSY = 20000,
  POLYTHREAD_ATOMIC_ITERS = 16,
  POLYTHREAD_YIELDS = 8,
  POLYTHREAD_IMPORT_FUNC_STRLEN = 8
};

static pthread_barrier_t start_barrier;
static uint64_t mixed_atomic_counter __attribute__((aligned(8)));

struct polythread_import_descriptor {
  uint64_t target;
  uint64_t trampoline;
  uint64_t flags;
  uint64_t stack_arg_qwords;
};

static struct polythread_import_descriptor
  polythread_imports[POLY_IMPORT_FUNC_COUNT];

static inline void poly_state_key_set(uint64_t value) {
  asm volatile(
    ".byte 0x0f,0x24,0x65,0x50,0x4f,0x4c,0x59,0x21\n"
    : "+a"(value)
    :
    : "memory");
}

static inline uint64_t poly_state_key_get(void) {
  uint64_t value;
  asm volatile(
    ".byte 0x0f,0x24,0x66,0x50,0x4f,0x4c,0x59,0x21\n"
    : "=a"(value)
    :
    : "memory");
  return value;
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

__attribute__((naked, noinline, used))
static void polythread_import_return_trampoline(void) {
  __asm__(
    POLY_OP_PIRET
    "ud2\n");
}

__attribute__((noinline, used))
static uint64_t polythread_x86_import_sum6(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  for (unsigned n = 0; n < POLYTHREAD_YIELDS; n++)
    sched_yield();
  return a0 + a1 + a2 + a3 + a4 + a5;
}

static void setup_polythread_imports(void) {
  polythread_imports[POLYTHREAD_IMPORT_FUNC_STRLEN].target =
    (uint64_t) (uintptr_t) polythread_x86_import_sum6;
  polythread_imports[POLYTHREAD_IMPORT_FUNC_STRLEN].trampoline =
    (uint64_t) (uintptr_t) polythread_import_return_trampoline;
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
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
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
    ".byte 0x0f,0x24,0x11,0x50,0x4f,0x4c,0x59,0x21\n"
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

static uint64_t pcall_aarch64_hidden_busy(uint64_t seed) {
  uint64_t result;
  uint64_t arg0 = seed;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
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
    ".byte 0x0f,0x24,0x11,0x50,0x4f,0x4c,0x59,0x21\n"
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
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
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
    ".byte 0x0f,0x24,0x11,0x50,0x4f,0x4c,0x59,0x21\n"
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

static uint64_t trap_aarch64_syscall(uint64_t number, uint64_t arg6,
    uint64_t arg7) {
  uint64_t result = number;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x91000008\n" // add x8,x0,#0
    ".long 0x91000026\n" // add x6,x1,#0
    ".long 0x91000047\n" // add x7,x2,#0
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd42fffe0\n" // brk #0x7fff
    : "+a"(result), "+D"(arg6), "+S"(arg7)
    :
    : "rbx", "rcx", "rdx", "r8", "r9", "r10", "r11", "r13", "r14",
      "memory");
  return result;
}

static uint64_t trap_riscv_syscall(uint64_t number, uint64_t arg6) {
  uint64_t result = number;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00058813\n" // addi a6,a1,0
    ".long 0x00050893\n" // addi a7,a0,0
    ".long 0x00000073\n" // ecall
    ".long 0x0000000b\n" // custom-0 x86 escape
    : "+a"(result), "+D"(arg6)
    :
    : "rbx", "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "r13",
      "r14", "memory");
  return result;
}

static uint64_t trap_aarch64_import(uint64_t arg0, uint64_t arg6,
    uint64_t arg7) {
  uint64_t result = arg0;
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_A64
    ".long 0xd29c1010\n" // movz x16,#0xe080
    ".long 0xf2bffff0\n" // movk x16,#0xffff,lsl #16
    ".long 0xf2dffff0\n" // movk x16,#0xffff,lsl #32
    ".long 0xf2fffff0\n" // movk x16,#0xffff,lsl #48
    ".long 0x91000006\n" // add x6,x0,#0
    ".long 0x91000027\n" // add x7,x1,#0
    ".long 0x91000040\n" // add x0,x2,#0
    ".long 0xd63f0200\n" // blr x16, unresolved strlen descriptor
    ".long 0xd42fffe0\n" // brk #0x7fff
    : "+a"(arg6), "+D"(arg7), "+S"(result)
    :
    : "rbx", "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", "r13",
      "r14", "memory");
  return arg6;
}

static uint64_t trap_riscv_import(uint64_t arg0, uint64_t arg6,
    uint64_t arg7) {
  uint64_t result = arg0;
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> unresolved strlen descriptor
    ".long 0x00058813\n" // addi a6,a1,0
    ".long 0x00060893\n" // addi a7,a2,0
    ".long 0x00050513\n" // addi a0,a0,0
    ".long 0x000280e7\n" // jalr ra,0(t0)
    ".long 0x0000000b\n" // custom-0 x86 escape
    : "+a"(result), "+D"(arg6), "+S"(arg7)
    :
    : "rbx", "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

static uint64_t descriptor_aarch64_import_sum6(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  register uint64_t r8_arg asm("r8") = a5;
  asm volatile(
    "movq %[imports], %%r12\n"
    POLY_OP_ENTER_A64
    ".long 0xd29c1010\n" // movz x16,#0xe080
    ".long 0xf2bffff0\n" // movk x16,#0xffff,lsl #16
    ".long 0xf2dffff0\n" // movk x16,#0xffff,lsl #32
    ".long 0xf2fffff0\n" // movk x16,#0xffff,lsl #48
    ".long 0xd63f0200\n" // blr x16, descriptor-backed import
    ".long 0xd42fffe0\n" // brk #0x7fff
    : "+a"(a0), "+D"(a1), "+S"(a2), "+d"(a3), "+c"(a4),
      "+r"(r8_arg)
    : [imports] "r"((uint64_t) (uintptr_t) polythread_imports)
    : "rbx", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  return a0;
}

static uint64_t descriptor_riscv_import_sum6(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  register uint64_t r8_arg asm("r8") = a5;
  asm volatile(
    "movq %[imports], %%r12\n"
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> descriptor-backed import
    ".long 0x000280e7\n" // jalr ra,0(t0)
    ".long 0x0000000b\n" // custom-0 x86 escape
    : "+a"(a0), "+D"(a1), "+S"(a2), "+d"(a3), "+c"(a4),
      "+r"(r8_arg)
    : [imports] "r"((uint64_t) (uintptr_t) polythread_imports)
    : "rbx", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  return a0;
}

static uint64_t pcall_aarch64_hidden_set(uint64_t value) {
  uint64_t result;
  uint64_t arg0 = value;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
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
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
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
    ".byte 0x0f,0x24,0x11,0x50,0x4f,0x4c,0x59,0x21\n"
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
    ".byte 0x0f,0x24,0x11,0x50,0x4f,0x4c,0x59,0x21\n"
    "1:\n"
    ".long 0x00aa0533\n" // add a0,s4,a0
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(result), "+D"(arg0)
    :
    : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "memory");
  return result;
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
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
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
    ".byte 0x0f,0x24,0x11,0x50,0x4f,0x4c,0x59,0x21\n"
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

  uint64_t default_aarch64_seed = base + 0x20000ULL;
  uint64_t default_riscv_seed = base + 0x30000ULL;
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
      poly_trap_status_arg6() != aarch64_trap_arg6 ||
      poly_trap_status_arg7() != aarch64_trap_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 trap packet reason=%llu mode=%llu number=%llu arg6=%llu arg7=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
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
      poly_trap_status_arg6() != riscv_trap_arg6 ||
      poly_trap_status_arg7() != riscv_trap_number) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv trap packet reason=%llu mode=%llu number=%llu arg6=%llu arg7=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
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
      poly_trap_status_arg6() != aarch64_import_arg6 ||
      poly_trap_status_arg7() != aarch64_import_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default aarch64 import packet reason=%llu mode=%llu number=%llu arg6=%llu arg7=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
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
      poly_trap_status_arg6() != riscv_import_arg6 ||
      poly_trap_status_arg7() != riscv_import_arg7) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu default riscv import packet reason=%llu mode=%llu number=%llu arg6=%llu arg7=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return (void *) 1;
  }

  if (wait_for_workers(worker_id, "default-hidden-checked") != 0)
    return (void *) 1;

  if (wait_for_workers(worker_id, "descriptor-import-start") != 0)
    return (void *) 1;
  uint64_t descriptor_aarch64_base = base + 0xd0000ULL;
  uint64_t descriptor_aarch64_result = descriptor_aarch64_import_sum6(
    descriptor_aarch64_base + 1, descriptor_aarch64_base + 2,
    descriptor_aarch64_base + 3, descriptor_aarch64_base + 4,
    descriptor_aarch64_base + 5, descriptor_aarch64_base + 6);
  uint64_t descriptor_aarch64_expected =
    descriptor_aarch64_base * 6 + 21;
  if (descriptor_aarch64_result != descriptor_aarch64_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu descriptor aarch64 import got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) descriptor_aarch64_result,
      (unsigned long long) descriptor_aarch64_expected);
    return (void *) 1;
  }

  uint64_t descriptor_riscv_base = base + 0xe0000ULL;
  uint64_t descriptor_riscv_result = descriptor_riscv_import_sum6(
    descriptor_riscv_base + 1, descriptor_riscv_base + 2,
    descriptor_riscv_base + 3, descriptor_riscv_base + 4,
    descriptor_riscv_base + 5, descriptor_riscv_base + 6);
  uint64_t descriptor_riscv_expected = descriptor_riscv_base * 6 + 21;
  if (descriptor_riscv_result != descriptor_riscv_expected) {
    fprintf(stderr,
      "POLYTHREAD_FAIL: worker=%lu descriptor riscv import got=%llu expected=%llu\n",
      (unsigned long) worker_id,
      (unsigned long long) descriptor_riscv_result,
      (unsigned long long) descriptor_riscv_expected);
    return (void *) 1;
  }
  if (wait_for_workers(worker_id, "descriptor-import-done") != 0)
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
  setup_polythread_imports();
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
