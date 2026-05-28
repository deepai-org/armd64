#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "polycpuid.h"

#define POLY_OP_EXIT ".byte 0x0f,0x24,0x00,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_ENTER_A64 ".byte 0x0f,0x24,0x01,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_ENTER_RV64 ".byte 0x0f,0x24,0x02,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_PCALL_A64 ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_PCALL_RV64 ".byte 0x0f,0x24,0x11,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x24,0x60,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x24,0x63,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x24,0x62,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_STATE_EXPORT ".byte 0x0f,0x24,0x67,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_STATE_IMPORT ".byte 0x0f,0x24,0x68,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_ABI_GPR_CLOBBERS "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9"
#define POLY_ABI_GPR_CLOBBERS_NO_RAX "rcx", "rdx", "rsi", "rdi", "r8", "r9"
#define POLY_ABI_GPR_CLOBBERS_NO_RAX_RDI "rcx", "rdx", "rsi", "r8", "r9"

static struct poly_xsave_state polyprobe_state __attribute__((aligned(64)));

static inline void poly_mode_x86(void) { asm volatile(POLY_OP_EXIT ::: "memory"); }
static inline void poly_syscall_x86(void) { asm volatile(".byte 0x0f,0x24,0x30,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_syscall_number_status(void) { asm volatile(".byte 0x0f,0x24,0x31,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_syscall_mode_status(void) { asm volatile(".byte 0x0f,0x24,0x32,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_break_number_status(void) { asm volatile(".byte 0x0f,0x24,0x39,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_break_mode_status(void) { asm volatile(".byte 0x0f,0x24,0x3a,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_switch_count_status(void) { asm volatile(".byte 0x0f,0x24,0x40,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_foreign_insn_count_status(void) { asm volatile(".byte 0x0f,0x24,0x42,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_foreign_syscall_count_status(void) { asm volatile(".byte 0x0f,0x24,0x43,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_foreign_break_count_status(void) { asm volatile(".byte 0x0f,0x24,0x44,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_trap_reason_status(void) { asm volatile(".byte 0x0f,0x24,0x50,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_trap_mode_status(void) { asm volatile(".byte 0x0f,0x24,0x51,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_trap_number_status(void) { asm volatile(".byte 0x0f,0x24,0x52,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_trap_arg0_status(void) { asm volatile(".byte 0x0f,0x24,0x53,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_trap_selector_status(void) { asm volatile(".byte 0x0f,0x24,0x5a,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }

static inline void poly_trap_vector_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline void poly_trap_vector_mode_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
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

static void stage(const char *msg) {
  if (write(1, msg, strlen(msg)) < 0)
    return;
  ssize_t ignored = write(1, "\n", 1);
  (void) ignored;
}

static int poly_is_raw_foreign_mode(uint64_t mode) {
  return mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV;
}

__attribute__((noinline, used))
uint64_t polyprobe_trap_vector_dispatch(uint64_t reason, uint64_t mode,
    uint64_t number, uint64_t pc, uint64_t selector, uint64_t arg0,
    uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) {
  (void) pc;
  (void) selector;
  (void) arg1;
  (void) arg2;
  (void) arg3;
  (void) arg4;
  (void) arg5;

  if (!poly_is_raw_foreign_mode(mode))
    return (uint64_t) -38;
  if (reason == POLY_TRAP_SYSCALL && number == 172)
    return 4242;
  if (reason == POLY_TRAP_BREAK)
    return 0x4c000000ULL | (mode << 8) | number;
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
    "pushq %r12\n"
    "pushq %r11\n"
    "pushq %r10\n"
    "pushq %r9\n"
    "pushq %r8\n"
    "movq %rdi, %r9\n"
    "movq %rsi, %r8\n"
    "movq %rcx, %r10\n"
    "movq %rdx, %rcx\n"
    "movq %r10, %rdx\n"
    "movq %rbx, %rsi\n"
    "movq %rax, %rdi\n"
    "call polyprobe_trap_vector_dispatch\n"
    "addq $40, %rsp\n"
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
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_arith_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01100513\n" // addi a0,zero,17
    ".long 0x00550513\n" // addi a0,a0,5
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void poly_opcode_aarch64_arith_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800540\n" // movz x0,#42
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void poly_opcode_riscv_arith_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01100513\n" // addi a0,zero,17
    ".long 0x00550513\n" // addi a0,a0,5
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_sp_probe(void) {
  asm volatile(
    "movq $19, %%rax\n"
    "movq $23, %%rdi\n"
    POLY_OP_ENTER_A64
    ".long 0xd10043ff\n" // sub sp,sp,#16
    ".long 0xa90007e0\n" // stp x0,x1,[sp]
    ".long 0xa9400fe2\n" // ldp x2,x3,[sp]
    ".long 0x8b030040\n" // add x0,x2,x3
    ".long 0x910043ff\n" // add sp,sp,#16
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_sp_probe(void) {
  asm volatile(
    "movq $17, %%rax\n"
    "movq $25, %%rdi\n"
    POLY_OP_ENTER_RV64
    ".long 0xff010113\n" // addi sp,sp,-16
    ".long 0x00a13023\n" // sd a0,0(sp)
    ".long 0x00b13423\n" // sd a1,8(sp)
    ".long 0x00013603\n" // ld a2,0(sp)
    ".long 0x00813683\n" // ld a3,8(sp)
    ".long 0x00d60533\n" // add a0,a2,a3
    ".long 0x01010113\n" // addi sp,sp,16
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_wide_regs_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28000ea\n" // movz x10,#7
    ".long 0xd280046b\n" // movz x11,#35
    ".long 0x8b0b014c\n" // add x12,x10,x11
    ".long 0x8b0a0180\n" // add x0,x12,x10
    ".long 0xd42fffe0\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_wide_regs_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00900813\n" // addi x16,zero,9
    ".long 0x02100913\n" // addi x18,zero,33
    ".long 0x012809b3\n" // add x19,x16,x18
    ".long 0x01098533\n" // add a0,x19,x16
    ".long 0x0000000b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_state_seed_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2824693\n" // movz x19,#0x1234
    ".long 0x9e670268\n" // fmov d8,x19
    ".long 0xd42fffe0\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_state_seed_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x32100993\n" // addi x19,zero,0x321
    ".long 0xf2098953\n" // fmv.d.x f18,x19
    ".long 0x0000000b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_state_gpr_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa1303e0\n" // mov x0,x19
    ".long 0xd42fffe0\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_state_fp_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x9e660100\n" // fmov x0,d8
    ".long 0xd42fffe0\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_state_gpr_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00098533\n" // add a0,x19,zero
    ".long 0x0000000b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_state_fp_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0xe2090553\n" // fmv.x.d a0,f18
    ".long 0x0000000b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_imm_regs_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28000ea\n" // movz x10,#7
    ".long 0x9100154d\n" // add x13,x10,#5
    ".long 0xd10021a0\n" // sub x0,x13,#8
    ".long 0xd42fffe0\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_imm_regs_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0xffd00293\n" // addi x5,zero,-3
    ".long 0x03628513\n" // addi a0,x5,54
    ".long 0x0000000b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_aarch64_abi_args_probe(void) {
  asm volatile(
    "movq $1, %%rdi\n"
    "movq $2, %%rsi\n"
    "movq $3, %%rdx\n"
    "movq $4, %%rcx\n"
    "movq $5, %%r8\n"
    "movq $6, %%r9\n"
    POLY_OP_ENTER_A64
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
    POLY_OP_ENTER_RV64
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
    POLY_OP_PCALL_A64
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
    POLY_OP_PCALL_RV64
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
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
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
    ::: "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
}

static inline void raw_fp64_aarch64_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x1e612800\n"
    ".long 0x1e613800\n"
    ".long 0x1e610800\n"
    ".long 0xd42fffe0\n"
    ::: "xmm0", "memory");
}

static inline void raw_fp64_riscv_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
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
    POLY_OP_PCALL_A64
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
    POLY_OP_PCALL_RV64
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
    POLY_OP_ENTER_A64
    ".long 0xd2800120\n"
    ".long 0xd5033fbf\n"
    ".long 0xd5033f9f\n"
    ".long 0xd5033fdf\n"
    ".long 0x91002000\n"
    ".long 0xd42fffe0\n"
    POLY_OP_ENTER_RV64
    ".long 0x01400513\n"
    ".long 0x0ff0000f\n"
    ".long 0x0000100f\n"
    ".long 0x00250513\n"
    ".long 0x0000000b\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline uint64_t raw_mixed_probe(uint64_t value) {
  register uint64_t rax __asm__("rax") = value;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x91000400\n"
    ".long 0xd42fffc0\n"
    ".long 0x00550513\n"
    ".long 0x0000002b\n"
    ".long 0x91000400\n"
    ".long 0xd42fffe0\n"
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
    ".long 0xd42fffc0\n"
    ".long 0x00550513\n"
    ".long 0x0000002b\n"
    ".long 0x91000400\n"
    ".long 0xd42fffe0\n"
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
    ".long 0xd42fffe0\n"
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
    ".long 0x0000000b\n"
    : "+a"(rax), "+D"(rdi)
    :
    : POLY_ABI_GPR_CLOBBERS_NO_RAX_RDI, "memory");
}

static inline void raw_aarch64_getpid_probe(void) {
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2801588\n"
    ".long 0xd4000001\n"
    ".long 0xd42fffe0\n"
    ::: POLY_ABI_GPR_CLOBBERS, "memory");
}

static inline void raw_riscv_getpid_probe(void) {
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x0ac00893\n"
    ".long 0x00000073\n"
    ".long 0x0000000b\n"
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
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID import manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_escapes.eax, poly_escapes.ebx, poly_escapes.ecx, poly_escapes.edx);
    return 1;
  }
  struct poly_cpuid_regs expected_state =
    poly_cpuid_expected_state_leaf();
  struct poly_cpuid_regs poly_state =
    poly_read_cpuid(POLY_CPUID_BASE + 3, 0);
  if (poly_state.eax != expected_state.eax ||
      poly_state.ebx != expected_state.ebx ||
      poly_state.ecx != expected_state.ecx ||
      poly_state.edx != expected_state.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID state leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_state.eax, poly_state.ebx, poly_state.ecx, poly_state.edx);
    return 1;
  }
  struct poly_cpuid_regs expected_arch_state =
    poly_cpuid_expected_arch_state_leaf();
  struct poly_cpuid_regs poly_arch_state =
    poly_read_cpuid(POLY_CPUID_BASE + 4, 0);
  if (poly_arch_state.eax != expected_arch_state.eax ||
      poly_arch_state.ebx != expected_arch_state.ebx ||
      poly_arch_state.ecx != expected_arch_state.ecx ||
      poly_arch_state.edx != expected_arch_state.edx) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly CPUID arch state leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      poly_arch_state.eax, poly_arch_state.ebx, poly_arch_state.ecx,
      poly_arch_state.edx);
    return 1;
  }
  struct poly_cpuid_regs xsave_leaf0 =
    poly_read_cpuid(0x0000000d, 0);
  if ((xsave_leaf0.eax & (1U << POLY_STATE_XSAVE_COMPONENT_ARCH)) == 0 ||
      xsave_leaf0.ecx < POLY_STATE_XSAVE_OFFSET_ARCH + POLY_STATE_XSAVE_BYTES_ARCH) {
    fprintf(stderr, "POLY_PROBE_FAIL: standard XSAVE poly component missing eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      xsave_leaf0.eax, xsave_leaf0.ebx, xsave_leaf0.ecx, xsave_leaf0.edx);
    return 1;
  }
  struct poly_cpuid_regs xsave_poly =
    poly_read_cpuid(0x0000000d, POLY_STATE_XSAVE_COMPONENT_ARCH);
  if (xsave_poly.eax != POLY_STATE_XSAVE_BYTES_ARCH ||
      xsave_poly.ebx != POLY_STATE_XSAVE_OFFSET_ARCH ||
      xsave_poly.ecx != 0x2 ||
      xsave_poly.edx != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: standard XSAVE poly leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
      xsave_poly.eax, xsave_poly.ebx, xsave_poly.ecx, xsave_poly.edx);
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
  raw_riscv_sp_probe();
  rsp_after = read_rsp();
  if (read_rax() != 42 || rsp_after != rsp_before) {
    fprintf(stderr, "POLY_PROBE_FAIL: riscv SP frame mismatch result=%llu rsp_before=0x%llx rsp_after=0x%llx\n",
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
  raw_aarch64_state_seed_probe();
  raw_riscv_state_seed_probe();
  poly_state_export(&polyprobe_state);
  if (polyprobe_state.header.magic != POLY_STATE_XSAVE_MAGIC ||
      polyprobe_state.header.layout_version != POLY_STATE_XSAVE_LAYOUT_VERSION ||
      polyprobe_state.header.header_bytes != POLY_STATE_XSAVE_HEADER_BYTES ||
      polyprobe_state.header.total_bytes != sizeof(polyprobe_state) ||
      (polyprobe_state.header.flags & POLY_STATE_XSAVE_FLAG_NO_HIDDEN_BANKS) == 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: poly state export header mismatch magic=0x%x version=%u header=%u bytes=%u flags=0x%llx\n",
      polyprobe_state.header.magic, polyprobe_state.header.layout_version,
      polyprobe_state.header.header_bytes, polyprobe_state.header.total_bytes,
      (unsigned long long) polyprobe_state.header.flags);
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
  polyprobe_state.aarch64_gpr[19] = 0x2468;
  polyprobe_state.aarch64_fp[8].lo = 0x3579;
  polyprobe_state.riscv_gpr[19] = 0x432;
  polyprobe_state.riscv_fp[18].lo = 0x543;
  poly_state_import(&polyprobe_state);
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
  raw_aarch64_break_probe((uint64_t) break_string);
  if (read_rax() != 0x4c000301ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 break trap vector mismatch\n");
    return 1;
  }
  poly_break_number_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 break number mismatch\n");
    return 1;
  }
  poly_break_mode_status();
  if (read_rax() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 break mode mismatch\n");
    return 1;
  }
  poly_trap_selector_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw aarch64 break trap selector mismatch\n");
    return 1;
  }
  raw_riscv_break_probe((uint64_t) break_string);
  if (read_rax() != 0x4c000401ULL) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv break trap vector mismatch\n");
    return 1;
  }
  poly_break_number_status();
  if (read_rax() != 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv break number mismatch\n");
    return 1;
  }
  poly_break_mode_status();
  if (read_rax() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv break mode mismatch\n");
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
  poly_trap_selector_status();
  if (read_rax() != 0) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw riscv break trap selector mismatch\n");
    return 1;
  }
  poly_trap_arg0_status();
  if (read_rax() != (uint64_t) break_string) {
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

  poly_foreign_break_count_status();
  uint64_t breaks_before = read_rax();
  raw_aarch64_break_probe((uint64_t) break_string);
  poly_foreign_break_count_status();
  if (read_rax() != breaks_before + 1) {
    fprintf(stderr, "POLY_PROBE_FAIL: raw foreign break count mismatch\n");
    return 1;
  }

  puts("POLY_PROBE_OK");
  return 0;
}
