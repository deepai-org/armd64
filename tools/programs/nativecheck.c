#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/polycpuid.h"

#define POLY_OP_ENTER_A64 ".byte 0x0f,0x3a,0xfc,0x01\n"
#define POLY_OP_ENTER_RV64 ".byte 0x0f,0x3a,0xfc,0x02\n"
#define POLY_OP_ENTER_MODE ".byte 0x0f,0x3a,0xfc,0x03\n"
#define POLY_OP_SWITCH_MODE ".byte 0x0f,0x3a,0xfc,0x04\n"
#define POLY_OP_PCALL_SIG_MODE ".byte 0x0f,0x3a,0xfc,0x2d\n"
#define POLY_OP_PCALL_SIG_IMM_MODE_SLOT0 ".byte 0x0f,0x3a,0xfc,0x2e,0x00\n"
#define POLY_OP_PCALL_SIG_IMM_MODE_SLOT3 ".byte 0x0f,0x3a,0xfc,0x2e,0x03\n"
#define POLY_OP_PCALL_SIG_IMM_MODE_SLOT8 ".byte 0x0f,0x3a,0xfc,0x2e,0x08\n"
#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x3a,0xfc,0x60\n"
#define POLY_OP_TRAP_VECTOR_GET ".byte 0x0f,0x3a,0xfc,0x61\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x3a,0xfc,0x62\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x3a,0xfc,0x63\n"
#define POLY_OP_TRAP_VECTOR_MODE_GET ".byte 0x0f,0x3a,0xfc,0x64\n"
#define POLY_OP_STATE_KEY_SET ".byte 0x0f,0x3a,0xfc,0x65\n"
#define POLY_OP_STATE_KEY_GET ".byte 0x0f,0x3a,0xfc,0x66\n"
#define POLY_OP_STATE_EXPORT ".byte 0x0f,0x3a,0xfc,0x67\n"
#define POLY_OP_STATE_IMPORT ".byte 0x0f,0x3a,0xfc,0x68\n"
#define POLY_OP_TRAP_STATUS_REASON ".byte 0x0f,0x3a,0xfc,0x50\n"
#define POLY_OP_TRAP_STATUS_MODE ".byte 0x0f,0x3a,0xfc,0x51\n"
#define POLY_OP_TRAP_STATUS_NUMBER ".byte 0x0f,0x3a,0xfc,0x52\n"
#define POLY_OP_TRAP_STATUS_SELECTOR ".byte 0x0f,0x3a,0xfc,0x5a\n"
#define POLY_OP_TRAP_STATUS_ARG6 ".byte 0x0f,0x3a,0xfc,0x5c\n"
#define POLY_OP_TRAP_STATUS_ARG7 ".byte 0x0f,0x3a,0xfc,0x5d\n"
#define POLY_OP_SYSCALL_STATUS_NUMBER ".byte 0x0f,0x3a,0xfc,0x31\n"
#define POLY_OP_SYSCALL_STATUS_MODE ".byte 0x0f,0x3a,0xfc,0x32\n"
#define POLY_OP_BREAK_STATUS_NUMBER ".byte 0x0f,0x3a,0xfc,0x39\n"
#define POLY_OP_BREAK_STATUS_MODE ".byte 0x0f,0x3a,0xfc,0x3a\n"
#define POLY_OP_ABI_SIGNATURE_SET ".byte 0x0f,0x3a,0xfc,0x69\n"
#define POLY_OP_ABI_SIGNATURE_GET ".byte 0x0f,0x3a,0xfc,0x6a\n"
#define POLY_OP_MONITOR_PACKET_SET ".byte 0x0f,0x3a,0xfc,0x6b\n"
#define POLY_OP_MONITOR_PACKET_GET ".byte 0x0f,0x3a,0xfc,0x6c\n"
#define POLY_OP_LANDING_POLICY_SET ".byte 0x0f,0x3a,0xfc,0x6d\n"
#define POLY_OP_LANDING_POLICY_GET ".byte 0x0f,0x3a,0xfc,0x6e\n"

#ifndef ARCH_GET_XCOMP_SUPP
#define ARCH_GET_XCOMP_SUPP 0x1021
#endif
#ifndef ARCH_GET_XCOMP_PERM
#define ARCH_GET_XCOMP_PERM 0x1022
#endif
#ifndef ARCH_REQ_XCOMP_PERM
#define ARCH_REQ_XCOMP_PERM 0x1023
#endif

#define POLY_NATIVE_XSAVE_AREA_BYTES \
  (POLY_STATE_XSAVE_OFFSET_ARCH + POLY_STATE_XSAVE_BYTES_ARCH)

typedef uint8_t poly_native_xsave_area_t[POLY_NATIVE_XSAVE_AREA_BYTES];

static uint8_t nativecheck_real_xsave_area[POLY_NATIVE_XSAVE_AREA_BYTES]
  __attribute__((aligned(64)));

struct nativecheck_monitor_packet {
  struct poly_trap_packet trap;
  uint64_t args[POLY_TRAP_PACKET_ARG_COUNT];
};

static int expect_monitor_packet(const char *label,
    const struct nativecheck_monitor_packet *packet, uint32_t reason,
    uint32_t source_mode, uint64_t number, uint64_t selector,
    uint64_t arg0, uint64_t arg6, uint64_t arg7) {
  if (packet->trap.reason != reason ||
      packet->trap.source_mode != source_mode ||
      packet->trap.number != number ||
      packet->trap.selector != selector ||
      packet->args[0] != arg0 ||
      packet->args[6] != arg6 ||
      packet->args[7] != arg7 ||
      (packet->trap.flags & POLY_TRAP_PACKET_FLAG_MONITOR_MEMORY) == 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly monitor packet %s mismatch reason=%u mode=%u number=%llu selector=%llu arg0=%llu arg6=%llu arg7=%llu flags=0x%llx\n",
      label, packet->trap.reason, packet->trap.source_mode,
      (unsigned long long) packet->trap.number,
      (unsigned long long) packet->trap.selector,
      (unsigned long long) packet->args[0],
      (unsigned long long) packet->args[6],
      (unsigned long long) packet->args[7],
      (unsigned long long) packet->trap.flags);
    return 1;
  }
  return 0;
}

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
}

static inline uint64_t read_xmm0_u64(void) {
  uint64_t value;
  asm volatile("movq %%xmm0,%0" : "=r"(value));
  return value;
}

static inline void write_xmm0_u64(uint64_t value) {
  asm volatile("movq %0,%%xmm0" :: "r"(value) : "xmm0", "memory");
}

static inline void write_xmm1_u64(uint64_t value) {
  asm volatile("movq %0,%%xmm1" :: "r"(value) : "xmm1", "memory");
}

struct nativecheck_u128 {
  uint64_t lo;
  uint64_t hi;
} __attribute__((aligned(16)));

static inline void write_xmm0_xmm1_u128(uint64_t x0_lo, uint64_t x0_hi,
    uint64_t x1_lo, uint64_t x1_hi) {
  struct nativecheck_u128 x0 = { x0_lo, x0_hi };
  struct nativecheck_u128 x1 = { x1_lo, x1_hi };
  asm volatile(
    "movdqu %0,%%xmm0\n"
    "movdqu %1,%%xmm1\n"
    :
    : "m"(x0), "m"(x1)
    : "xmm0", "xmm1", "memory");
}

static inline struct nativecheck_u128 read_xmm0_u128(void) {
  struct nativecheck_u128 value;
  asm volatile("movdqu %%xmm0,%0" : "=m"(value) :: "memory");
  return value;
}

static inline void poly_trap_vector_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline void poly_trap_vector_get(void) {
  asm volatile(POLY_OP_TRAP_VECTOR_GET ::: "memory");
}

static inline void poly_trap_vector_mode_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
}

static inline uint64_t poly_trap_vector_mode_set_result(uint64_t value) {
  uint64_t result = value;
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET
      : "+a"(result)
      :
      : "memory");
  return result;
}

static inline void poly_trap_vector_mode_get(void) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_GET ::: "memory");
}

static inline void poly_monitor_packet_set_value(uint64_t value) {
  asm volatile(POLY_OP_MONITOR_PACKET_SET :: "a"(value) : "memory");
}

static inline void poly_monitor_packet_get(void) {
  asm volatile(POLY_OP_MONITOR_PACKET_GET ::: "memory");
}

static uint64_t poly_aarch64_trap_vector_set_get(uint64_t value) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032d1f\n" // aarch64 trap-vector set, x0=value.
    ".long 0xd5032d3f\n" // aarch64 trap-vector get, x0=result.
    ".long 0xd5032e1f\n" // aarch64 x86 escape.
    : "=a"(result)
    : "0"(value)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  return result;
}

static uint64_t poly_aarch64_trap_vector_mode_set_get(uint64_t value) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032d5f\n" // aarch64 trap-vector-mode set, x0=value.
    ".long 0xd5032d7f\n" // aarch64 trap-vector-mode get, x0=result.
    ".long 0xd5032e1f\n" // aarch64 x86 escape.
    : "=a"(result)
    : "0"(value)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  return result;
}

static uint64_t poly_aarch64_trap_vector_mode_set(uint64_t value) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032d5f\n" // aarch64 trap-vector-mode set, x0=value.
    ".long 0xd5032e1f\n" // aarch64 x86 escape.
    : "=a"(result)
    : "0"(value)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  return result;
}

static uint64_t poly_aarch64_monitor_packet_set_get(uint64_t value) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032d9f\n" // aarch64 monitor-packet set, x0=value.
    ".long 0xd5032dbf\n" // aarch64 monitor-packet get, x0=result.
    ".long 0xd5032e1f\n" // aarch64 x86 escape.
    : "=a"(result)
    : "0"(value)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  return result;
}

static uint64_t poly_riscv_trap_vector_set_get(uint64_t value) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3000700b\n" // riscv trap-vector set, a0=value.
    ".long 0x3200700b\n" // riscv trap-vector get, a0=result.
    ".long 0x0000700b\n" // riscv x86 escape.
    : "=a"(result)
    : "0"(value)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  return result;
}

static uint64_t poly_riscv_trap_vector_mode_set_get(uint64_t value) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3400700b\n" // riscv trap-vector-mode set, a0=value.
    ".long 0x3600700b\n" // riscv trap-vector-mode get, a0=result.
    ".long 0x0000700b\n" // riscv x86 escape.
    : "=a"(result)
    : "0"(value)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  return result;
}

static uint64_t poly_riscv_trap_vector_mode_set(uint64_t value) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3400700b\n" // riscv trap-vector-mode set, a0=value.
    ".long 0x0000700b\n" // riscv x86 escape.
    : "=a"(result)
    : "0"(value)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  return result;
}

static uint64_t poly_riscv_monitor_packet_set_get(uint64_t value) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3800700b\n" // riscv monitor-packet set, a0=value.
    ".long 0x3a00700b\n" // riscv monitor-packet get, a0=result.
    ".long 0x0000700b\n" // riscv x86 escape.
    : "=a"(result)
    : "0"(value)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  return result;
}

static inline void poly_trap_vector_clear(void) {
  asm volatile(
    "xor %%eax,%%eax\n"
    POLY_OP_TRAP_VECTOR_SET
    "xor %%eax,%%eax\n"
    POLY_OP_TRAP_VECTOR_MODE_SET
    "xor %%eax,%%eax\n"
    POLY_OP_MONITOR_PACKET_SET
    :::
    "rax", "memory");
}

static inline void poly_state_export(struct poly_xsave_state *state) {
  asm volatile(POLY_OP_STATE_EXPORT :: "a"(state) : "memory");
}

static inline void poly_state_import(struct poly_xsave_state *state) {
  asm volatile(POLY_OP_STATE_IMPORT :: "a"(state) : "memory");
}

static __attribute__((noinline)) uint64_t
nativecheck_aarch64_read_tls(uint64_t tls_base) {
  uint64_t result;
  asm volatile(
    "movq %1, %%r13\n"
    POLY_OP_ENTER_A64
    ".long 0xd53bd040\n" // mrs x0,tpidr_el0
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result)
    : "r"(tls_base)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
      "r11", "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_riscv_read_tls(uint64_t tls_base) {
  uint64_t result;
  asm volatile(
    "movq %1, %%r13\n"
    POLY_OP_ENTER_RV64
    ".long 0x00020513\n" // addi a0,tp,0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "=a"(result)
    : "r"(tls_base)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
      "r11", "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_aarch64_switch_riscv_read_tls(uint64_t tls_base) {
  uint64_t result;
  asm volatile(
    "movq %1, %%r13\n"
    POLY_OP_ENTER_A64
    ".long 0xd5032e3f\n" // aarch64 polyctrl switch to RISC-V
    ".long 0x00020513\n" // addi a0,tp,0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "=a"(result)
    : "r"(tls_base)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
      "r11", "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_riscv_switch_aarch64_read_tls(uint64_t tls_base) {
  uint64_t result;
  asm volatile(
    "movq %1, %%r13\n"
    POLY_OP_ENTER_RV64
    ".long 0x0200700b\n" // riscv polyctrl switch to AArch64
    ".long 0xd53bd040\n" // mrs x0,tpidr_el0
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result)
    : "r"(tls_base)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
      "r11", "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
poly_abi_signature_set(uint64_t slot, uint64_t kind) {
  uint64_t rax = slot;
  uint64_t rdx = kind;
  asm volatile(POLY_OP_ABI_SIGNATURE_SET
      : "+a"(rax), "+d"(rdx)
      :
      : "memory");
  return rax;
}

static __attribute__((noinline)) uint64_t
poly_abi_signature_get(uint64_t slot) {
  uint64_t rax = slot;
  asm volatile(POLY_OP_ABI_SIGNATURE_GET
      : "+a"(rax)
      :
      : "memory");
  return rax;
}

static int check_poly_abi_signature_slot_default(uint32_t slot, uint32_t kind,
    const char *name) {
  uint64_t actual = poly_abi_signature_get(slot);
  if (actual != kind) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: ABI signature default mismatch %s slot=%u expected=%u actual=%llu\n",
      name, slot, kind, (unsigned long long) actual);
    return 1;
  }
  return 0;
}

static __attribute__((noinline)) uint64_t
nativecheck_aarch64_abi_signature_set_get_slot5(void) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28000a0\n" // movz x0,#5 (signature slot)
    ".long 0xd2800081\n" // movz x1,#4 (native-register kind)
    ".long 0xd5032f9f\n" // aarch64 ABI_SIGNATURE_SET
    ".long 0xd28000a0\n" // movz x0,#5
    ".long 0xd5032fbf\n" // aarch64 ABI_SIGNATURE_GET
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_riscv_abi_signature_set_get_slot5(void) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00500513\n" // addi a0,zero,5 (signature slot)
    ".long 0x00400593\n" // addi a1,zero,4 (native-register kind)
    ".long 0x1800700b\n" // riscv ABI_SIGNATURE_SET
    ".long 0x00500513\n" // addi a0,zero,5
    ".long 0x1a00700b\n" // riscv ABI_SIGNATURE_GET
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "=a"(result)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_aarch64_abi_signature_set_get_slot6_i128(void) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28000c0\n" // movz x0,#6 (signature slot)
    ".long 0xd28000a1\n" // movz x1,#5 (native-register i128 kind)
    ".long 0xd5032f9f\n" // aarch64 ABI_SIGNATURE_SET
    ".long 0xd28000c0\n" // movz x0,#6
    ".long 0xd5032fbf\n" // aarch64 ABI_SIGNATURE_GET
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_aarch64_abi_signature_set(uint64_t slot, uint64_t kind) {
  uint64_t result = slot;
  register uint64_t kind_reg asm("rdx") = kind;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032f9f\n" // aarch64 ABI_SIGNATURE_SET, x0=slot, x1=kind
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(result), "+d"(kind_reg)
    :
    : "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_aarch64_abi_signature_get(uint64_t slot) {
  uint64_t result = slot;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032fbf\n" // aarch64 ABI_SIGNATURE_GET, x0=slot
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(result)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_riscv_abi_signature_set_get_slot6_i128(void) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00600513\n" // addi a0,zero,6 (signature slot)
    ".long 0x00500593\n" // addi a1,zero,5 (native-register i128 kind)
    ".long 0x1800700b\n" // riscv ABI_SIGNATURE_SET
    ".long 0x00600513\n" // addi a0,zero,6
    ".long 0x1a00700b\n" // riscv ABI_SIGNATURE_GET
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "=a"(result)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_riscv_abi_signature_set(uint64_t slot, uint64_t kind) {
  uint64_t result = slot;
  register uint64_t kind_reg asm("rdx") = kind;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x1800700b\n" // riscv ABI_SIGNATURE_SET, a0=slot, a1=kind
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(result), "+d"(kind_reg)
    :
    : "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_riscv_abi_signature_get(uint64_t slot) {
  uint64_t result = slot;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x1a00700b\n" // riscv ABI_SIGNATURE_GET, a0=slot
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(result)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
poly_landing_policy_set(uint64_t policy) {
  uint64_t rax = policy;
  asm volatile(POLY_OP_LANDING_POLICY_SET
      : "+a"(rax)
      :
      : "memory");
  return rax;
}

static __attribute__((noinline)) uint64_t
poly_landing_policy_get(void) {
  uint64_t rax;
  asm volatile(POLY_OP_LANDING_POLICY_GET
      : "=a"(rax)
      :
      : "memory");
  return rax;
}

static __attribute__((noinline)) uint64_t
nativecheck_aarch64_landing_policy_set_get(uint64_t policy) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032fdf\n" // aarch64 LANDING_POLICY_SET, x0=policy
    ".long 0xd5032fff\n" // aarch64 LANDING_POLICY_GET, x0=result
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result)
    : "0"(policy)
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_aarch64_landing_policy_set(uint64_t policy) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd5032fdf\n" // aarch64 LANDING_POLICY_SET, x0=policy
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result)
    : "0"(policy)
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_riscv_landing_policy_set_get(uint64_t policy) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3c00700b\n" // riscv LANDING_POLICY_SET, a0=policy
    ".long 0x3e00700b\n" // riscv LANDING_POLICY_GET, a0=result
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "=a"(result)
    : "0"(policy)
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static __attribute__((noinline)) uint64_t
nativecheck_riscv_landing_policy_set(uint64_t policy) {
  uint64_t result;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x3c00700b\n" // riscv LANDING_POLICY_SET, a0=policy
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "=a"(result)
    : "0"(policy)
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

static struct poly_xsave_state
  nativecheck_import_live_state __attribute__((aligned(64)));
static struct poly_xsave_state
  nativecheck_import_restore_state __attribute__((aligned(64)));
static unsigned nativecheck_import_helper_calls;
static unsigned nativecheck_direct_x86_helper_calls;
static unsigned nativecheck_direct_x86_i128_helper_calls;
static unsigned nativecheck_direct_x86_fp64_helper_calls;
static unsigned nativecheck_direct_x86_vec128_helper_calls;
static unsigned nativecheck_descriptor_target_calls;
static uint64_t nativecheck_expected_source_sp __attribute__((used));
enum {
  NATIVECHECK_IMPORT_FUNC_STRLEN = 8,
  NATIVECHECK_IMPORT_DESCRIPTOR_QWORDS =
    POLY_IMPORT_X86_DESCRIPTOR_SIZE / sizeof(uint64_t),
  NATIVECHECK_IMPORT_DESCRIPTOR_QWORD_COUNT =
    (NATIVECHECK_IMPORT_FUNC_STRLEN + 1) *
    NATIVECHECK_IMPORT_DESCRIPTOR_QWORDS
};
static uint64_t nativecheck_import_descriptor_table[
  NATIVECHECK_IMPORT_DESCRIPTOR_QWORD_COUNT] __attribute__((aligned(64)));
static sigjmp_buf nativecheck_sigill_env;
static volatile sig_atomic_t nativecheck_expect_sigill;

static void nativecheck_sigill_handler(int signal_number) {
  (void) signal_number;
  if (nativecheck_expect_sigill)
    siglongjmp(nativecheck_sigill_env, 1);
  _exit(98);
}

__attribute__((noinline, used))
static uint64_t nativecheck_import_x86_sum6(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  nativecheck_import_helper_calls++;
  poly_state_export(&nativecheck_import_live_state);
  memcpy(&nativecheck_import_restore_state, &nativecheck_import_live_state,
    sizeof(nativecheck_import_restore_state));
  nativecheck_import_restore_state.import_return.top = 0;
  poly_state_import(&nativecheck_import_restore_state);
  poly_state_import(&nativecheck_import_live_state);
  return a0 + a1 + a2 + a3 + a4 + a5;
}

__attribute__((noinline, noipa, used))
static uint64_t nativecheck_direct_x86_sum6(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  nativecheck_direct_x86_helper_calls++;
  return a0 + a1 + a2 + a3 + a4 + a5;
}

__attribute__((noinline, noipa, used))
static unsigned __int128 nativecheck_direct_x86_i128(uint64_t lo,
    uint64_t hi) {
  nativecheck_direct_x86_i128_helper_calls++;
  return ((unsigned __int128) (hi + 0x20) << 64) | (lo + 0x10);
}

__attribute__((naked, noinline, used))
static void nativecheck_direct_x86_fp64_mul(void) {
  __asm__(
    "incl nativecheck_direct_x86_fp64_helper_calls(%rip)\n"
    "mulsd %xmm1,%xmm0\n"
    "ret\n");
}

__attribute__((naked, noinline, used))
static void nativecheck_direct_x86_vec128_add(void) {
  __asm__(
    "incl nativecheck_direct_x86_vec128_helper_calls(%rip)\n"
    "paddq %xmm1,%xmm0\n"
    "ret\n");
}

__attribute__((naked, noinline, used))
static void nativecheck_direct_x86_source_sp_matches(void) {
  __asm__(
    "movq nativecheck_expected_source_sp(%rip), %rax\n"
    "cmpq %rax, %r11\n"
    "sete %al\n"
    "movzbq %al, %rax\n"
    "ret\n");
}

__attribute__((noreturn, noinline, noipa, used))
static void nativecheck_descriptor_target_should_not_run(void) {
  nativecheck_descriptor_target_calls++;
  _exit(97);
}

static void nativecheck_install_descriptor_poison(void) {
  memset(nativecheck_import_descriptor_table, 0,
    sizeof(nativecheck_import_descriptor_table));
  const size_t base = (size_t) NATIVECHECK_IMPORT_FUNC_STRLEN *
    NATIVECHECK_IMPORT_DESCRIPTOR_QWORDS;
  nativecheck_import_descriptor_table[base] =
    (uint64_t) (uintptr_t) nativecheck_descriptor_target_should_not_run;
  nativecheck_import_descriptor_table[base + 1] =
    (uint64_t) (uintptr_t) nativecheck_descriptor_target_should_not_run;
}

static inline uint64_t read_xcr0(void) {
  uint32_t eax;
  uint32_t edx;
  asm volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0) : "memory");
  return ((uint64_t) edx << 32) | eax;
}

static long native_arch_prctl(int code, unsigned long addr) {
  return syscall(SYS_arch_prctl, code, addr);
}

static inline void native_xsave64(void *area, uint64_t mask) {
  uint32_t eax = (uint32_t) mask;
  uint32_t edx = (uint32_t) (mask >> 32);
  asm volatile("xsave64 %0"
    : "+m" (*(poly_native_xsave_area_t *) area)
    : "a" (eax), "d" (edx)
    : "memory");
}

static inline void native_xrstor64(void *area, uint64_t mask) {
  uint32_t eax = (uint32_t) mask;
  uint32_t edx = (uint32_t) (mask >> 32);
  asm volatile("xrstor64 %0"
    :
    : "m" (*(poly_native_xsave_area_t *) area), "a" (eax), "d" (edx)
    : "memory");
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

static inline uint64_t poly_syscall_status_number(void) {
  uint64_t value;
  asm volatile(POLY_OP_SYSCALL_STATUS_NUMBER : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_syscall_status_mode(void) {
  uint64_t value;
  asm volatile(POLY_OP_SYSCALL_STATUS_MODE : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_break_status_number(void) {
  uint64_t value;
  asm volatile(POLY_OP_BREAK_STATUS_NUMBER : "=a"(value) :: "memory");
  return value;
}

static inline uint64_t poly_break_status_mode(void) {
  uint64_t value;
  asm volatile(POLY_OP_BREAK_STATUS_MODE : "=a"(value) :: "memory");
  return value;
}

__attribute__((naked, noinline, used))
static uint64_t nativecheck_call_on_stack(uint64_t (*target)(void),
    void *stack_top) {
  __asm__(
    "pushq %r12\n"
    "movq %rsp,%r12\n"
    "movq %rsi,%rsp\n"
    "andq $-16,%rsp\n"
    "call *%rdi\n"
    "movq %r12,%rsp\n"
    "popq %r12\n"
    "ret\n");
}

__attribute__((naked, noinline, used))
static uint64_t nativecheck_x86_signature_pcall_aarch64_leaf(void) {
  __asm__(
    "leaq 1f(%rip),%rbx\n"
    "leaq 2f(%rip),%r11\n"
    "movq $1,%r15\n"
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT3
    "1:\n"
    ".long 0xd2800540\n" // movz x0,#42
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    "ret\n");
}

__attribute__((naked, noinline, used))
static uint64_t nativecheck_x86_signature_pcall_riscv_leaf(void) {
  __asm__(
    "leaq 1f(%rip),%rbx\n"
    "leaq 2f(%rip),%r11\n"
    "movq $2,%r15\n"
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT3
    "1:\n"
    ".long 0x02a00513\n" // addi a0,zero,42
    ".long 0x00008067\n" // ret
    "2:\n"
    "ret\n");
}

__attribute__((noreturn, noinline))
static void expect_signature_pcall_stackless_target(
    uint64_t (*target)(void)) {
  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0)
    _exit(97);

  const long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0)
    _exit(97);
  const size_t mapping_size = (size_t) page_size * 20U;
  void *mapping = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED)
    _exit(97);

  uint8_t *base = (uint8_t *) mapping;
  void *stack_top = base + ((size_t) page_size * 12U);
  uint64_t *source_stack = (uint64_t *) stack_top;
  uintptr_t foreign_rsp =
    (((uintptr_t) stack_top - 8U - 0x4000U) & ~(uintptr_t) 0xfU);
  uint64_t *foreign_stack = (uint64_t *) foreign_rsp;

  for (unsigned i = 0; i < 16; ++i) {
    source_stack[i] = 0x1111000000000000ULL + i;
    foreign_stack[i] = 0x2222000000000000ULL + i;
  }

  const uint64_t result = nativecheck_call_on_stack(
    target, stack_top);
  if (result != 42)
    _exit(96);

  for (unsigned i = 0; i < 16; ++i) {
    if (foreign_stack[i] == 0x1111000000000000ULL + i)
      _exit(80 + i);
  }

  _exit(0);
}

__attribute__((noreturn, noinline))
static void child_expect_signature_pcall_aarch64_stackless(void) {
  expect_signature_pcall_stackless_target(
    nativecheck_x86_signature_pcall_aarch64_leaf);
}

__attribute__((noreturn, noinline))
static void child_expect_signature_pcall_riscv_stackless(void) {
  expect_signature_pcall_stackless_target(
    nativecheck_x86_signature_pcall_riscv_leaf);
}

__attribute__((noreturn, noinline))
static void child_expect_aarch64_svc_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_ecall_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n" // ecall
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_aarch64_brk_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd42000a0\n" // brk #5
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_aarch64_brk1_signal(void) {
  static const char payload[] = "polyglot";
  register uint64_t rax __asm__("rax") = (uint64_t) (uintptr_t) payload;
  register uint64_t rdi __asm__("rdi") = (uint64_t) (uintptr_t) payload;
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd4200020\n" // brk #1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(rax), "+D"(rdi)
    :
    : "rbx", "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_ebreak_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00500893\n" // addi a7,zero,5
    ".long 0x00100073\n" // ebreak
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_ebreak1_signal(void) {
  static const char payload[] = "polyglot";
  register uint64_t rax __asm__("rax") = (uint64_t) (uintptr_t) payload;
  register uint64_t rdi __asm__("rdi") = (uint64_t) (uintptr_t) payload;
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00100893\n" // addi a7,zero,1
    ".long 0x00100073\n" // ebreak
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(rax), "+D"(rdi)
    :
    : "rbx", "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11",
      "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_compressed_ebreak_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00500893\n" // addi a7,zero,5
    ".short 0x9002\n" // c.ebreak
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_aarch64_import_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_A64
    ".long 0xd29c1010\n" // movz x16,#0xe080
    ".long 0xf2bffff0\n" // movk x16,#0xffff,lsl #16
    ".long 0xf2dffff0\n" // movk x16,#0xffff,lsl #32
    ".long 0xf2fffff0\n" // movk x16,#0xffff,lsl #48
    ".long 0xd28009a0\n" // movz x0,#77
    ".long 0xd63f0200\n" // blr x16, unresolved strlen descriptor
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_import_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> unresolved strlen descriptor
    ".long 0x04d00513\n" // addi a0,zero,77
    ".long 0x05800813\n" // addi a6,zero,88
    ".long 0x06300893\n" // addi a7,zero,99
    ".long 0x000280e7\n" // jalr ra,0(t0)
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_aarch64_illegal_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xffffffff\n" // unallocated in the supported AArch64 subset
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_illegal_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0xffffffff\n" // unallocated in the supported RISC-V subset
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_compressed_illegal_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".short 0x0000\n" // reserved 16-bit compressed encoding
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_invalid_generic_enter_frontend_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    "movq $255, %%r15\n"
    POLY_OP_ENTER_MODE
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "r15", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_state_key_set_signal(void) {
  uint64_t value = 0x53544154454b4559ULL;
  asm volatile(POLY_OP_STATE_KEY_SET : "+a"(value) :: "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_state_key_get_signal(void) {
  uint64_t value = 0;
  asm volatile(POLY_OP_STATE_KEY_GET : "=a"(value) :: "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_invalid_generic_switch_frontend_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    "leaq 1f(%%rip), %%rbx\n"
    "movq $255, %%r15\n"
    POLY_OP_SWITCH_MODE
    "1:\n"
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "r15", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_invalid_generic_pcall_frontend_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    "xorq %%r12, %%r12\n"
    "movq $255, %%r15\n"
    POLY_OP_PCALL_SIG_MODE
    "1:\n"
    "retq\n"
    "2:\n"
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_invalid_generic_pcall_slot_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    "movq $8, %%r12\n"
    "movq %0, %%r15\n"
    POLY_OP_PCALL_SIG_MODE
    "1:\n"
    "retq\n"
    "2:\n"
    :
    : "i"(POLY_FRONTEND_AARCH64)
    : "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_invalid_generic_pcall_imm_slot_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    "movq %0, %%r15\n"
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT8
    "1:\n"
    "retq\n"
    "2:\n"
    :
    : "i"(POLY_FRONTEND_AARCH64)
    : "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "r15", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_landing_policy_missing_pcall_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  if (poly_landing_policy_set(POLY_LANDING_POLICY_REQUIRE_CALL) != 0)
    _exit(97);
  asm volatile(
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    "movq %0, %%r15\n"
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT0
    "1:\n"
    ".long 0xd2800540\n" // movz x0,#42, intentionally no landing pad
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    :
    : "i"(POLY_FRONTEND_AARCH64)
    : "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "r15", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_landing_policy_missing_switch_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  if (poly_landing_policy_set(POLY_LANDING_POLICY_REQUIRE_SWITCH) != 0)
    _exit(97);
  asm volatile(
    "leaq 1f(%%rip), %%rbx\n"
    "movq %0, %%r15\n"
    POLY_OP_SWITCH_MODE
    "1:\n"
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape, no landing pad
    "2:\n"
    :
    : "i"(POLY_FRONTEND_AARCH64)
    : "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "r15", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_aarch64_invalid_generic_switch_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800010\n" // movz x16,#0
    ".long 0xd2801ff1\n" // movz x17,#255
    ".long 0xd5032f1f\n" // aarch64 generic switch frontend=x17 target=x16
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_aarch64_invalid_generic_call_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800010\n" // movz x16,#0
    ".long 0xd2801ff1\n" // movz x17,#255
    ".long 0xd2800012\n" // movz x18,#0
    ".long 0xd5032f3f\n" // aarch64 generic pcall frontend=x17 target=x16
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_aarch64_invalid_generic_signature_slot_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800010\n" // movz x16,#0
    ".long 0xd2800051\n" // movz x17,#2 (RISC-V frontend)
    ".long 0xd2800012\n" // movz x18,#0
    ".long 0xd2800113\n" // movz x19,#8 (invalid signature slot)
    ".long 0xd5032f5f\n" // aarch64 generic signature pcall
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_invalid_generic_switch_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00000293\n" // addi x5,zero,0
    ".long 0x0ff00313\n" // addi x6,zero,255
    ".long 0x1000700b\n" // riscv generic switch frontend=x6 target=x5
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_invalid_generic_call_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00000293\n" // addi x5,zero,0
    ".long 0x0ff00313\n" // addi x6,zero,255
    ".long 0x00000393\n" // addi x7,zero,0
    ".long 0x1200700b\n" // riscv generic pcall frontend=x6 target=x5
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_riscv_invalid_generic_signature_slot_signal(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00000293\n" // addi x5,zero,0
    ".long 0x00100313\n" // addi x6,zero,1 (AArch64 frontend)
    ".long 0x00000393\n" // addi x7,zero,0
    ".long 0x00800e13\n" // addi x28,zero,8 (invalid signature slot)
    ".long 0x1400700b\n" // riscv generic signature pcall
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_malformed_import_return_xsave_signal(void) {
  struct poly_xsave_state bad __attribute__((aligned(64)));
  memset(&bad, 0, sizeof(bad));
  poly_state_export(&bad);
  bad.import_return.top = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH + 1;
  bad.import_return.depth = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH;
  poly_state_import(&bad);
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_bad_import_return_mode_xsave_signal(void) {
  struct poly_xsave_state bad __attribute__((aligned(64)));
  memset(&bad, 0, sizeof(bad));
  poly_state_export(&bad);
  bad.import_return.top = 1;
  bad.import_return.depth = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH;
  bad.import_return.frames[0].source_mode = POLY_MODE_X86;
  bad.import_return.frames[0].import_id = 8;
  poly_state_import(&bad);
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_bad_import_return_id_xsave_signal(void) {
  struct poly_xsave_state bad __attribute__((aligned(64)));
  memset(&bad, 0, sizeof(bad));
  poly_state_export(&bad);
  bad.import_return.top = 1;
  bad.import_return.depth = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH;
  bad.import_return.frames[0].source_mode = POLY_MODE_RAW_AARCH64;
  bad.import_return.frames[0].import_id = POLY_IMPORT_FUNC_COUNT;
  poly_state_import(&bad);
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_malformed_cross_return_xsave_signal(void) {
  struct poly_xsave_state bad __attribute__((aligned(64)));
  memset(&bad, 0, sizeof(bad));
  poly_state_export(&bad);
  bad.cross_return.top = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH + 1;
  bad.cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
  poly_state_import(&bad);
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_bad_cross_return_mode_xsave_signal(void) {
  struct poly_xsave_state bad __attribute__((aligned(64)));
  memset(&bad, 0, sizeof(bad));
  poly_state_export(&bad);
  bad.cross_return.top = 1;
  bad.cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
  bad.cross_return.frames[0].caller_mode = 255;
  bad.cross_return.frames[0].target_mode = POLY_MODE_RAW_RISCV;
  bad.cross_return.frames[0].abi_kind = POLY_CROSS_BRIDGE_DEFAULT;
  poly_state_import(&bad);
  _exit(99);
}

__attribute__((noreturn, noinline))
static void child_expect_bad_cross_return_bridge_xsave_signal(void) {
  struct poly_xsave_state bad __attribute__((aligned(64)));
  memset(&bad, 0, sizeof(bad));
  poly_state_export(&bad);
  bad.cross_return.top = 1;
  bad.cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
  bad.cross_return.frames[0].caller_mode = POLY_MODE_RAW_AARCH64;
  bad.cross_return.frames[0].target_mode = POLY_MODE_RAW_RISCV;
  bad.cross_return.frames[0].abi_kind = 99;
  poly_state_import(&bad);
  _exit(99);
}

static int expect_child_signal(const char *name, int expected_signal,
    void (*child_func)(void)) {
  pid_t child = fork();
  if (child < 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: %s fork failed\n", name);
    return 1;
  }
  if (child == 0)
    child_func();

  int status = 0;
  if (waitpid(child, &status, 0) != child) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: %s wait failed\n", name);
    return 1;
  }
  if (!WIFSIGNALED(status) || WTERMSIG(status) != expected_signal) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: %s expected signal %d status=0x%x\n",
      name, expected_signal, status);
    return 1;
  }
  return 0;
}

static int expect_child_exit(const char *name, int expected_status,
    void (*child_func)(void)) {
  pid_t child = fork();
  if (child < 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: %s fork failed\n", name);
    return 1;
  }
  if (child == 0)
    child_func();

  int status = 0;
  if (waitpid(child, &status, 0) != child) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: %s wait failed\n", name);
    return 1;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != expected_status) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: %s expected exit %d status=0x%x\n",
      name, expected_status, status);
    return 1;
  }
  return 0;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_landing_policy_pcall_aarch64(void) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%rbx\n"
    "leaq 2f(%%rip), %%r11\n"
    "movq %1, %%r15\n"
    POLY_OP_PCALL_SIG_IMM_MODE_SLOT0
    "1:\n"
    ".long 0xd5032f7f\n" // aarch64 landing pad
    ".long 0xd2800540\n" // movz x0,#42
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result)
    : "i"(POLY_FRONTEND_AARCH64)
    : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
      "r11", "r13", "r14", "r15", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_landing_policy_switch_aarch64(void) {
  uint64_t result = 0;
  asm volatile(
    "leaq 1f(%%rip), %%rbx\n"
    "movq %1, %%r15\n"
    POLY_OP_SWITCH_MODE
    "1:\n"
    ".long 0xd5032f7f\n" // aarch64 landing pad
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    "2:\n"
    "movq $1, %0\n"
    : "=r"(result)
    : "i"(POLY_FRONTEND_AARCH64)
    : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9",
      "r10", "r11", "r13", "r14", "r15", "memory");
  return result;
}

static int run_poly_no_vector_signal_probe(void) {
  if (expect_child_signal("poly aarch64 svc no-vector", SIGILL,
        child_expect_aarch64_svc_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv ecall no-vector", SIGILL,
        child_expect_riscv_ecall_signal) != 0)
    return 1;
  if (expect_child_signal("poly aarch64 brk no-vector", SIGTRAP,
        child_expect_aarch64_brk_signal) != 0)
    return 1;
  if (expect_child_signal("poly aarch64 brk #1 no-vector", SIGTRAP,
        child_expect_aarch64_brk1_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv ebreak no-vector", SIGTRAP,
        child_expect_riscv_ebreak_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv ebreak id1 no-vector", SIGTRAP,
        child_expect_riscv_ebreak1_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv compressed ebreak no-vector", SIGTRAP,
        child_expect_riscv_compressed_ebreak_signal) != 0)
    return 1;
  if (expect_child_signal("poly aarch64 import no-vector", SIGILL,
        child_expect_aarch64_import_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv import no-vector", SIGILL,
        child_expect_riscv_import_signal) != 0)
    return 1;
  if (expect_child_signal("poly aarch64 illegal no-vector", SIGILL,
        child_expect_aarch64_illegal_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv illegal no-vector", SIGILL,
        child_expect_riscv_illegal_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv compressed illegal no-vector", SIGILL,
        child_expect_riscv_compressed_illegal_signal) != 0)
    return 1;

  puts("NATIVE_POLY_NO_VECTOR_SIGNALS_OK");
  return 0;
}

static int run_poly_invalid_generic_control_signal_probe(void) {
  if (expect_child_signal("poly invalid generic enter frontend", SIGILL,
        child_expect_invalid_generic_enter_frontend_signal) != 0)
    return 1;
  if (expect_child_signal("poly invalid generic switch frontend", SIGILL,
        child_expect_invalid_generic_switch_frontend_signal) != 0)
    return 1;
  if (expect_child_signal("poly invalid generic pcall frontend", SIGILL,
        child_expect_invalid_generic_pcall_frontend_signal) != 0)
    return 1;
  if (expect_child_signal("poly invalid generic pcall slot", SIGILL,
        child_expect_invalid_generic_pcall_slot_signal) != 0)
    return 1;
  if (expect_child_signal("poly invalid generic pcall immediate slot", SIGILL,
        child_expect_invalid_generic_pcall_imm_slot_signal) != 0)
    return 1;
  if (expect_child_signal("poly aarch64 invalid generic switch", SIGILL,
        child_expect_aarch64_invalid_generic_switch_signal) != 0)
    return 1;
  if (expect_child_signal("poly aarch64 invalid generic pcall", SIGILL,
        child_expect_aarch64_invalid_generic_call_signal) != 0)
    return 1;
  if (expect_child_signal("poly aarch64 invalid generic signature slot",
        SIGILL, child_expect_aarch64_invalid_generic_signature_slot_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv invalid generic switch", SIGILL,
        child_expect_riscv_invalid_generic_switch_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv invalid generic pcall", SIGILL,
        child_expect_riscv_invalid_generic_call_signal) != 0)
    return 1;
  if (expect_child_signal("poly riscv invalid generic signature slot", SIGILL,
        child_expect_riscv_invalid_generic_signature_slot_signal) != 0)
    return 1;

  puts("NATIVE_POLY_INVALID_GENERIC_CONTROLS_OK");
  return 0;
}

static int run_poly_landing_policy_probe(void) {
  uint64_t call_result;
  uint64_t switch_result;

  if (poly_landing_policy_set(POLY_LANDING_POLICY_SUPPORTED) != 0 ||
      poly_landing_policy_get() != POLY_LANDING_POLICY_SUPPORTED) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly landing policy set/get mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    return 1;
  }
  if (poly_landing_policy_set(POLY_LANDING_POLICY_SUPPORTED << 1) !=
      (uint64_t) -EINVAL) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly landing policy accepted invalid flags got=0x%llx\n",
      (unsigned long long)
        poly_landing_policy_set(POLY_LANDING_POLICY_SUPPORTED << 1));
    poly_landing_policy_set(0);
    return 1;
  }

  const uint64_t aarch64_policy = nativecheck_aarch64_landing_policy_set_get(
    POLY_LANDING_POLICY_REQUIRE_SWITCH);
  const uint64_t aarch64_policy_x86 = poly_landing_policy_get();
  if (aarch64_policy != POLY_LANDING_POLICY_REQUIRE_SWITCH ||
      aarch64_policy_x86 != POLY_LANDING_POLICY_REQUIRE_SWITCH) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 landing policy set/get mismatch foreign=0x%llx x86=0x%llx\n",
      (unsigned long long) aarch64_policy,
      (unsigned long long) aarch64_policy_x86);
    poly_landing_policy_set(0);
    return 1;
  }
  if (poly_landing_policy_set(0) != 0)
    return 1;

  const uint64_t aarch64_invalid_policy =
    nativecheck_aarch64_landing_policy_set(
      POLY_LANDING_POLICY_SUPPORTED << 1);
  const uint64_t aarch64_invalid_policy_x86 = poly_landing_policy_get();
  if (aarch64_invalid_policy != (uint64_t) -EINVAL ||
      aarch64_invalid_policy_x86 != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 landing policy invalid set mismatch result=0x%llx x86=0x%llx\n",
      (unsigned long long) aarch64_invalid_policy,
      (unsigned long long) aarch64_invalid_policy_x86);
    poly_landing_policy_set(0);
    return 1;
  }

  const uint64_t riscv_policy = nativecheck_riscv_landing_policy_set_get(
    POLY_LANDING_POLICY_REQUIRE_CALL);
  const uint64_t riscv_policy_x86 = poly_landing_policy_get();
  if (riscv_policy != POLY_LANDING_POLICY_REQUIRE_CALL ||
      riscv_policy_x86 != POLY_LANDING_POLICY_REQUIRE_CALL) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv landing policy set/get mismatch foreign=0x%llx x86=0x%llx\n",
      (unsigned long long) riscv_policy,
      (unsigned long long) riscv_policy_x86);
    poly_landing_policy_set(0);
    return 1;
  }
  if (poly_landing_policy_set(0) != 0)
    return 1;

  const uint64_t riscv_invalid_policy =
    nativecheck_riscv_landing_policy_set(
      POLY_LANDING_POLICY_SUPPORTED << 1);
  const uint64_t riscv_invalid_policy_x86 = poly_landing_policy_get();
  if (riscv_invalid_policy != (uint64_t) -EINVAL ||
      riscv_invalid_policy_x86 != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv landing policy invalid set mismatch result=0x%llx x86=0x%llx\n",
      (unsigned long long) riscv_invalid_policy,
      (unsigned long long) riscv_invalid_policy_x86);
    poly_landing_policy_set(0);
    return 1;
  }

  if (poly_landing_policy_set(POLY_LANDING_POLICY_REQUIRE_CALL) != 0)
    return 1;
  call_result = nativecheck_landing_policy_pcall_aarch64();
  if (poly_landing_policy_set(0) != 0)
    return 1;
  if (call_result != 42) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly landing policy pcall result=%llu\n",
      (unsigned long long) call_result);
    return 1;
  }

  if (poly_landing_policy_set(POLY_LANDING_POLICY_REQUIRE_SWITCH) != 0)
    return 1;
  switch_result = nativecheck_landing_policy_switch_aarch64();
  if (poly_landing_policy_set(0) != 0)
    return 1;
  if (switch_result != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly landing policy switch result=%llu\n",
      (unsigned long long) switch_result);
    return 1;
  }

  if (expect_child_signal("poly landing policy missing pcall", SIGILL,
        child_expect_landing_policy_missing_pcall_signal) != 0) {
    poly_landing_policy_set(0);
    return 1;
  }
  if (poly_landing_policy_set(0) != 0)
    return 1;
  if (expect_child_signal("poly landing policy missing switch", SIGILL,
        child_expect_landing_policy_missing_switch_signal) != 0) {
    poly_landing_policy_set(0);
    return 1;
  }
  if (poly_landing_policy_set(0) != 0)
    return 1;

  if (poly_landing_policy_get() != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly landing policy leaked flags=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    poly_landing_policy_set(0);
    return 1;
  }

  puts("NATIVE_POLY_LANDING_POLICY_OK");
  return 0;
}

__attribute__((naked, noinline, used))
static void poly_trap_vector_handler(void) {
  __asm__(
    "cmpq $1, %rax\n"
    "jne 3f\n"
    "cmpq $3, %rbx\n"
    "jne 1f\n"
    "cmpq $172, %rcx\n"
    "jne 9f\n"
    "cmpq $7, %rsi\n"
    "jne 9f\n"
    "movq $39, %rax\n"
    "syscall\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "1:\n"
    "cmpq $4, %rbx\n"
    "jne 9f\n"
    "cmpq $172, %rcx\n"
    "jne 9f\n"
    "cmpq $0, %rsi\n"
    "jne 9f\n"
    "movq $39, %rax\n"
    "syscall\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "3:\n"
    "cmpq $2, %rax\n"
    "je 4f\n"
    "cmpq $3, %rax\n"
    "je 31f\n"
    "cmpq $4, %rax\n"
    "je 6f\n"
    "jmp 9f\n"
    "31:\n"
    "cmpq $3, %rbx\n"
    "je 32f\n"
    "cmpq $4, %rbx\n"
    "jne 9f\n"
    "32:\n"
    "cmpq $8, %rcx\n"
    "jne 9f\n"
    "cmpq $0, %rsi\n"
    "jne 9f\n"
    "cmpq $77, %rdi\n"
    "jne 9f\n"
    "cmpq $88, %r13\n"
    "jne 9f\n"
    "cmpq $99, %r14\n"
    "jne 9f\n"
    "movq $5555, %rax\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "4:\n"
    "cmpq $3, %rbx\n"
    "jne 5f\n"
    "cmpq $5, %rcx\n"
    "jne 9f\n"
    "cmpq $5, %rsi\n"
    "jne 9f\n"
    "cmpq $11, %rdi\n"
    "jne 9f\n"
    "cmpq $12, %r8\n"
    "jne 9f\n"
    "cmpq $13, %r9\n"
    "jne 9f\n"
    "cmpq $14, %r10\n"
    "jne 9f\n"
    "cmpq $15, %r11\n"
    "jne 9f\n"
    "cmpq $16, %r12\n"
    "jne 9f\n"
    "cmpq $17, %r13\n"
    "jne 9f\n"
    "cmpq $18, %r14\n"
    "jne 9f\n"
    "movq $4444, %rax\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "5:\n"
    "cmpq $4, %rbx\n"
    "jne 9f\n"
    "cmpq $5, %rcx\n"
    "jne 9f\n"
    "cmpq $0, %rsi\n"
    "jne 9f\n"
    "cmpq $21, %rdi\n"
    "jne 9f\n"
    "cmpq $22, %r8\n"
    "jne 9f\n"
    "cmpq $23, %r9\n"
    "jne 9f\n"
    "cmpq $24, %r10\n"
    "jne 9f\n"
    "cmpq $25, %r11\n"
    "jne 9f\n"
    "cmpq $26, %r12\n"
    "jne 9f\n"
    "cmpq $27, %r13\n"
    "jne 9f\n"
    "cmpq $5, %r14\n"
    "jne 9f\n"
    "movq $4545, %rax\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "6:\n"
    "cmpq $3, %rbx\n"
    "jne 7f\n"
    "cmpl $0xffffffff, %ecx\n"
    "jne 9f\n"
    "cmpq $4, %rsi\n"
    "jne 9f\n"
    "movq $4664, %rax\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "7:\n"
    "cmpq $4, %rbx\n"
    "jne 9f\n"
    "cmpl $0xffffffff, %ecx\n"
    "jne 8f\n"
    "cmpq $4, %rsi\n"
    "jne 9f\n"
    "movq $4665, %rax\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "8:\n"
    "cmpq $0, %rcx\n"
    "jne 9f\n"
    "cmpq $2, %rsi\n"
    "jne 9f\n"
    "movq $4666, %rax\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    "9:\n"
    "movq $0xffffffffffffffff, %rax\n"
    "pxor %xmm0, %xmm0\n"
    POLY_OP_TRAP_RETURN
    "ud2\n");
}

extern const unsigned char poly_aarch64_trap_vector_raw[];
extern const unsigned char poly_aarch64_trap_vector_ext_raw[];
extern const unsigned char poly_riscv_trap_vector_raw[];
extern const unsigned char poly_riscv_trap_vector_ext_raw[];

__asm__(
  ".pushsection .text\n"
  ".balign 4\n"
  ".globl poly_aarch64_trap_vector_raw\n"
  ".type poly_aarch64_trap_vector_raw,@function\n"
  "poly_aarch64_trap_vector_raw:\n"
  ".long 0xaa0a03e0\n" // mov x0,x10, return trap arg5
  ".long 0xd2800f6b\n" // movz x11,#123, deliberate handler clobber
  ".long 0xd5032edf\n" // aarch64 polyctrl trap return, architectural trap return
  "ud2\n"
  ".size poly_aarch64_trap_vector_raw, .-poly_aarch64_trap_vector_raw\n"
  ".balign 4\n"
  ".globl poly_aarch64_trap_vector_ext_raw\n"
  ".type poly_aarch64_trap_vector_ext_raw,@function\n"
  "poly_aarch64_trap_vector_ext_raw:\n"
  ".long 0x8b0c0160\n" // add x0,x11,x12, return trap arg6+arg7
  ".long 0xd5032edf\n" // aarch64 polyctrl trap return, architectural trap return
  "ud2\n"
  ".size poly_aarch64_trap_vector_ext_raw, .-poly_aarch64_trap_vector_ext_raw\n"
  ".balign 4\n"
  ".globl poly_riscv_trap_vector_raw\n"
  ".type poly_riscv_trap_vector_raw,@function\n"
  "poly_riscv_trap_vector_raw:\n"
  ".long 0x00038513\n" // addi a0,t2,0, return trap arg5
  ".long 0x07b00913\n" // addi s2,zero,123, deliberate handler clobber
  ".long 0x0c00700b\n" // riscv polyctrl trap return
  "ud2\n"
  ".size poly_riscv_trap_vector_raw, .-poly_riscv_trap_vector_raw\n"
  ".balign 4\n"
  ".globl poly_riscv_trap_vector_ext_raw\n"
  ".type poly_riscv_trap_vector_ext_raw,@function\n"
  "poly_riscv_trap_vector_ext_raw:\n"
  ".long 0x01de0533\n" // add a0,t3,t4, return trap arg6+arg7
  ".long 0x0c00700b\n" // riscv polyctrl trap return
  "ud2\n"
  ".size poly_riscv_trap_vector_ext_raw, .-poly_riscv_trap_vector_ext_raw\n"
  ".popsection\n");

static int run_poly_trap_vector_probe(void) {
  void *handler = (void *) poly_trap_vector_handler;
  uint64_t expected_pid = (uint64_t) getpid();
  struct nativecheck_monitor_packet monitor_packet __attribute__((aligned(64)));
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value((uint64_t) handler);
  poly_monitor_packet_set_value((uint64_t) (uintptr_t) &monitor_packet);
  poly_trap_vector_get();
  if (read_rax() != (uint64_t) handler) {
    fputs("NATIVE_CHECK_FAIL: poly trap vector get mismatch\n", stderr);
    return 1;
  }
  poly_trap_vector_mode_get();
  if (read_rax() != POLY_MODE_X86) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly trap vector mode get mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (poly_trap_vector_mode_set_result(255) != (uint64_t) -EINVAL) {
    fputs("NATIVE_CHECK_FAIL: poly x86 trap vector mode accepted invalid mode\n",
      stderr);
    return 1;
  }
  poly_trap_vector_mode_get();
  if (read_rax() != POLY_MODE_X86) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly x86 invalid trap vector mode mutated state got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  poly_monitor_packet_get();
  if (read_rax() != (uint64_t) (uintptr_t) &monitor_packet) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly monitor packet get mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (poly_aarch64_trap_vector_set_get((uint64_t) handler) !=
      (uint64_t) handler) {
    fputs("NATIVE_CHECK_FAIL: poly aarch64 trap vector set/get mismatch\n",
      stderr);
    return 1;
  }
  if (poly_aarch64_trap_vector_mode_set_get(POLY_MODE_X86) !=
      POLY_MODE_X86) {
    fputs("NATIVE_CHECK_FAIL: poly aarch64 trap vector mode set/get mismatch\n",
      stderr);
    return 1;
  }
  if (poly_aarch64_trap_vector_mode_set(255) != (uint64_t) -EINVAL) {
    fputs("NATIVE_CHECK_FAIL: poly aarch64 trap vector mode accepted invalid mode\n",
      stderr);
    return 1;
  }
  poly_trap_vector_mode_get();
  if (read_rax() != POLY_MODE_X86) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 invalid trap vector mode mutated state got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (poly_aarch64_monitor_packet_set_get(
        (uint64_t) (uintptr_t) &monitor_packet) !=
      (uint64_t) (uintptr_t) &monitor_packet) {
    fputs("NATIVE_CHECK_FAIL: poly aarch64 monitor packet set/get mismatch\n",
      stderr);
    return 1;
  }
  if (poly_riscv_trap_vector_set_get((uint64_t) handler) !=
      (uint64_t) handler) {
    fputs("NATIVE_CHECK_FAIL: poly riscv trap vector set/get mismatch\n",
      stderr);
    return 1;
  }
  if (poly_riscv_trap_vector_mode_set_get(POLY_MODE_X86) !=
      POLY_MODE_X86) {
    fputs("NATIVE_CHECK_FAIL: poly riscv trap vector mode set/get mismatch\n",
      stderr);
    return 1;
  }
  if (poly_riscv_trap_vector_mode_set(255) != (uint64_t) -EINVAL) {
    fputs("NATIVE_CHECK_FAIL: poly riscv trap vector mode accepted invalid mode\n",
      stderr);
    return 1;
  }
  poly_trap_vector_mode_get();
  if (read_rax() != POLY_MODE_X86) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv invalid trap vector mode mutated state got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (poly_riscv_monitor_packet_set_get(
        (uint64_t) (uintptr_t) &monitor_packet) !=
      (uint64_t) (uintptr_t) &monitor_packet) {
    fputs("NATIVE_CHECK_FAIL: poly riscv monitor packet set/get mismatch\n",
      stderr);
    return 1;
  }
  pid_t child = fork();
  if (child < 0) {
    fputs("NATIVE_CHECK_FAIL: poly trap vector fork failed\n", stderr);
    return 1;
  }
  if (child == 0) {
    poly_trap_vector_get();
    if (read_rax() != 0)
      _exit(11);
    poly_trap_vector_mode_get();
    if (read_rax() != POLY_MODE_X86)
      _exit(12);
    poly_monitor_packet_get();
    if (read_rax() != 0)
      _exit(13);
    _exit(0);
  }
  int status = 0;
  if (waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
      WEXITSTATUS(status) != 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly trap vector leaked across address space status=0x%x\n",
      status);
    return 1;
  }
  poly_trap_vector_get();
  if (read_rax() != (uint64_t) handler) {
    fputs("NATIVE_CHECK_FAIL: poly parent trap vector lost after fork\n", stderr);
    return 1;
  }
  poly_monitor_packet_get();
  if (read_rax() != (uint64_t) (uintptr_t) &monitor_packet) {
    fputs("NATIVE_CHECK_FAIL: poly parent monitor packet address lost after fork\n",
      stderr);
    return 1;
  }

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800366\n" // movz x6,#27
    ".long 0xd28000a7\n" // movz x7,#5
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  uint64_t result = read_rax();
  if (result != expected_pid) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 svc trap vector result mismatch got=%llu expected=%llu\n",
      (unsigned long long) result, (unsigned long long) expected_pid);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_SYSCALL) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent trap packet reason mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_reason());
    return 1;
  }
  if (poly_syscall_status_number() != 172 ||
      poly_syscall_status_mode() != POLY_MODE_RAW_AARCH64 ||
      poly_trap_status_selector() != 7) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent syscall status mismatch number=%llu mode=%llu selector=%llu\n",
      (unsigned long long) poly_syscall_status_number(),
      (unsigned long long) poly_syscall_status_mode(),
      (unsigned long long) poly_trap_status_selector());
    return 1;
  }
  if (poly_trap_status_arg6() != 27 || poly_trap_status_arg7() != 5) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 syscall packet extended args mismatch arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (monitor_packet.trap.reason != POLY_TRAP_SYSCALL ||
      monitor_packet.trap.source_mode != POLY_MODE_RAW_AARCH64 ||
      monitor_packet.trap.number != 172 ||
      monitor_packet.trap.selector != 7 ||
      monitor_packet.args[6] != 27 || monitor_packet.args[7] != 5 ||
      (monitor_packet.trap.flags & POLY_TRAP_PACKET_FLAG_MONITOR_MEMORY) == 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly monitor packet aarch64 mismatch reason=%u mode=%u number=%llu selector=%llu arg6=%llu arg7=%llu flags=0x%llx\n",
      monitor_packet.trap.reason, monitor_packet.trap.source_mode,
      (unsigned long long) monitor_packet.trap.number,
      (unsigned long long) monitor_packet.trap.selector,
      (unsigned long long) monitor_packet.args[6],
      (unsigned long long) monitor_packet.args[7],
      (unsigned long long) monitor_packet.trap.flags);
    return 1;
  }
  pid_t trap_child = fork();
  if (trap_child < 0) {
    fputs("NATIVE_CHECK_FAIL: poly trap packet fork failed\n", stderr);
    return 1;
  }
  if (trap_child == 0) {
    if (poly_trap_status_reason() != 0)
      _exit(21);
    if (poly_syscall_status_number() != 0)
      _exit(22);
    if (poly_syscall_status_mode() != POLY_MODE_X86)
      _exit(23);
    _exit(0);
  }
  status = 0;
  if (waitpid(trap_child, &status, 0) != trap_child || !WIFEXITED(status) ||
      WEXITSTATUS(status) != 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly trap packet leaked across address space status=0x%x\n",
      status);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_SYSCALL) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent trap packet lost after fork got=%llu\n",
      (unsigned long long) poly_trap_status_reason());
    return 1;
  }
  if (poly_syscall_status_number() != 172 ||
      poly_syscall_status_mode() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent syscall status lost after fork number=%llu mode=%llu\n",
      (unsigned long long) poly_syscall_status_number(),
      (unsigned long long) poly_syscall_status_mode());
    return 1;
  }

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01b00813\n" // addi a6,zero,27
    ".long 0x0ac00893\n" // addi x17,x0,172
    ".long 0x00000073\n" // ecall
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != expected_pid) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv ecall trap vector result mismatch got=%llu expected=%llu\n",
      (unsigned long long) result, (unsigned long long) expected_pid);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_SYSCALL ||
      poly_syscall_status_number() != 172 ||
      poly_syscall_status_mode() != POLY_MODE_RAW_RISCV ||
      poly_trap_status_selector() != 0 ||
      poly_trap_status_arg6() != 27 || poly_trap_status_arg7() != 172) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv syscall packet mismatch reason=%llu number=%llu mode=%llu selector=%llu arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_syscall_status_number(),
      (unsigned long long) poly_syscall_status_mode(),
      (unsigned long long) poly_trap_status_selector(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (monitor_packet.trap.reason != POLY_TRAP_SYSCALL ||
      monitor_packet.trap.source_mode != POLY_MODE_RAW_RISCV ||
      monitor_packet.trap.number != 172 ||
      monitor_packet.trap.selector != 0 ||
      monitor_packet.args[6] != 27 || monitor_packet.args[7] != 172 ||
      (monitor_packet.trap.flags & POLY_TRAP_PACKET_FLAG_MONITOR_MEMORY) == 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly monitor packet riscv mismatch reason=%u mode=%u number=%llu selector=%llu arg6=%llu arg7=%llu flags=0x%llx\n",
      monitor_packet.trap.reason, monitor_packet.trap.source_mode,
      (unsigned long long) monitor_packet.trap.number,
      (unsigned long long) monitor_packet.trap.selector,
      (unsigned long long) monitor_packet.args[6],
      (unsigned long long) monitor_packet.args[7],
      (unsigned long long) monitor_packet.trap.flags);
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0x8b020020\n" // add x0,x1,x2
    ".long 0x8b030000\n" // add x0,x0,x3
    ".long 0x8b040000\n" // add x0,x0,x4
    ".long 0x8b050000\n" // add x0,x0,x5
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 20) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 trap return preserved args mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n" // ecall
    ".long 0x00c58533\n" // add a0,a1,a2
    ".long 0x00d50533\n" // add a0,a0,a3
    ".long 0x00e50533\n" // add a0,a0,a4
    ".long 0x00f50533\n" // add a0,a0,a5
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 20) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv trap return preserved args mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0xaa0803e0\n" // mov x0,x8
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 172) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 trap return preserved syscall register mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n" // ecall
    ".long 0x00088513\n" // addi a0,a7,0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 172) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv trap return preserved syscall register mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  write_xmm0_u64(0x4008000000000000ULL);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  uint64_t fp_result = read_xmm0_u64();
  if (fp_result != 0x4008000000000000ULL) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 trap return preserved fp register mismatch got=0x%llx\n",
      (unsigned long long) fp_result);
    return 1;
  }

  write_xmm0_u64(0x4010000000000000ULL);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n" // ecall
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  fp_result = read_xmm0_u64();
  if (fp_result != 0x4010000000000000ULL) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv trap return preserved fp register mismatch got=0x%llx\n",
      (unsigned long long) fp_result);
    return 1;
  }

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800160\n" // movz x0,#11
    ".long 0xd2800181\n" // movz x1,#12
    ".long 0xd28001a2\n" // movz x2,#13
    ".long 0xd28001c3\n" // movz x3,#14
    ".long 0xd28001e4\n" // movz x4,#15
    ".long 0xd2800205\n" // movz x5,#16
    ".long 0xd2800226\n" // movz x6,#17
    ".long 0xd2800247\n" // movz x7,#18
    ".long 0xd42000a0\n" // brk #5
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 4444) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 brk trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_BREAK) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent break packet reason mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_reason());
    return 1;
  }
  if (poly_break_status_number() != 5 ||
      poly_break_status_mode() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent break status mismatch number=%llu mode=%llu\n",
      (unsigned long long) poly_break_status_number(),
      (unsigned long long) poly_break_status_mode());
    return 1;
  }
  if (poly_trap_status_arg6() != 17 || poly_trap_status_arg7() != 18) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 break packet extended args mismatch arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (expect_monitor_packet("aarch64 break", &monitor_packet, POLY_TRAP_BREAK,
      POLY_MODE_RAW_AARCH64, 5, 5, 11, 17, 18) != 0)
    return 1;
  pid_t break_child = fork();
  if (break_child < 0) {
    fputs("NATIVE_CHECK_FAIL: poly break packet fork failed\n", stderr);
    return 1;
  }
  if (break_child == 0) {
    if (poly_trap_status_reason() != 0)
      _exit(31);
    if (poly_break_status_number() != 0)
      _exit(32);
    if (poly_break_status_mode() != POLY_MODE_X86)
      _exit(33);
    _exit(0);
  }
  status = 0;
  if (waitpid(break_child, &status, 0) != break_child || !WIFEXITED(status) ||
      WEXITSTATUS(status) != 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly break packet leaked across address space status=0x%x\n",
      status);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_BREAK) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent break packet lost after fork got=%llu\n",
      (unsigned long long) poly_trap_status_reason());
    return 1;
  }
  if (poly_break_status_number() != 5 ||
      poly_break_status_mode() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent break status lost after fork number=%llu mode=%llu\n",
      (unsigned long long) poly_break_status_number(),
      (unsigned long long) poly_break_status_mode());
    return 1;
  }

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01500513\n" // addi a0,zero,21
    ".long 0x01600593\n" // addi a1,zero,22
    ".long 0x01700613\n" // addi a2,zero,23
    ".long 0x01800693\n" // addi a3,zero,24
    ".long 0x01900713\n" // addi a4,zero,25
    ".long 0x01a00793\n" // addi a5,zero,26
    ".long 0x01b00813\n" // addi a6,zero,27
    ".long 0x00500893\n" // addi x17,x0,5
    ".long 0x00100073\n" // ebreak
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 4545) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv ebreak trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_break_status_number() != 5 ||
      poly_break_status_mode() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv break status mismatch number=%llu mode=%llu\n",
      (unsigned long long) poly_break_status_number(),
      (unsigned long long) poly_break_status_mode());
    return 1;
  }
  if (poly_trap_status_arg6() != 27 || poly_trap_status_arg7() != 5) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv break packet extended args mismatch arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (expect_monitor_packet("riscv break", &monitor_packet, POLY_TRAP_BREAK,
      POLY_MODE_RAW_RISCV, 5, 0, 21, 27, 5) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01500513\n" // addi a0,zero,21
    ".long 0x01600593\n" // addi a1,zero,22
    ".long 0x01700613\n" // addi a2,zero,23
    ".long 0x01800693\n" // addi a3,zero,24
    ".long 0x01900713\n" // addi a4,zero,25
    ".long 0x01a00793\n" // addi a5,zero,26
    ".long 0x01b00813\n" // addi a6,zero,27
    ".long 0x00500893\n" // addi x17,x0,5
    ".short 0x9002\n" // c.ebreak
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 4545) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed ebreak trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_BREAK) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed break packet reason mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_reason());
    return 1;
  }
  if (poly_break_status_number() != 5 ||
      poly_break_status_mode() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed break status mismatch number=%llu mode=%llu\n",
      (unsigned long long) poly_break_status_number(),
      (unsigned long long) poly_break_status_mode());
    return 1;
  }
  if (poly_trap_status_arg6() != 27 || poly_trap_status_arg7() != 5) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed break packet extended args mismatch arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (expect_monitor_packet("riscv compressed break", &monitor_packet,
      POLY_TRAP_BREAK, POLY_MODE_RAW_RISCV, 5, 0, 21, 27, 5) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_A64
    ".long 0xd29c1010\n" // movz x16,#0xe080
    ".long 0xf2bffff0\n" // movk x16,#0xffff,lsl #16
    ".long 0xf2dffff0\n" // movk x16,#0xffff,lsl #32
    ".long 0xf2fffff0\n" // movk x16,#0xffff,lsl #48
    ".long 0xd28009a0\n" // movz x0,#77
    ".long 0xd2800b06\n" // movz x6,#88
    ".long 0xd2800c67\n" // movz x7,#99
    ".long 0xd63f0200\n" // blr x16, unresolved strlen descriptor
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  result = read_rax();
  if (result != 5555) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 import trap result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_IMPORT) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly import packet reason mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_reason());
    return 1;
  }
  if (poly_trap_status_mode() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 import packet mode mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_mode());
    return 1;
  }
  if (poly_trap_status_arg6() != 88 || poly_trap_status_arg7() != 99) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 import packet extended args mismatch arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (expect_monitor_packet("aarch64 import", &monitor_packet,
      POLY_TRAP_IMPORT, POLY_MODE_RAW_AARCH64, 8, 0, 77, 88, 99) != 0)
    return 1;

  nativecheck_install_descriptor_poison();
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    "movq %[descriptor], %%r12\n"
    POLY_OP_ENTER_A64
    ".long 0xd29c1010\n" // movz x16,#0xe080
    ".long 0xf2bffff0\n" // movk x16,#0xffff,lsl #16
    ".long 0xf2dffff0\n" // movk x16,#0xffff,lsl #32
    ".long 0xf2fffff0\n" // movk x16,#0xffff,lsl #48
    ".long 0xd28009a0\n" // movz x0,#77
    ".long 0xd2800b06\n" // movz x6,#88
    ".long 0xd2800c67\n" // movz x7,#99
    ".long 0xd63f0200\n" // blr x16, import must trap despite descriptor
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    :
    : [descriptor] "r"(nativecheck_import_descriptor_table)
    : "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  result = read_rax();
  if (result != 5555) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 descriptor-backed import trap result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (nativecheck_descriptor_target_calls != 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 descriptor target executed calls=%u\n",
      nativecheck_descriptor_target_calls);
    return 1;
  }
  if (expect_monitor_packet("aarch64 descriptor-backed import", &monitor_packet,
      POLY_TRAP_IMPORT, POLY_MODE_RAW_AARCH64, 8, 0, 77, 88, 99) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> unresolved strlen descriptor
    ".long 0x04d00513\n" // addi a0,zero,77
    ".long 0x05800813\n" // addi a6,zero,88
    ".long 0x06300893\n" // addi a7,zero,99
    ".long 0x000280e7\n" // jalr ra,0(t0)
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  result = read_rax();
  if (result != 5555) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv import trap result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_IMPORT) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv import packet reason mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_reason());
    return 1;
  }
  if (poly_trap_status_mode() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv import packet mode mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_mode());
    return 1;
  }
  if (poly_trap_status_arg6() != 88 || poly_trap_status_arg7() != 99) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv import packet extended args mismatch arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (expect_monitor_packet("riscv import", &monitor_packet,
      POLY_TRAP_IMPORT, POLY_MODE_RAW_RISCV, 8, 0, 77, 88, 99) != 0)
    return 1;

  nativecheck_install_descriptor_poison();
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    "movq %[descriptor], %%r12\n"
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> strlen import
    ".long 0x04d00513\n" // addi a0,zero,77
    ".long 0x05800813\n" // addi a6,zero,88
    ".long 0x06300893\n" // addi a7,zero,99
    ".long 0x000280e7\n" // jalr ra,0(t0), must trap despite descriptor
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    :
    : [descriptor] "r"(nativecheck_import_descriptor_table)
    : "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  result = read_rax();
  if (result != 5555) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv descriptor-backed import trap result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (nativecheck_descriptor_target_calls != 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv descriptor target executed calls=%u\n",
      nativecheck_descriptor_target_calls);
    return 1;
  }
  if (expect_monitor_packet("riscv descriptor-backed import", &monitor_packet,
      POLY_TRAP_IMPORT, POLY_MODE_RAW_RISCV, 8, 0, 77, 88, 99) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> unresolved strlen descriptor
    ".long 0x04d00513\n" // addi a0,zero,77
    ".long 0x05800813\n" // addi a6,zero,88
    ".long 0x06300893\n" // addi a7,zero,99
    ".short 0x9282\n" // c.jalr t0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  result = read_rax();
  if (result != 5555) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed import trap result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_IMPORT) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed import packet reason mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_reason());
    return 1;
  }
  if (poly_trap_status_mode() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed import packet mode mismatch got=%llu\n",
      (unsigned long long) poly_trap_status_mode());
    return 1;
  }
  if (poly_trap_status_arg6() != 88 || poly_trap_status_arg7() != 99) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed import packet extended args mismatch arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (expect_monitor_packet("riscv compressed import", &monitor_packet,
      POLY_TRAP_IMPORT, POLY_MODE_RAW_RISCV, 8, 0, 77, 88, 99) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xffffffff\n" // unallocated in the supported AArch64 subset
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 4664) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 illegal trap result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_ILLEGAL ||
      poly_trap_status_mode() != POLY_MODE_RAW_AARCH64 ||
      poly_trap_status_number() != 0xffffffffULL ||
      poly_trap_status_selector() != 4) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 illegal trap packet mismatch reason=%llu mode=%llu number=0x%llx selector=%llu\n",
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
      (unsigned long long) poly_trap_status_selector());
    return 1;
  }
  if (expect_monitor_packet("aarch64 illegal", &monitor_packet,
      POLY_TRAP_ILLEGAL, POLY_MODE_RAW_AARCH64, 0xffffffffULL, 4, 0, 0,
      0) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0xffffffff\n" // unallocated in the supported RISC-V subset
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 4665) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv illegal trap result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_ILLEGAL ||
      poly_trap_status_mode() != POLY_MODE_RAW_RISCV ||
      poly_trap_status_number() != 0xffffffffULL ||
      poly_trap_status_selector() != 4) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv illegal trap packet mismatch reason=%llu mode=%llu number=0x%llx selector=%llu\n",
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
      (unsigned long long) poly_trap_status_selector());
    return 1;
  }
  if (expect_monitor_packet("riscv illegal", &monitor_packet,
      POLY_TRAP_ILLEGAL, POLY_MODE_RAW_RISCV, 0xffffffffULL, 4, 0, 0, 0) !=
      0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  asm volatile(
    POLY_OP_ENTER_RV64
    ".short 0x0000\n" // reserved 16-bit compressed encoding
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 4666) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed illegal trap result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_ILLEGAL ||
      poly_trap_status_mode() != POLY_MODE_RAW_RISCV ||
      poly_trap_status_number() != 0 ||
      poly_trap_status_selector() != 2) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv compressed illegal trap packet mismatch reason=%llu mode=%llu number=0x%llx selector=%llu\n",
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_number(),
      (unsigned long long) poly_trap_status_selector());
    return 1;
  }
  if (expect_monitor_packet("riscv compressed illegal", &monitor_packet,
      POLY_TRAP_ILLEGAL, POLY_MODE_RAW_RISCV, 0, 2, 0, 0, 0) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_AARCH64);
  poly_trap_vector_set_value((uint64_t) (void *) poly_aarch64_trap_vector_raw);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x00700813\n" // addi a6,zero,7
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n" // ecall
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 6) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv-to-aarch64 trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (expect_monitor_packet("riscv-to-aarch64 syscall vector",
      &monitor_packet, POLY_TRAP_SYSCALL, POLY_MODE_RAW_RISCV, 172, 0, 1,
      7, 172) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_RISCV);
  poly_trap_vector_set_value((uint64_t) (void *) poly_riscv_trap_vector_raw);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd28000e6\n" // movz x6,#7
    ".long 0xd2800107\n" // movz x7,#8
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 6) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64-to-riscv trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (expect_monitor_packet("aarch64-to-riscv syscall vector",
      &monitor_packet, POLY_TRAP_SYSCALL, POLY_MODE_RAW_AARCH64, 172, 7, 1,
      7, 8) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_AARCH64);
  poly_trap_vector_set_value((uint64_t) (void *) poly_aarch64_trap_vector_ext_raw);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01500513\n" // addi a0,zero,21
    ".long 0x01600593\n" // addi a1,zero,22
    ".long 0x01700613\n" // addi a2,zero,23
    ".long 0x01800693\n" // addi a3,zero,24
    ".long 0x01900713\n" // addi a4,zero,25
    ".long 0x01a00793\n" // addi a5,zero,26
    ".long 0x01b00813\n" // addi a6,zero,27
    ".long 0x00500893\n" // addi a7,zero,5
    ".long 0x00100073\n" // ebreak
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 32) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv-to-aarch64 extended trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (expect_monitor_packet("riscv-to-aarch64 break vector",
      &monitor_packet, POLY_TRAP_BREAK, POLY_MODE_RAW_RISCV, 5, 0, 21, 27,
      5) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_RISCV);
  poly_trap_vector_set_value((uint64_t) (void *) poly_riscv_trap_vector_ext_raw);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800160\n" // movz x0,#11
    ".long 0xd2800181\n" // movz x1,#12
    ".long 0xd28001a2\n" // movz x2,#13
    ".long 0xd28001c3\n" // movz x3,#14
    ".long 0xd28001e4\n" // movz x4,#15
    ".long 0xd2800205\n" // movz x5,#16
    ".long 0xd2800226\n" // movz x6,#17
    ".long 0xd2800247\n" // movz x7,#18
    ".long 0xd42000a0\n" // brk #5
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 35) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64-to-riscv extended trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (expect_monitor_packet("aarch64-to-riscv break vector",
      &monitor_packet, POLY_TRAP_BREAK, POLY_MODE_RAW_AARCH64, 5, 5, 11, 17,
      18) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_AARCH64);
  poly_trap_vector_set_value((uint64_t) (void *) poly_aarch64_trap_vector_ext_raw);
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_RV64
    ".long 0xffffe2b7\n" // lui t0,0xffffe -> 0xffffffffffffe000
    ".long 0x08028293\n" // addi t0,t0,0x80 -> unresolved strlen descriptor
    ".long 0x01500513\n" // addi a0,zero,21
    ".long 0x01600593\n" // addi a1,zero,22
    ".long 0x01700613\n" // addi a2,zero,23
    ".long 0x01800693\n" // addi a3,zero,24
    ".long 0x01900713\n" // addi a4,zero,25
    ".long 0x01a00793\n" // addi a5,zero,26
    ".long 0x01b00813\n" // addi a6,zero,27
    ".long 0x00500893\n" // addi a7,zero,5
    ".long 0x000280e7\n" // jalr ra,0(t0)
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  result = read_rax();
  if (result != 32) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv-to-aarch64 import trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_IMPORT ||
      poly_trap_status_mode() != POLY_MODE_RAW_RISCV ||
      poly_trap_status_arg6() != 27 || poly_trap_status_arg7() != 5) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv-to-aarch64 import trap packet mismatch reason=%llu mode=%llu arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (expect_monitor_packet("riscv-to-aarch64 import vector",
      &monitor_packet, POLY_TRAP_IMPORT, POLY_MODE_RAW_RISCV, 8, 0, 21, 27,
      5) != 0)
    return 1;

  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_RISCV);
  poly_trap_vector_set_value((uint64_t) (void *) poly_riscv_trap_vector_ext_raw);
  asm volatile(
    "xorq %%r12,%%r12\n"
    POLY_OP_ENTER_A64
    ".long 0xd29c1010\n" // movz x16,#0xe080
    ".long 0xf2bffff0\n" // movk x16,#0xffff,lsl #16
    ".long 0xf2dffff0\n" // movk x16,#0xffff,lsl #32
    ".long 0xf2fffff0\n" // movk x16,#0xffff,lsl #48
    ".long 0xd2800160\n" // movz x0,#11
    ".long 0xd2800181\n" // movz x1,#12
    ".long 0xd28001a2\n" // movz x2,#13
    ".long 0xd28001c3\n" // movz x3,#14
    ".long 0xd28001e4\n" // movz x4,#15
    ".long 0xd2800205\n" // movz x5,#16
    ".long 0xd2800226\n" // movz x6,#17
    ".long 0xd2800247\n" // movz x7,#18
    ".long 0xd63f0200\n" // blr x16, unresolved strlen descriptor
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory");
  result = read_rax();
  if (result != 35) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64-to-riscv import trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }
  if (poly_trap_status_reason() != POLY_TRAP_IMPORT ||
      poly_trap_status_mode() != POLY_MODE_RAW_AARCH64 ||
      poly_trap_status_arg6() != 17 || poly_trap_status_arg7() != 18) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64-to-riscv import trap packet mismatch reason=%llu mode=%llu arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_trap_status_mode(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }
  if (expect_monitor_packet("aarch64-to-riscv import vector",
      &monitor_packet, POLY_TRAP_IMPORT, POLY_MODE_RAW_AARCH64, 8, 0, 11,
      17, 18) != 0)
    return 1;

  poly_trap_vector_mode_set_value(POLY_MODE_RAW_AARCH64);
  poly_trap_vector_set_value((uint64_t) (void *) poly_aarch64_trap_vector_raw);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd28009ab\n" // movz x11,#77
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0x8b0b0000\n" // add x0,x0,x11
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 83) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 trap return preserved synthetic register mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  poly_trap_vector_mode_set_value(POLY_MODE_RAW_RISCV);
  poly_trap_vector_set_value((uint64_t) (void *) poly_riscv_trap_vector_raw);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x04d00913\n" // addi s2,zero,77
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n" // ecall
    ".long 0x01250533\n" // add a0,a0,s2
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  result = read_rax();
  if (result != 83) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv trap return preserved synthetic register mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  puts("NATIVE_POLY_TRAP_VECTOR_OK");
  return 0;
}

static int run_poly_state_key_probe(void) {
  if (expect_child_signal("poly reserved state-key set", SIGILL,
        child_expect_state_key_set_signal) != 0)
    return 1;
  if (expect_child_signal("poly reserved state-key get", SIGILL,
        child_expect_state_key_get_signal) != 0)
    return 1;

  puts("NATIVE_POLY_STATE_KEY_REJECT_OK");
  return 0;
}

static int run_poly_invalid_import_no_mutation_probe(void) {
  struct poly_xsave_state before __attribute__((aligned(64)));
  struct poly_xsave_state bad __attribute__((aligned(64)));
  struct sigaction action;
  struct sigaction old_action;
  const uint64_t trap_vector = (uint64_t) poly_trap_vector_handler;

  memset(&before, 0, sizeof(before));
  memset(&bad, 0, sizeof(bad));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_RISCV);
  poly_trap_vector_set_value(trap_vector);
  poly_state_export(&before);

  memcpy(&bad, &before, sizeof(bad));
  bad.header.trap_vector_pc = 0;
  bad.header.trap_vector_mode = POLY_MODE_X86;
  bad.import_return.top = 1;
  bad.import_return.depth = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH;
  bad.import_return.frames[0].source_mode = POLY_MODE_X86;
  bad.import_return.frames[0].import_id = 8;

  memset(&action, 0, sizeof(action));
  action.sa_handler = nativecheck_sigill_handler;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGILL, &action, &old_action) != 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly invalid import sigaction failed\n");
    return 1;
  }

  nativecheck_expect_sigill = 1;
  if (sigsetjmp(nativecheck_sigill_env, 1) == 0) {
    poly_state_import(&bad);
    nativecheck_expect_sigill = 0;
    sigaction(SIGILL, &old_action, 0);
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly invalid import returned without SIGILL\n");
    return 1;
  }
  nativecheck_expect_sigill = 0;
  if (sigaction(SIGILL, &old_action, 0) != 0) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly invalid import sigaction restore failed\n");
    return 1;
  }

  poly_trap_vector_get();
  if (read_rax() != before.header.trap_vector_pc) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly invalid import mutated trap vector got=0x%llx expected=0x%llx\n",
      (unsigned long long) read_rax(),
      (unsigned long long) before.header.trap_vector_pc);
    return 1;
  }
  poly_trap_vector_mode_get();
  if (read_rax() != before.header.trap_vector_mode) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly invalid import mutated trap mode got=%llu expected=%u\n",
      (unsigned long long) read_rax(),
      before.header.trap_vector_mode);
    return 1;
  }

  poly_trap_vector_clear();
  return 0;
}

static int run_poly_cross_return_xsave_roundtrip_probe(void) {
  struct poly_xsave_state clean __attribute__((aligned(64)));
  struct poly_xsave_state cross __attribute__((aligned(64)));
  struct poly_xsave_state roundtrip __attribute__((aligned(64)));

  memset(&clean, 0, sizeof(clean));
  memset(&cross, 0, sizeof(cross));
  memset(&roundtrip, 0, sizeof(roundtrip));
  poly_state_export(&clean);

  memcpy(&cross, &clean, sizeof(cross));
  cross.cross_return.top = 2;
  cross.cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
  cross.cross_return.frames[0].return_pc = 0x1111222233334444ULL;
  cross.cross_return.frames[0].return_sp = 0x2222333344445555ULL;
  cross.cross_return.frames[0].caller_mode = POLY_MODE_RAW_AARCH64;
  cross.cross_return.frames[0].target_mode = POLY_MODE_RAW_RISCV;
  cross.cross_return.frames[0].abi_kind = POLY_CROSS_BRIDGE_DEFAULT;
  cross.cross_return.frames[0].flags = 0x12;
  cross.cross_return.frames[1].return_pc = 0x3333444455556666ULL;
  cross.cross_return.frames[1].return_sp = 0x4444555566667777ULL;
  cross.cross_return.frames[1].caller_mode = POLY_MODE_RAW_RISCV;
  cross.cross_return.frames[1].target_mode = POLY_MODE_RAW_AARCH64;
  cross.cross_return.frames[1].abi_kind = POLY_CROSS_BRIDGE_VEC128_U32;
  cross.cross_return.frames[1].flags = 0x34;

  poly_state_import(&cross);
  poly_state_export(&roundtrip);
  if (roundtrip.cross_return.top != 2 ||
      roundtrip.cross_return.depth != POLY_STATE_XSAVE_CROSS_RETURN_DEPTH ||
      memcmp(roundtrip.cross_return.frames, cross.cross_return.frames,
        2 * sizeof(cross.cross_return.frames[0])) != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly cross-return xsave roundtrip mismatch top=%llu depth=%llu\n",
      (unsigned long long) roundtrip.cross_return.top,
      (unsigned long long) roundtrip.cross_return.depth);
    poly_state_import(&clean);
    return 1;
  }

  poly_state_import(&clean);
  poly_state_export(&roundtrip);
  if (roundtrip.cross_return.top != 0 ||
      roundtrip.cross_return.depth != POLY_STATE_XSAVE_CROSS_RETURN_DEPTH) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly cross-return xsave clean restore mismatch top=%llu depth=%llu\n",
      (unsigned long long) roundtrip.cross_return.top,
      (unsigned long long) roundtrip.cross_return.depth);
    return 1;
  }

  puts("NATIVE_POLY_CROSS_RETURN_XSAVE_OK");
  return 0;
}

static int run_poly_frontend_tls_probe(void) {
  struct poly_xsave_state snapshot __attribute__((aligned(64)));
  const uint64_t aarch64_tls = 0x111ULL;
  const uint64_t riscv_tls = 0x222ULL;
  uint64_t result = nativecheck_aarch64_read_tls(aarch64_tls);
  if (result != aarch64_tls) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 TLS read got=0x%llx expected=0x%llx\n",
      (unsigned long long) result, (unsigned long long) aarch64_tls);
    return 1;
  }

  result = nativecheck_riscv_read_tls(riscv_tls);
  if (result != riscv_tls) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv TLS read got=0x%llx expected=0x%llx\n",
      (unsigned long long) result, (unsigned long long) riscv_tls);
    return 1;
  }

  memset(&snapshot, 0, sizeof(snapshot));
  poly_state_export(&snapshot);
  if (snapshot.frontend_tls.flags != 1 ||
      snapshot.frontend_tls.aarch64_tls_base != aarch64_tls ||
      snapshot.frontend_tls.riscv_tls_base != riscv_tls) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly frontend TLS export mismatch flags=0x%llx a64=0x%llx rv=0x%llx\n",
      (unsigned long long) snapshot.frontend_tls.flags,
      (unsigned long long) snapshot.frontend_tls.aarch64_tls_base,
      (unsigned long long) snapshot.frontend_tls.riscv_tls_base);
    return 1;
  }

  if (nativecheck_aarch64_read_tls(0x333ULL) != 0x333ULL ||
      nativecheck_riscv_read_tls(0x444ULL) != 0x444ULL) {
    fputs("NATIVE_CHECK_FAIL: poly frontend TLS mutation setup failed\n",
      stderr);
    return 1;
  }

  poly_state_import(&snapshot);
  result = nativecheck_riscv_switch_aarch64_read_tls(0x555ULL);
  if (result != aarch64_tls) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv-to-aarch64 TLS isolation got=0x%llx expected=0x%llx\n",
      (unsigned long long) result, (unsigned long long) aarch64_tls);
    return 1;
  }

  poly_state_import(&snapshot);
  result = nativecheck_aarch64_switch_riscv_read_tls(0x666ULL);
  if (result != riscv_tls) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64-to-riscv TLS isolation got=0x%llx expected=0x%llx\n",
      (unsigned long long) result, (unsigned long long) riscv_tls);
    return 1;
  }

  return 0;
}

static int run_poly_state_save_restore_probe(void) {
  struct poly_xsave_state snapshot __attribute__((aligned(64)));
  struct poly_xsave_state trap_snapshot __attribute__((aligned(64)));
  struct nativecheck_monitor_packet monitor_packet __attribute__((aligned(64)));
  const uint64_t trap_vector = (uint64_t) poly_trap_vector_handler;

  if (expect_child_signal("poly malformed import-return xstate", SIGILL,
        child_expect_malformed_import_return_xsave_signal) != 0)
    return 1;
  if (expect_child_signal("poly bad import-return mode xstate", SIGILL,
        child_expect_bad_import_return_mode_xsave_signal) != 0)
    return 1;
  if (expect_child_signal("poly bad import-return id xstate", SIGILL,
        child_expect_bad_import_return_id_xsave_signal) != 0)
    return 1;
  if (expect_child_signal("poly malformed cross-return xstate", SIGILL,
        child_expect_malformed_cross_return_xsave_signal) != 0)
    return 1;
  if (expect_child_signal("poly bad cross-return mode xstate", SIGILL,
        child_expect_bad_cross_return_mode_xsave_signal) != 0)
    return 1;
  if (expect_child_signal("poly bad cross-return bridge xstate", SIGILL,
        child_expect_bad_cross_return_bridge_xsave_signal) != 0)
    return 1;
  if (run_poly_invalid_import_no_mutation_probe() != 0)
    return 1;
  if (run_poly_cross_return_xsave_roundtrip_probe() != 0)
    return 1;
  if (run_poly_frontend_tls_probe() != 0)
    return 1;

  memset(&snapshot, 0, sizeof(snapshot));
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_RISCV);
  poly_trap_vector_set_value(trap_vector);
  poly_monitor_packet_set_value((uint64_t) (uintptr_t) &monitor_packet);
  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly state export signature set failed\n",
      stderr);
    return 1;
  }
  if (poly_landing_policy_set(POLY_LANDING_POLICY_REQUIRE_CALL) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly state export landing policy set failed\n",
      stderr);
    return 1;
  }
  poly_state_export(&snapshot);

  if (snapshot.header.magic != POLY_STATE_XSAVE_MAGIC ||
      snapshot.header.layout_version != POLY_STATE_XSAVE_LAYOUT_VERSION ||
      snapshot.header.header_bytes != POLY_STATE_XSAVE_HEADER_BYTES ||
      snapshot.header.total_bytes != sizeof(snapshot)) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state export header mismatch magic=0x%x version=%u bytes=%u\n",
      snapshot.header.magic, snapshot.header.layout_version,
      snapshot.header.total_bytes);
    return 1;
  }
  if (snapshot.header.trap_vector_pc != trap_vector ||
      snapshot.header.trap_vector_mode != POLY_MODE_RAW_RISCV ||
      snapshot.header.monitor_packet_addr !=
        (uint64_t) (uintptr_t) &monitor_packet) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state export trap vector mismatch pc=0x%llx mode=%u packet=0x%llx\n",
      (unsigned long long) snapshot.header.trap_vector_pc,
      snapshot.header.trap_vector_mode,
      (unsigned long long) snapshot.header.monitor_packet_addr);
    return 1;
  }
  if (snapshot.abi_signature.slot_count != POLY_ABI_SIGNATURE_SLOT_COUNT ||
      snapshot.abi_signature.slots[3].kind !=
        POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state export ABI signature mismatch count=%llu slot3=%u\n",
      (unsigned long long) snapshot.abi_signature.slot_count,
      snapshot.abi_signature.slots[3].kind);
    return 1;
  }
  if (snapshot.landing_policy.flags != POLY_LANDING_POLICY_REQUIRE_CALL ||
      snapshot.landing_policy.supported_flags != POLY_LANDING_POLICY_SUPPORTED) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state export landing policy mismatch flags=0x%llx supported=0x%llx\n",
      (unsigned long long) snapshot.landing_policy.flags,
      (unsigned long long) snapshot.landing_policy.supported_flags);
    return 1;
  }

  poly_landing_policy_set(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value(trap_vector);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800366\n" // movz x6,#27
    ".long 0xd28000a7\n" // movz x7,#5
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  memset(&trap_snapshot, 0, sizeof(trap_snapshot));
  poly_state_export(&trap_snapshot);
  if (trap_snapshot.trap.reason != POLY_TRAP_SYSCALL ||
      trap_snapshot.trap.source_mode != POLY_MODE_RAW_AARCH64 ||
      trap_snapshot.trap.number != 172 ||
      trap_snapshot.trap.selector != 7 ||
      trap_snapshot.trap_args[6] != 27 ||
      trap_snapshot.trap_args[7] != 5) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state export aarch64 syscall trap mismatch reason=%u mode=%u number=%llu selector=%llu arg6=%llu arg7=%llu\n",
      trap_snapshot.trap.reason,
      trap_snapshot.trap.source_mode,
      (unsigned long long) trap_snapshot.trap.number,
      (unsigned long long) trap_snapshot.trap.selector,
      (unsigned long long) trap_snapshot.trap_args[6],
      (unsigned long long) trap_snapshot.trap_args[7]);
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x01b00813\n" // addi a6,zero,27
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n" // ecall
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  memset(&trap_snapshot, 0, sizeof(trap_snapshot));
  poly_state_export(&trap_snapshot);
  if (trap_snapshot.trap.reason != POLY_TRAP_SYSCALL ||
      trap_snapshot.trap.source_mode != POLY_MODE_RAW_RISCV ||
      trap_snapshot.trap.number != 172 ||
      trap_snapshot.trap.selector != 0 ||
      trap_snapshot.trap_args[6] != 27 ||
      trap_snapshot.trap_args[7] != 172) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state export riscv syscall trap mismatch reason=%u mode=%u number=%llu selector=%llu arg6=%llu arg7=%llu\n",
      trap_snapshot.trap.reason,
      trap_snapshot.trap.source_mode,
      (unsigned long long) trap_snapshot.trap.number,
      (unsigned long long) trap_snapshot.trap.selector,
      (unsigned long long) trap_snapshot.trap_args[6],
      (unsigned long long) trap_snapshot.trap_args[7]);
    return 1;
  }

  poly_trap_vector_clear();
  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly state import signature mutate failed\n",
      stderr);
    return 1;
  }
  if (poly_landing_policy_set(0) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly state import landing policy mutate failed\n",
      stderr);
    return 1;
  }
  poly_state_import(&snapshot);

  poly_trap_vector_get();
  if (read_rax() != trap_vector) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state import trap vector mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  poly_trap_vector_mode_get();
  if (read_rax() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state import trap mode mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (poly_abi_signature_get(3) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state import ABI signature mismatch got=%llu\n",
      (unsigned long long) poly_abi_signature_get(3));
    return 1;
  }
  if (poly_landing_policy_get() != POLY_LANDING_POLICY_REQUIRE_CALL) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state import landing policy mismatch got=0x%llx\n",
      (unsigned long long) poly_landing_policy_get());
    return 1;
  }
  poly_landing_policy_set(0);
  poly_monitor_packet_get();
  if (read_rax() != (uint64_t) (uintptr_t) &monitor_packet) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state import monitor packet mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }

  poly_state_import(&trap_snapshot);
  if (poly_trap_status_reason() != POLY_TRAP_SYSCALL ||
      poly_syscall_status_number() != 172 ||
      poly_syscall_status_mode() != POLY_MODE_RAW_RISCV ||
      poly_trap_status_selector() != 0 ||
      poly_trap_status_arg6() != 27 ||
      poly_trap_status_arg7() != 172) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state import syscall trap mismatch reason=%llu number=%llu mode=%llu selector=%llu arg6=%llu arg7=%llu\n",
      (unsigned long long) poly_trap_status_reason(),
      (unsigned long long) poly_syscall_status_number(),
      (unsigned long long) poly_syscall_status_mode(),
      (unsigned long long) poly_trap_status_selector(),
      (unsigned long long) poly_trap_status_arg6(),
      (unsigned long long) poly_trap_status_arg7());
    return 1;
  }

  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly state signature restore failed\n",
      stderr);
    return 1;
  }
  poly_trap_vector_clear();
  puts("NATIVE_POLY_STATE_SAVE_RESTORE_OK");
  return 0;
}

static int run_poly_real_xsave_probe(uint64_t xcr0) {
  const uint64_t poly_mask = 1ULL << POLY_STATE_XSAVE_COMPONENT_ARCH;
  const uint64_t trap_vector = (uint64_t) poly_trap_vector_handler;
  const uint64_t three_bits = 0x4008000000000000ULL;
  const uint64_t five_bits = 0x4014000000000000ULL;
  const uint64_t seven_bits = 0x401c000000000000ULL;
  const uint64_t ten_bits = 0x4024000000000000ULL;
  const uint64_t twelve_bits = 0x4028000000000000ULL;
  struct nativecheck_monitor_packet monitor_packet __attribute__((aligned(64)));
  struct poly_xsave_state *saved =
    (struct poly_xsave_state *) (void *)
    (nativecheck_real_xsave_area + POLY_STATE_XSAVE_OFFSET_ARCH);

  if ((xcr0 & poly_mask) == 0) {
    puts("NATIVE_POLY_REAL_XSAVE_SKIPPED");
    return 0;
  }

  memset(nativecheck_real_xsave_area, 0,
    sizeof(nativecheck_real_xsave_area));
  memset(&monitor_packet, 0, sizeof(monitor_packet));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_RISCV);
  poly_trap_vector_set_value(trap_vector);
  poly_monitor_packet_set_value((uint64_t) (uintptr_t) &monitor_packet);
  if (poly_abi_signature_set(4, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: real XSAVE signature set failed\n", stderr);
    return 1;
  }
  write_xmm0_u64(three_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800c75\n" // movz x21,#99
    ".long 0x1e604015\n" // fmov d21,d0
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");
  write_xmm0_u64(five_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x06f00a93\n" // addi s5,zero,111
    ".long 0x22a50ad3\n" // fsgnj.d f21,fa0,fa0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");
  native_xsave64(nativecheck_real_xsave_area, poly_mask);

  if (saved->header.magic != POLY_STATE_XSAVE_MAGIC ||
      saved->header.layout_version != POLY_STATE_XSAVE_LAYOUT_VERSION ||
      saved->header.header_bytes != POLY_STATE_XSAVE_HEADER_BYTES ||
      saved->header.total_bytes != POLY_STATE_XSAVE_BYTES_ARCH ||
      saved->header.trap_vector_pc != trap_vector ||
      saved->header.trap_vector_mode != POLY_MODE_RAW_RISCV ||
      saved->header.monitor_packet_addr !=
        (uint64_t) (uintptr_t) &monitor_packet) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XSAVE poly state mismatch magic=0x%x version=%u bytes=%u pc=0x%llx mode=%u packet=0x%llx\n",
      saved->header.magic,
      saved->header.layout_version,
      saved->header.total_bytes,
      (unsigned long long) saved->header.trap_vector_pc,
      saved->header.trap_vector_mode,
      (unsigned long long) saved->header.monitor_packet_addr);
    return 1;
  }
  if (saved->abi_signature.slot_count != POLY_ABI_SIGNATURE_SLOT_COUNT ||
      saved->abi_signature.slots[4].kind !=
        POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XSAVE ABI signature mismatch count=%llu slot4=%u\n",
      (unsigned long long) saved->abi_signature.slot_count,
      saved->abi_signature.slots[4].kind);
    return 1;
  }
  if (saved->aarch64_gpr[21] != 99 ||
      saved->aarch64_fp[21].lo != three_bits ||
      saved->riscv_gpr[21] != 111 ||
      saved->riscv_fp[21].lo != five_bits) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XSAVE register bank mismatch a64x21=%llu a64v21=0x%llx rvx21=%llu rvf21=0x%llx\n",
      (unsigned long long) saved->aarch64_gpr[21],
      (unsigned long long) saved->aarch64_fp[21].lo,
      (unsigned long long) saved->riscv_gpr[21],
      (unsigned long long) saved->riscv_fp[21].lo);
    return 1;
  }

  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value(0);
  poly_monitor_packet_set_value(0);
  if (poly_abi_signature_set(4, POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS) != 0) {
    fputs("NATIVE_CHECK_FAIL: real XRSTOR signature mutate failed\n", stderr);
    return 1;
  }
  write_xmm0_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28001b5\n" // movz x21,#13
    ".long 0x1e604015\n" // fmov d21,d0
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");
  write_xmm0_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00d00a93\n" // addi s5,zero,13
    ".long 0x22a50ad3\n" // fsgnj.d f21,fa0,fa0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");
  native_xrstor64(nativecheck_real_xsave_area, poly_mask);

  poly_trap_vector_get();
  if (read_rax() != trap_vector) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR trap vector mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) read_rax(),
      (unsigned long long) trap_vector);
    return 1;
  }
  poly_trap_vector_mode_get();
  if (read_rax() != POLY_MODE_RAW_RISCV) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR trap vector mode mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (poly_abi_signature_get(4) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR ABI signature mismatch got=%llu\n",
      (unsigned long long) poly_abi_signature_get(4));
    return 1;
  }
  poly_monitor_packet_get();
  if (read_rax() != (uint64_t) (uintptr_t) &monitor_packet) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR monitor packet mismatch got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa1503e0\n" // mov x0,x21
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != 99) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR aarch64 x21 mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  write_xmm1_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x1e612aa0\n" // fadd d0,d21,d1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "xmm1", "memory");
  if (read_xmm0_u64() != ten_bits) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR aarch64 d21 mismatch got=0x%llx\n",
      (unsigned long long) read_xmm0_u64());
    return 1;
  }
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x000a8513\n" // addi a0,s5,0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != 111) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR riscv s5 mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  write_xmm1_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x02baf553\n" // fadd.d fa0,f21,fa1
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "xmm1", "memory");
  if (read_xmm0_u64() != twelve_bits) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR riscv f21 mismatch got=0x%llx\n",
      (unsigned long long) read_xmm0_u64());
    return 1;
  }

  memset(nativecheck_real_xsave_area, 0,
    sizeof(nativecheck_real_xsave_area));
  poly_trap_vector_mode_set_value(POLY_MODE_RAW_AARCH64);
  poly_trap_vector_set_value(trap_vector);
  poly_monitor_packet_set_value((uint64_t) (uintptr_t) &monitor_packet);
  if (poly_abi_signature_set(4, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: real XRSTOR init signature mutate failed\n",
      stderr);
    return 1;
  }
  write_xmm0_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2801155\n" // movz x21,#138
    ".long 0x1e604015\n" // fmov d21,d0
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");
  write_xmm0_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x08a00a93\n" // addi s5,zero,138
    ".long 0x22a50ad3\n" // fsgnj.d f21,fa0,fa0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");
  native_xrstor64(nativecheck_real_xsave_area, poly_mask);

  poly_trap_vector_get();
  if (read_rax() != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR init trap vector not cleared got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  poly_trap_vector_mode_get();
  if (read_rax() != POLY_MODE_X86) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR init trap mode not reset got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  poly_monitor_packet_get();
  if (read_rax() != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR init monitor packet not cleared got=0x%llx\n",
      (unsigned long long) read_rax());
    return 1;
  }
  if (poly_abi_signature_get(4) != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR init ABI signature mismatch got=%llu\n",
      (unsigned long long) poly_abi_signature_get(4));
    return 1;
  }
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa1503e0\n" // mov x0,x21
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR init aarch64 x21 not cleared got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  write_xmm1_u64(0);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x1e612aa0\n" // fadd d0,d21,d1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "xmm1", "memory");
  if (read_xmm0_u64() != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR init aarch64 d21 not cleared got=0x%llx\n",
      (unsigned long long) read_xmm0_u64());
    return 1;
  }
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x000a8513\n" // addi a0,s5,0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR init riscv s5 not cleared got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }
  write_xmm1_u64(0);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x02baf553\n" // fadd.d fa0,f21,fa1
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "xmm1", "memory");
  if (read_xmm0_u64() != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: real XRSTOR init riscv f21 not cleared got=0x%llx\n",
      (unsigned long long) read_xmm0_u64());
    return 1;
  }

  poly_trap_vector_clear();
  puts("NATIVE_POLY_REAL_XSAVE_OK");
  return 0;
}

static void request_poly_xsave_permission(uint64_t *xcr0) {
  const uint64_t poly_mask = 1ULL << POLY_STATE_XSAVE_COMPONENT_ARCH;
  unsigned long xcomp_supp = 0;
  unsigned long xcomp_perm = 0;

  if (native_arch_prctl(ARCH_GET_XCOMP_SUPP,
        (unsigned long) (uintptr_t) &xcomp_supp) == 0) {
    printf("NATIVE_POLY_XSAVE_XCOMP_SUPP=0x%llx\n",
      (unsigned long long) xcomp_supp);
  }
  else {
    printf("NATIVE_POLY_XSAVE_XCOMP_SUPP_UNAVAILABLE errno=%d\n", errno);
  }

  if (native_arch_prctl(ARCH_GET_XCOMP_PERM,
        (unsigned long) (uintptr_t) &xcomp_perm) == 0) {
    printf("NATIVE_POLY_XSAVE_XCOMP_PERM=0x%llx\n",
      (unsigned long long) xcomp_perm);
  }
  else {
    printf("NATIVE_POLY_XSAVE_XCOMP_PERM_UNAVAILABLE errno=%d\n", errno);
  }

  if ((*xcr0 & poly_mask) != 0)
    return;

  errno = 0;
  if (native_arch_prctl(ARCH_REQ_XCOMP_PERM,
        POLY_STATE_XSAVE_COMPONENT_ARCH) == 0) {
    *xcr0 = read_xcr0();
    if ((*xcr0 & poly_mask) != 0) {
      puts("NATIVE_POLY_XSAVE_ARCH_PRCTL_ENABLED");
    }
    else {
      printf("NATIVE_POLY_XSAVE_ARCH_PRCTL_NO_XCR0 xcr0=0x%llx\n",
        (unsigned long long) *xcr0);
    }
  }
  else {
    printf("NATIVE_POLY_XSAVE_ARCH_PRCTL_UNAVAILABLE errno=%d\n", errno);
  }
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_direct_pcall_aarch64_import_sum6(uint64_t a0,
    uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  register uint64_t r8_arg asm("r8") = a5;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_import_x86_sum6;
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

__attribute__((noinline, noipa))
static uint64_t nativecheck_direct_pcall_riscv_import_sum6(uint64_t a0,
    uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  register uint64_t r8_arg asm("r8") = a5;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_import_x86_sum6;
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

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_direct_pcall_aarch64_import_sum6(
    uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
    uint64_t a5) {
  register uint64_t r8_arg asm("r8") = a5;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_import_x86_sum6;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+a"(a0), "+d"(a1), "+c"(a2), "+D"(a3), "+S"(a4),
      "+r"(r8_arg), "+r"(target)
    :
    : "rbx", "r9", "r11", "r12", "r13", "r14", "memory");
  return a0;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_direct_pcall_riscv_import_sum6(
    uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
    uint64_t a5) {
  register uint64_t r8_arg asm("r8") = a5;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_import_x86_sum6;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+a"(a0), "+d"(a1), "+c"(a2), "+D"(a3), "+S"(a4),
      "+r"(r8_arg), "+r"(target)
    :
    : "rbx", "r9", "r11", "r12", "r13", "r14", "memory");
  return a0;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_generic_pcall_aarch64_x86_direct_sum6(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_sum6;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032f3f\n" // generic pcall frontend=x17 target=x16
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_generic_pcall_riscv_x86_direct_sum6(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_sum6;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x1200700b\n" // generic pcall frontend=t1 target=t0 return=t2
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_aarch64_direct_x86_source_sp_matches(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_source_sp_matches;
  asm volatile(
    "movq %%rsp, nativecheck_expected_source_sp(%%rip)\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032f3f\n" // generic pcall frontend=x17 target=x16
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_riscv_direct_x86_source_sp_matches(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_source_sp_matches;
  asm volatile(
    "movq %%rsp, nativecheck_expected_source_sp(%%rip)\n"
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x1200700b\n" // generic pcall frontend=t1 target=t0 return=t2
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_aarch64_signature_imm_direct_x86_source_sp_matches(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_source_sp_matches;
  asm volatile(
    "movq %%rsp, nativecheck_expected_source_sp(%%rip)\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_riscv_signature_imm_direct_x86_source_sp_matches(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_source_sp_matches;
  asm volatile(
    "movq %%rsp, nativecheck_expected_source_sp(%%rip)\n"
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_pcall_aarch64_x86_direct_sum6(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_sum6;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd28000e6\n" // movz x6,#7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000072\n" // adr x18,return
    ".long 0xd2800073\n" // movz x19,#3 (signature slot)
    ".long 0xd5032f5f\n" // generic signature pcall
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_pcall_riscv_x86_direct_sum6(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_sum6;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x00700813\n" // addi a6,zero,7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x01038393\n" // addi t2,t2,16 -> return
    ".long 0x00300e13\n" // addi t3,zero,3 (signature slot)
    ".long 0x1400700b\n" // generic signature pcall
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_pcall_aarch64_x86_direct_sum6(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_sum6;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd28000e6\n" // movz x6,#7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_pcall_riscv_x86_direct_sum6(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_sum6;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x00700813\n" // addi a6,zero,7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_pcall_aarch64_x86_direct_fp64(
    uint64_t left_bits, uint64_t right_bits) {
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_fp64_mul;
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    : "+r"(target)
    :
    : "rax", "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12",
      "r13", "r14", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_pcall_riscv_x86_direct_fp64(
    uint64_t left_bits, uint64_t right_bits) {
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_fp64_mul;
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    : "+r"(target)
    :
    : "rax", "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12",
      "r13", "r14", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

__attribute__((noinline, noipa))
static uint64_t
nativecheck_signature_imm_pcall_aarch64_x86_direct_fp64_raw_consume(
    uint64_t left_bits, uint64_t right_bits) {
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_fp64_mul;
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    ".long 0x1e602800\n" // return: fadd d0,d0,d0
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "+r"(target)
    :
    : "rax", "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12",
      "r13", "r14", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

__attribute__((noinline, noipa))
static uint64_t
nativecheck_signature_imm_pcall_riscv_x86_direct_fp64_raw_consume(
    uint64_t left_bits, uint64_t right_bits) {
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_fp64_mul;
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    ".long 0x02a50553\n" // return: fadd.d fa0,fa0,fa0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "+r"(target)
    :
    : "rax", "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12",
      "r13", "r14", "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

__attribute__((noinline, noipa))
static struct nativecheck_u128
nativecheck_signature_imm_pcall_aarch64_x86_direct_vec128(void) {
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_vec128_add;
  write_xmm0_xmm1_u128(1, 2, 3, 4);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032c7f\n" // generic signature pcall, immediate slot 3
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    : "+r"(target)
    :
    : "rax", "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12",
      "r13", "r14", "xmm0", "xmm1", "memory");
  return read_xmm0_u128();
}

__attribute__((noinline, noipa))
static struct nativecheck_u128
nativecheck_signature_imm_pcall_riscv_x86_direct_vec128(void) {
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_vec128_add;
  write_xmm0_xmm1_u128(1, 2, 3, 4);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x2600700b\n" // generic signature pcall, immediate slot 3
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    : "+r"(target)
    :
    : "rax", "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12",
      "r13", "r14", "xmm0", "xmm1", "memory");
  return read_xmm0_u128();
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_pcall_aarch64_riscv_sum6(void) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, riscv target from R10/P7
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
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
    : "=a"(result)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_pcall_riscv_aarch64_sum6(void) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, aarch64 target from R10/P7
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
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
    : "=a"(result)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12",
      "r13", "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_pcall_aarch64_riscv_fp64(
    uint64_t left_bits, uint64_t right_bits) {
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

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_imm_pcall_riscv_aarch64_fp64(
    uint64_t left_bits, uint64_t right_bits) {
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

__attribute__((noinline, noipa))
static struct nativecheck_u128
nativecheck_signature_imm_pcall_aarch64_riscv_vec128(void) {
  write_xmm0_xmm1_u128(1, 2, 3, 4);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, RISC-V target from R10/P7
    ".long 0xd2800051\n" // movz x17,#2 (RISC-V frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032cbf\n" // generic signature pcall, immediate slot 5
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x00c50533\n" // add a0,a0,a2
    ".long 0x00d585b3\n" // add a1,a1,a3
    ".long 0x00008067\n" // ret through hardware return cookie
    "2:\n"
    ::: "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "r12", "r13", "r14", "xmm0", "xmm1", "memory");
  return read_xmm0_u128();
}

__attribute__((noinline, noipa))
static struct nativecheck_u128
nativecheck_signature_imm_pcall_riscv_aarch64_vec128(void) {
  struct nativecheck_u128 result;
  uint64_t v0_lo = 1;
  uint64_t v0_hi = 2;
  uint64_t v1_lo = 3;
  uint64_t v1_hi = 4;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, AArch64 target from R10/P7
    ".long 0x00100313\n" // addi t1,zero,1 (AArch64 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x2a00700b\n" // generic signature pcall, immediate slot 5
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    "jmp 2f\n"
    ".p2align 2\n"
    "1:\n"
    ".long 0x4ee18400\n" // add v0.2d,v0.2d,v1.2d
    ".long 0xd65f03c0\n" // ret x30 through hardware return cookie
    "2:\n"
    : "+a"(v0_lo), "+d"(v0_hi), "+c"(v1_lo), "+D"(v1_hi)
    :
    : "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14",
      "xmm0", "xmm1", "memory");
  result.lo = v0_lo;
  result.hi = v0_hi;
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_pcall_aarch64_x86_direct_i128(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_i128;
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000072\n" // adr x18,return
    ".long 0xd2800093\n" // movz x19,#4 (native i128 signature slot)
    ".long 0xd5032f5f\n" // generic signature pcall
    ".long 0x8b010000\n" // return: add x0,x0,x1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t nativecheck_signature_pcall_riscv_x86_direct_i128(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) nativecheck_direct_x86_i128;
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x01038393\n" // addi t2,t2,16 -> return
    ".long 0x00400e13\n" // addi t3,zero,4 (native i128 signature slot)
    ".long 0x1400700b\n" // generic signature pcall
    ".long 0x00b50533\n" // return: add a0,a0,a1
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

static int check_poly_import_return_xsave_frame(uint32_t expected_mode,
    uint64_t expected_import_id) {
  const struct poly_import_return_state *state =
    &nativecheck_import_live_state.import_return;
  if (nativecheck_import_live_state.header.layout_version !=
        POLY_STATE_XSAVE_LAYOUT_VERSION ||
      state->top != 1 ||
      state->depth != POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly import xsave header mismatch mode=%u version=%u top=%llu depth=%llu\n",
      expected_mode,
      nativecheck_import_live_state.header.layout_version,
      (unsigned long long) state->top,
      (unsigned long long) state->depth);
    return 1;
  }

  const struct poly_import_return_frame *frame = &state->frames[0];
  if (frame->source_mode != expected_mode ||
      frame->alias_valid != 1 ||
      frame->return_pc == 0 ||
      frame->return_sp == 0 ||
      frame->import_id != expected_import_id ||
      frame->return_flags != 0) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly import xsave frame mismatch expected_mode=%u mode=%u alias=%u pc=0x%llx sp=0x%llx import=%llu flags=0x%llx\n",
      expected_mode,
      frame->source_mode,
      frame->alias_valid,
      (unsigned long long) frame->return_pc,
      (unsigned long long) frame->return_sp,
      (unsigned long long) frame->import_id,
      (unsigned long long) frame->return_flags);
    return 1;
  }

  return 0;
}

static int run_poly_import_return_xsave_probe(void) {
  const uint64_t expected = 21;
  memset(&nativecheck_import_live_state, 0,
    sizeof(nativecheck_import_live_state));
  memset(&nativecheck_import_restore_state, 0,
    sizeof(nativecheck_import_restore_state));
  nativecheck_import_helper_calls = 0;

  uint64_t result = nativecheck_direct_pcall_aarch64_import_sum6(
    1, 2, 3, 4, 5, 6);
  if (result != expected || nativecheck_import_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly direct aarch64 import xsave helper result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_import_helper_calls);
    return 1;
  }
  if (check_poly_import_return_xsave_frame(POLY_MODE_RAW_AARCH64,
        UINT64_MAX) != 0)
    return 1;

  memset(&nativecheck_import_live_state, 0,
    sizeof(nativecheck_import_live_state));
  memset(&nativecheck_import_restore_state, 0,
    sizeof(nativecheck_import_restore_state));
  nativecheck_import_helper_calls = 0;

  result = nativecheck_direct_pcall_riscv_import_sum6(1, 2, 3, 4, 5, 6);
  if (result != expected || nativecheck_import_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly direct riscv import xsave helper result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_import_helper_calls);
    return 1;
  }
  if (check_poly_import_return_xsave_frame(POLY_MODE_RAW_RISCV,
        UINT64_MAX) != 0)
    return 1;

  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly import-return signature slot sysv set failed\n",
      stderr);
    return 1;
  }

  memset(&nativecheck_import_live_state, 0,
    sizeof(nativecheck_import_live_state));
  memset(&nativecheck_import_restore_state, 0,
    sizeof(nativecheck_import_restore_state));
  nativecheck_import_helper_calls = 0;

  result = nativecheck_signature_imm_direct_pcall_aarch64_import_sum6(
    1, 2, 3, 4, 5, 6);
  if (result != expected || nativecheck_import_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly signature direct aarch64 import xsave helper result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_import_helper_calls);
    return 1;
  }
  if (check_poly_import_return_xsave_frame(POLY_MODE_RAW_AARCH64,
        UINT64_MAX) != 0)
    return 1;

  memset(&nativecheck_import_live_state, 0,
    sizeof(nativecheck_import_live_state));
  memset(&nativecheck_import_restore_state, 0,
    sizeof(nativecheck_import_restore_state));
  nativecheck_import_helper_calls = 0;

  result = nativecheck_signature_imm_direct_pcall_riscv_import_sum6(
    1, 2, 3, 4, 5, 6);
  if (result != expected || nativecheck_import_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly signature direct riscv import xsave helper result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_import_helper_calls);
    return 1;
  }
  if (check_poly_import_return_xsave_frame(POLY_MODE_RAW_RISCV,
        UINT64_MAX) != 0)
    return 1;

  puts("NATIVE_POLY_IMPORT_RETURN_XSAVE_OK");
  return 0;
}

static int run_poly_direct_x86_pcall_probe(void) {
  const uint64_t expected = 21;
  const uint64_t exchange_expected = 27;
  const uint64_t fp_left_bits = 0x3ff8000000000000ULL;
  const uint64_t fp_right_bits = 0x4002000000000000ULL;
  const uint64_t fp_expected_bits = 0x400b000000000000ULL;
  const uint64_t fp_raw_consume_expected_bits = 0x401b000000000000ULL;
  nativecheck_direct_x86_helper_calls = 0;

  uint64_t result =
    nativecheck_generic_pcall_aarch64_x86_direct_sum6();
  if (result != expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_generic_pcall_riscv_x86_direct_sum6();
  if (result != expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  result = nativecheck_aarch64_direct_x86_source_sp_matches();
  if (result != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 direct x86 source stack pointer result=%llu expected_sp=0x%llx\n",
      (unsigned long long) result,
      (unsigned long long) nativecheck_expected_source_sp);
    return 1;
  }

  result = nativecheck_riscv_direct_x86_source_sp_matches();
  if (result != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv direct x86 source stack pointer result=%llu expected_sp=0x%llx\n",
      (unsigned long long) result,
      (unsigned long long) nativecheck_expected_source_sp);
    return 1;
  }

  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly direct x86 signature slot sysv set failed\n",
      stderr);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_signature_pcall_aarch64_x86_direct_sum6();
  if (result != expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_signature_pcall_riscv_x86_direct_sum6();
  if (result != expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_aarch64_x86_direct_sum6();
  if (result != expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 immediate signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_riscv_x86_direct_sum6();
  if (result != expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv immediate signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_fp64_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_aarch64_x86_direct_fp64(
    fp_left_bits, fp_right_bits);
  if (result != fp_expected_bits ||
      nativecheck_direct_x86_fp64_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 immediate signature direct x86 fp64 result=0x%llx calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_fp64_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_fp64_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_riscv_x86_direct_fp64(
    fp_left_bits, fp_right_bits);
  if (result != fp_expected_bits ||
      nativecheck_direct_x86_fp64_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv immediate signature direct x86 fp64 result=0x%llx calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_fp64_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_vec128_helper_calls = 0;
  struct nativecheck_u128 vec_result =
    nativecheck_signature_imm_pcall_aarch64_x86_direct_vec128();
  if (vec_result.lo != 4 || vec_result.hi != 6 ||
      nativecheck_direct_x86_vec128_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 immediate signature direct x86 vec128 result=0x%llx:0x%llx calls=%u\n",
      (unsigned long long) vec_result.hi,
      (unsigned long long) vec_result.lo,
      nativecheck_direct_x86_vec128_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_vec128_helper_calls = 0;
  vec_result = nativecheck_signature_imm_pcall_riscv_x86_direct_vec128();
  if (vec_result.lo != 4 || vec_result.hi != 6 ||
      nativecheck_direct_x86_vec128_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv immediate signature direct x86 vec128 result=0x%llx:0x%llx calls=%u\n",
      (unsigned long long) vec_result.hi,
      (unsigned long long) vec_result.lo,
      nativecheck_direct_x86_vec128_helper_calls);
    return 1;
  }

  result = nativecheck_aarch64_signature_imm_direct_x86_source_sp_matches();
  if (result != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 immediate signature direct x86 source stack pointer result=%llu expected_sp=0x%llx\n",
      (unsigned long long) result,
      (unsigned long long) nativecheck_expected_source_sp);
    return 1;
  }

  result = nativecheck_riscv_signature_imm_direct_x86_source_sp_matches();
  if (result != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv immediate signature direct x86 source stack pointer result=%llu expected_sp=0x%llx\n",
      (unsigned long long) result,
      (unsigned long long) nativecheck_expected_source_sp);
    return 1;
  }

  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly direct x86 signature slot native set failed\n",
      stderr);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_aarch64_x86_direct_sum6();
  if (result != expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 native signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_riscv_x86_direct_sum6();
  if (result != expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv native signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_fp64_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_aarch64_x86_direct_fp64(
    fp_left_bits, fp_right_bits);
  if (result != fp_expected_bits ||
      nativecheck_direct_x86_fp64_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 native signature direct x86 fp64 result=0x%llx calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_fp64_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_fp64_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_riscv_x86_direct_fp64(
    fp_left_bits, fp_right_bits);
  if (result != fp_expected_bits ||
      nativecheck_direct_x86_fp64_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv native signature direct x86 fp64 result=0x%llx calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_fp64_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_fp64_helper_calls = 0;
  result =
    nativecheck_signature_imm_pcall_aarch64_x86_direct_fp64_raw_consume(
      fp_left_bits, fp_right_bits);
  if (result != fp_raw_consume_expected_bits ||
      nativecheck_direct_x86_fp64_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 native signature direct x86 fp64 raw consume result=0x%llx calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_fp64_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_fp64_helper_calls = 0;
  result = nativecheck_signature_imm_pcall_riscv_x86_direct_fp64_raw_consume(
    fp_left_bits, fp_right_bits);
  if (result != fp_raw_consume_expected_bits ||
      nativecheck_direct_x86_fp64_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv native signature direct x86 fp64 raw consume result=0x%llx calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_fp64_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_vec128_helper_calls = 0;
  vec_result = nativecheck_signature_imm_pcall_aarch64_x86_direct_vec128();
  if (vec_result.lo != 4 || vec_result.hi != 6 ||
      nativecheck_direct_x86_vec128_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 native signature direct x86 vec128 result=0x%llx:0x%llx calls=%u\n",
      (unsigned long long) vec_result.hi,
      (unsigned long long) vec_result.lo,
      nativecheck_direct_x86_vec128_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_vec128_helper_calls = 0;
  vec_result = nativecheck_signature_imm_pcall_riscv_x86_direct_vec128();
  if (vec_result.lo != 4 || vec_result.hi != 6 ||
      nativecheck_direct_x86_vec128_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv native signature direct x86 vec128 result=0x%llx:0x%llx calls=%u\n",
      (unsigned long long) vec_result.hi,
      (unsigned long long) vec_result.lo,
      nativecheck_direct_x86_vec128_helper_calls);
    return 1;
  }

  result = nativecheck_aarch64_signature_imm_direct_x86_source_sp_matches();
  if (result != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 native signature direct x86 source stack pointer result=%llu expected_sp=0x%llx\n",
      (unsigned long long) result,
      (unsigned long long) nativecheck_expected_source_sp);
    return 1;
  }

  result = nativecheck_riscv_signature_imm_direct_x86_source_sp_matches();
  if (result != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv native signature direct x86 source stack pointer result=%llu expected_sp=0x%llx\n",
      (unsigned long long) result,
      (unsigned long long) nativecheck_expected_source_sp);
    return 1;
  }

  if (poly_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_I128,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly direct x86 native i128 signature slot set failed\n",
      stderr);
    return 1;
  }

  nativecheck_direct_x86_i128_helper_calls = 0;
  result = nativecheck_signature_pcall_aarch64_x86_direct_i128();
  if (result != 51 || nativecheck_direct_x86_i128_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 native i128 signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_i128_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_i128_helper_calls = 0;
  result = nativecheck_signature_pcall_riscv_x86_direct_i128();
  if (result != 51 || nativecheck_direct_x86_i128_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv native i128 signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_i128_helper_calls);
    return 1;
  }

  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly direct x86 signature slot exchange set failed\n",
      stderr);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_signature_pcall_aarch64_x86_direct_sum6();
  if (result != exchange_expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 exchange signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_helper_calls = 0;
  result = nativecheck_signature_pcall_riscv_x86_direct_sum6();
  if (result != exchange_expected || nativecheck_direct_x86_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv exchange signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_helper_calls);
    return 1;
  }

  if (poly_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_I128,
        POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_I128) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly direct x86 preferred i128 signature slot set failed\n",
      stderr);
    return 1;
  }

  nativecheck_direct_x86_i128_helper_calls = 0;
  result = nativecheck_signature_pcall_aarch64_x86_direct_i128();
  if (result != 51 || nativecheck_direct_x86_i128_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 i128 signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_i128_helper_calls);
    return 1;
  }

  nativecheck_direct_x86_i128_helper_calls = 0;
  result = nativecheck_signature_pcall_riscv_x86_direct_i128();
  if (result != 51 || nativecheck_direct_x86_i128_helper_calls != 1) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv i128 signature direct x86 pcall result=%llu calls=%u\n",
      (unsigned long long) result, nativecheck_direct_x86_i128_helper_calls);
    return 1;
  }

  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly direct x86 signature slot restore failed\n",
      stderr);
    return 1;
  }
  if (poly_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_I128,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly direct x86 native i128 signature slot restore failed\n",
      stderr);
    return 1;
  }

  puts("NATIVE_POLY_DIRECT_X86_PCALL_OK");
  return 0;
}

static int run_poly_foreign_signature_pcall_probe(void) {
  const uint64_t expected = 21;
  const uint64_t fp_left_bits = 0x3ff8000000000000ULL;
  const uint64_t fp_right_bits = 0x4002000000000000ULL;
  const uint64_t fp_expected_bits = 0x400b000000000000ULL;
  uint64_t result;
  struct nativecheck_u128 vec_result;

  if (poly_abi_signature_set(5, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly foreign signature setup slot5 reset failed\n",
      stderr);
    return 1;
  }

  result = poly_abi_signature_set(5,
    POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32 + 1);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly x86 ABI signature invalid kind mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }
  result = poly_abi_signature_set(5, POLY_ABI_SIGNATURE_KIND_X86_SYSV);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly x86 stack-capable ABI signature accepted result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  result = poly_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_COUNT,
    POLY_ABI_SIGNATURE_KIND_NATIVE_REGS);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly x86 ABI signature invalid slot mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  result = poly_abi_signature_get(POLY_ABI_SIGNATURE_SLOT_COUNT);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly x86 ABI signature invalid get mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  result = nativecheck_aarch64_abi_signature_set_get_slot5();
  if (result != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 ABI signature control mismatch result=%llu x86_get=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }
  if (poly_abi_signature_set(5, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly foreign signature setup slot5 reset failed\n",
      stderr);
    return 1;
  }
  result = nativecheck_riscv_abi_signature_set_get_slot5();
  if (result != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv ABI signature control mismatch result=%llu x86_get=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }
  if (poly_abi_signature_set(5, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly foreign signature cleanup slot5 failed\n",
      stderr);
    return 1;
  }
  if (poly_abi_signature_set(6, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly foreign signature setup slot6 reset failed\n",
      stderr);
    return 1;
  }
  result = nativecheck_aarch64_abi_signature_set_get_slot6_i128();
  if (result != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128 ||
      poly_abi_signature_get(6) != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 ABI signature i128 control mismatch result=%llu x86_get=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(6));
    return 1;
  }
  if (poly_abi_signature_set(6, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly foreign signature setup slot6 reset failed\n",
      stderr);
    return 1;
  }
  result = nativecheck_riscv_abi_signature_set_get_slot6_i128();
  if (result != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128 ||
      poly_abi_signature_get(6) != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv ABI signature i128 control mismatch result=%llu x86_get=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(6));
    return 1;
  }
  if (poly_abi_signature_set(6, POLY_ABI_SIGNATURE_KIND_EXCHANGE) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly foreign signature cleanup slot6 failed\n",
      stderr);
    return 1;
  }

  result = nativecheck_aarch64_abi_signature_set(5,
    POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32 + 1);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 ABI signature invalid kind mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }
  result = nativecheck_aarch64_abi_signature_set(5,
    POLY_ABI_SIGNATURE_KIND_X86_SYSV);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 stack-capable ABI signature accepted result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  result = nativecheck_aarch64_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_COUNT,
    POLY_ABI_SIGNATURE_KIND_NATIVE_REGS);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 ABI signature invalid slot mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  result = nativecheck_aarch64_abi_signature_get(
    POLY_ABI_SIGNATURE_SLOT_COUNT);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 ABI signature invalid get mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  result = nativecheck_riscv_abi_signature_set(5,
    POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32 + 1);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv ABI signature invalid kind mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }
  result = nativecheck_riscv_abi_signature_set(5,
    POLY_ABI_SIGNATURE_KIND_X86_SYSV);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv stack-capable ABI signature accepted result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  result = nativecheck_riscv_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_COUNT,
    POLY_ABI_SIGNATURE_KIND_NATIVE_REGS);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv ABI signature invalid slot mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  result = nativecheck_riscv_abi_signature_get(
    POLY_ABI_SIGNATURE_SLOT_COUNT);
  if (result != (uint64_t) -EINVAL ||
      poly_abi_signature_get(5) != POLY_ABI_SIGNATURE_KIND_EXCHANGE) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv ABI signature invalid get mismatch result=%llu slot5=%llu\n",
      (unsigned long long) result,
      (unsigned long long) poly_abi_signature_get(5));
    return 1;
  }

  if (poly_abi_signature_set(POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_VEC128_U32,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly foreign vec128 signature slot set failed\n",
      stderr);
    return 1;
  }

  if (poly_abi_signature_set(3, POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0) {
    fputs("NATIVE_CHECK_FAIL: poly foreign signature slot native-regs set failed\n",
      stderr);
    return 1;
  }

  result = nativecheck_signature_imm_pcall_aarch64_riscv_sum6();
  if (result != expected) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 immediate signature riscv pcall result=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  result = nativecheck_signature_imm_pcall_riscv_aarch64_sum6();
  if (result != expected) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv immediate signature aarch64 pcall result=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  result = nativecheck_signature_imm_pcall_aarch64_riscv_fp64(
    fp_left_bits, fp_right_bits);
  if (result != fp_expected_bits) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 immediate signature riscv fp64 result=0x%llx\n",
      (unsigned long long) result);
    return 1;
  }

  result = nativecheck_signature_imm_pcall_riscv_aarch64_fp64(
    fp_left_bits, fp_right_bits);
  if (result != fp_expected_bits) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv immediate signature aarch64 fp64 result=0x%llx\n",
      (unsigned long long) result);
    return 1;
  }

  vec_result = nativecheck_signature_imm_pcall_aarch64_riscv_vec128();
  if (vec_result.lo != 4 || vec_result.hi != 6) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly aarch64 immediate signature riscv vec128 result=0x%llx:0x%llx\n",
      (unsigned long long) vec_result.hi,
      (unsigned long long) vec_result.lo);
    return 1;
  }

  vec_result = nativecheck_signature_imm_pcall_riscv_aarch64_vec128();
  if (vec_result.lo != 4 || vec_result.hi != 6) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly riscv immediate signature aarch64 vec128 result=0x%llx:0x%llx\n",
      (unsigned long long) vec_result.hi,
      (unsigned long long) vec_result.lo);
    return 1;
  }

  if (expect_child_exit("poly signature pcall aarch64 stackless transition",
        0, child_expect_signature_pcall_aarch64_stackless) != 0)
    return 1;
  if (expect_child_exit("poly signature pcall riscv stackless transition",
        0, child_expect_signature_pcall_riscv_stackless) != 0)
    return 1;

  puts("NATIVE_POLY_FOREIGN_SIGNATURE_PCALL_OK");
  return 0;
}

static int run_poly_state_register_bank_probe(void) {
  struct poly_xsave_state snapshot __attribute__((aligned(64)));
  const uint64_t three_bits = 0x4008000000000000ULL;
  const uint64_t five_bits = 0x4014000000000000ULL;
  const uint64_t seven_bits = 0x401c000000000000ULL;
  const uint64_t ten_bits = 0x4024000000000000ULL;
  const uint64_t twelve_bits = 0x4028000000000000ULL;
  const uint64_t aarch64_fpcr_rtz = 0x00c00000ULL;
  const uint64_t aarch64_fpsr_flags = 0x12ULL;
  const uint64_t riscv_fcsr_rtz_flags = 0x75ULL;
  volatile uint64_t aarch64_reservation_word = 0x11ULL;
  volatile uint64_t aarch64_other_reservation_word = 0x22ULL;
  volatile uint64_t riscv_reservation_word = 0x33ULL;
  volatile uint64_t riscv_other_reservation_word = 0x44ULL;

  memset(&snapshot, 0, sizeof(snapshot));
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);

  write_xmm0_u64(three_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd28009b4\n" // movz x20,#77
    ".long 0x1e604014\n" // fmov d20,d0
    ".long 0xd2a01801\n" // movz x1,#0xc0,lsl #16
    ".long 0xd51b4401\n" // msr fpcr,x1
    ".long 0xd2800242\n" // movz x2,#0x12
    ".long 0xd51b4422\n" // msr fpsr,x2
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");

  write_xmm0_u64(five_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x05800a13\n" // addi s4,zero,88
    ".long 0x22a50a53\n" // fsgnj.d f20,fa0,fa0
    ".long 0x07500293\n" // addi t0,zero,0x75
    ".long 0x00329073\n" // csrw fcsr,t0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");

  poly_state_export(&snapshot);
  if (snapshot.aarch64_gpr[20] != 77 ||
      snapshot.aarch64_fp[20].lo != three_bits ||
      snapshot.aarch64_status.fpcr != aarch64_fpcr_rtz ||
      snapshot.aarch64_status.fpsr != aarch64_fpsr_flags ||
      snapshot.riscv_gpr[20] != 88 ||
      snapshot.riscv_fp[20].lo != five_bits ||
      snapshot.riscv_status.fcsr != riscv_fcsr_rtz_flags) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state export register bank mismatch a64x20=%llu a64v20=0x%llx fpcr=0x%llx fpsr=0x%llx rvx20=%llu rvf20=0x%llx rvfcsr=0x%llx\n",
      (unsigned long long) snapshot.aarch64_gpr[20],
      (unsigned long long) snapshot.aarch64_fp[20].lo,
      (unsigned long long) snapshot.aarch64_status.fpcr,
      (unsigned long long) snapshot.aarch64_status.fpsr,
      (unsigned long long) snapshot.riscv_gpr[20],
      (unsigned long long) snapshot.riscv_fp[20].lo,
      (unsigned long long) snapshot.riscv_status.fcsr);
    return 1;
  }

  write_xmm0_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800174\n" // movz x20,#11
    ".long 0x1e604014\n" // fmov d20,d0
    ".long 0xd2800001\n" // movz x1,#0
    ".long 0xd51b4401\n" // msr fpcr,x1
    ".long 0xd51b4421\n" // msr fpsr,x1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");

  write_xmm0_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00b00a13\n" // addi s4,zero,11
    ".long 0x22a50a53\n" // fsgnj.d f20,fa0,fa0
    ".long 0x00301073\n" // csrw fcsr,zero
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "memory");

  poly_state_import(&snapshot);

  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa1403e0\n" // mov x0,x20
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != 77) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state import aarch64 x20 mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd53b4400\n" // mrs x0,fpcr
    ".long 0xd53b4421\n" // mrs x1,fpsr
    ".long 0x8b010000\n" // add x0,x0,x1
    ".long 0xd2800001\n" // movz x1,#0
    ".long 0xd51b4401\n" // msr fpcr,x1
    ".long 0xd51b4421\n" // msr fpsr,x1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != (aarch64_fpcr_rtz + aarch64_fpsr_flags)) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state import aarch64 fpcr/fpsr mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) read_rax(),
      (unsigned long long) (aarch64_fpcr_rtz + aarch64_fpsr_flags));
    return 1;
  }

  write_xmm1_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0x1e612a80\n" // fadd d0,d20,d1
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "xmm1", "memory");
  if (read_xmm0_u64() != ten_bits) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state import aarch64 d20 mismatch got=0x%llx\n",
      (unsigned long long) read_xmm0_u64());
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x000a0513\n" // addi a0,s4,0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != 88) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state import riscv s4 mismatch got=%llu\n",
      (unsigned long long) read_rax());
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00302573\n" // csrr a0,fcsr
    ".long 0x00301073\n" // csrw fcsr,zero
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != riscv_fcsr_rtz_flags) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state import riscv fcsr mismatch got=0x%llx expected=0x%llx\n",
      (unsigned long long) read_rax(),
      (unsigned long long) riscv_fcsr_rtz_flags);
    return 1;
  }

  write_xmm1_u64(seven_bits);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x02ba7553\n" // fadd.d fa0,f20,fa1
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r13", "r14", "xmm0", "xmm1", "memory");
  if (read_xmm0_u64() != twelve_bits) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly state import riscv f20 mismatch got=0x%llx\n",
      (unsigned long long) read_xmm0_u64());
    return 1;
  }

  memset(&snapshot, 0, sizeof(snapshot));
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xc85f7c01\n" // ldxr x1,[x0]
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    :
    : "a"((uint64_t) (uintptr_t) &aarch64_reservation_word)
    : "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  poly_state_export(&snapshot);
  if (snapshot.aarch64_status.reservation_addr !=
        (uint64_t) (uintptr_t) &aarch64_reservation_word ||
      snapshot.aarch64_status.reservation_size != 8) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state export aarch64 reservation mismatch addr=0x%llx size=%llu expected_addr=0x%llx\n",
      (unsigned long long) snapshot.aarch64_status.reservation_addr,
      (unsigned long long) snapshot.aarch64_status.reservation_size,
      (unsigned long long) (uintptr_t) &aarch64_reservation_word);
    return 1;
  }
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xc85f7c01\n" // ldxr x1,[x0]
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    :
    : "a"((uint64_t) (uintptr_t) &aarch64_other_reservation_word)
    : "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  poly_state_import(&snapshot);
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xaa0003e2\n" // mov x2,x0
    ".long 0xd2800665\n" // movz x5,#0x33
    ".long 0xc8047c45\n" // stxr w4,x5,[x2]
    ".long 0xaa0403e0\n" // mov x0,x4
    ".long 0xd5032e1f\n" // aarch64 polyctrl x86 escape
    :
    : "a"((uint64_t) (uintptr_t) &aarch64_reservation_word)
    : "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != 0 || aarch64_reservation_word != 0x33ULL) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state import aarch64 reservation stxr status=%llu word=0x%llx\n",
      (unsigned long long) read_rax(),
      (unsigned long long) aarch64_reservation_word);
    return 1;
  }

  memset(&snapshot, 0, sizeof(snapshot));
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x100535af\n" // lr.d a1,(a0)
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    :
    : "a"((uint64_t) (uintptr_t) &riscv_reservation_word)
    : "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  poly_state_export(&snapshot);
  if (snapshot.riscv_status.reservation_addr !=
        (uint64_t) (uintptr_t) &riscv_reservation_word ||
      snapshot.riscv_status.reservation_size != 8) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state export riscv reservation mismatch addr=0x%llx size=%llu expected_addr=0x%llx\n",
      (unsigned long long) snapshot.riscv_status.reservation_addr,
      (unsigned long long) snapshot.riscv_status.reservation_size,
      (unsigned long long) (uintptr_t) &riscv_reservation_word);
    return 1;
  }
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x100535af\n" // lr.d a1,(a0)
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    :
    : "a"((uint64_t) (uintptr_t) &riscv_other_reservation_word)
    : "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  poly_state_import(&snapshot);
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00050613\n" // addi a2,a0,0
    ".long 0x04400793\n" // addi a5,zero,0x44
    ".long 0x18f6372f\n" // sc.d a4,a5,(a2)
    ".long 0x00070513\n" // addi a0,a4,0
    ".long 0x0000700b\n" // riscv polyctrl x86 escape
    :
    : "a"((uint64_t) (uintptr_t) &riscv_reservation_word)
    : "rbx", "rcx", "rdx", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r13", "r14", "memory");
  if (read_rax() != 0 || riscv_reservation_word != 0x44ULL) {
    fprintf(stderr,
      "NATIVE_CHECK_FAIL: poly state import riscv reservation sc status=%llu word=0x%llx\n",
      (unsigned long long) read_rax(),
      (unsigned long long) riscv_reservation_word);
    return 1;
  }

  puts("NATIVE_POLY_STATE_REGISTER_BANK_OK");
  return 0;
}

int main(void) {
  const char *expect_poly_cpuid = getenv("EXPECT_POLY_CPUID");

  puts("NATIVE_ELF_OK");
  if (expect_poly_cpuid != NULL && strcmp(expect_poly_cpuid, "0") == 0) {
    if (poly_cpuid_present()) {
      fputs("NATIVE_CHECK_FAIL: poly CPUID leaf visible while disabled\n", stderr);
      return 1;
    }
    puts("NATIVE_CPUID_POLY_ABSENT");
  }
  else if (expect_poly_cpuid != NULL && strcmp(expect_poly_cpuid, "1") == 0) {
    if (!poly_cpuid_present()) {
      fputs("NATIVE_CHECK_FAIL: poly CPUID leaf missing while enabled\n", stderr);
      return 1;
    }
    struct poly_cpuid_regs features = poly_read_cpuid(POLY_CPUID_BASE + 1, 0);
    uint32_t expected_features = poly_cpuid_expected_feature_mask();
    uint32_t forbidden_feature_bits = poly_cpuid_forbidden_feature_mask();
    if ((features.ecx & forbidden_feature_bits) != 0) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: prototype CPUID feature bits advertised ecx=0x%x forbidden=0x%x\n",
        features.ecx, forbidden_feature_bits);
      return 1;
    }
    if (features.eax != POLY_CPUID_ABI_VERSION ||
        features.ebx != poly_cpuid_expected_mode_mask() ||
        features.ecx != expected_features ||
        features.edx != POLY_STATE_XSAVE_COMPONENT_ARCH) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID feature leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x expected_ecx=0x%x expected_edx=0x%x\n",
        features.eax, features.ebx, features.ecx, features.edx,
        expected_features, POLY_STATE_XSAVE_COMPONENT_ARCH);
      return 1;
    }
    struct poly_cpuid_regs expected_import_manifest =
      poly_cpuid_expected_escape_leaf5();
    struct poly_cpuid_regs import_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 5);
    if (import_manifest.eax != expected_import_manifest.eax ||
        import_manifest.ebx != expected_import_manifest.ebx ||
        import_manifest.ecx != expected_import_manifest.ecx ||
        import_manifest.edx != expected_import_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID import manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        import_manifest.eax, import_manifest.ebx, import_manifest.ecx,
        import_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_switch_manifest =
      poly_cpuid_expected_escape_leaf6();
    struct poly_cpuid_regs switch_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 6);
    if (switch_manifest.eax != expected_switch_manifest.eax ||
        switch_manifest.ebx != expected_switch_manifest.ebx ||
        switch_manifest.ecx != expected_switch_manifest.ecx ||
        switch_manifest.edx != expected_switch_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID generic switch manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        switch_manifest.eax, switch_manifest.ebx, switch_manifest.ecx,
        switch_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_pcall_imm_manifest =
      poly_cpuid_expected_escape_leaf7();
    struct poly_cpuid_regs pcall_imm_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 7);
    if (pcall_imm_manifest.eax != expected_pcall_imm_manifest.eax ||
        pcall_imm_manifest.ebx != expected_pcall_imm_manifest.ebx ||
        pcall_imm_manifest.ecx != expected_pcall_imm_manifest.ecx ||
        pcall_imm_manifest.edx != expected_pcall_imm_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID immediate pcall manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        pcall_imm_manifest.eax, pcall_imm_manifest.ebx,
        pcall_imm_manifest.ecx, pcall_imm_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_foreign_sig_manifest =
      poly_cpuid_expected_escape_leaf8();
    struct poly_cpuid_regs foreign_sig_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 8);
    if (foreign_sig_manifest.eax != expected_foreign_sig_manifest.eax ||
        foreign_sig_manifest.ebx != expected_foreign_sig_manifest.ebx ||
        foreign_sig_manifest.ecx != expected_foreign_sig_manifest.ecx ||
        foreign_sig_manifest.edx != expected_foreign_sig_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID foreign signature pcall manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        foreign_sig_manifest.eax, foreign_sig_manifest.ebx,
        foreign_sig_manifest.ecx, foreign_sig_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_landing_manifest =
      poly_cpuid_expected_escape_leaf9();
    struct poly_cpuid_regs landing_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 9);
    if (landing_manifest.eax != expected_landing_manifest.eax ||
        landing_manifest.ebx != expected_landing_manifest.ebx ||
        landing_manifest.ecx != expected_landing_manifest.ecx ||
        landing_manifest.edx != expected_landing_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID landing manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        landing_manifest.eax, landing_manifest.ebx,
        landing_manifest.ecx, landing_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_abi_manifest =
      poly_cpuid_expected_escape_leaf10();
    struct poly_cpuid_regs abi_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 10);
    if (abi_manifest.eax != expected_abi_manifest.eax ||
        abi_manifest.ebx != expected_abi_manifest.ebx ||
        abi_manifest.ecx != expected_abi_manifest.ecx ||
        abi_manifest.edx != expected_abi_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID ABI control manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        abi_manifest.eax, abi_manifest.ebx, abi_manifest.ecx,
        abi_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_foreign_imm_manifest =
      poly_cpuid_expected_escape_leaf11();
    struct poly_cpuid_regs foreign_imm_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 11);
    if (foreign_imm_manifest.eax != expected_foreign_imm_manifest.eax ||
        foreign_imm_manifest.ebx != expected_foreign_imm_manifest.ebx ||
        foreign_imm_manifest.ecx != expected_foreign_imm_manifest.ecx ||
        foreign_imm_manifest.edx != expected_foreign_imm_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID foreign immediate pcall manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        foreign_imm_manifest.eax, foreign_imm_manifest.ebx,
        foreign_imm_manifest.ecx, foreign_imm_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_trap_setup_manifest =
      poly_cpuid_expected_escape_leaf12();
    struct poly_cpuid_regs trap_setup_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 12);
    if (trap_setup_manifest.eax != expected_trap_setup_manifest.eax ||
        trap_setup_manifest.ebx != expected_trap_setup_manifest.ebx ||
        trap_setup_manifest.ecx != expected_trap_setup_manifest.ecx ||
        trap_setup_manifest.edx != expected_trap_setup_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID trap setup manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        trap_setup_manifest.eax, trap_setup_manifest.ebx,
        trap_setup_manifest.ecx, trap_setup_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_aarch64_trap_query_manifest =
      poly_cpuid_expected_escape_leaf13();
    struct poly_cpuid_regs aarch64_trap_query_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 13);
    if (aarch64_trap_query_manifest.eax !=
          expected_aarch64_trap_query_manifest.eax ||
        aarch64_trap_query_manifest.ebx !=
          expected_aarch64_trap_query_manifest.ebx ||
        aarch64_trap_query_manifest.ecx !=
          expected_aarch64_trap_query_manifest.ecx ||
        aarch64_trap_query_manifest.edx !=
          expected_aarch64_trap_query_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID aarch64 trap query manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        aarch64_trap_query_manifest.eax, aarch64_trap_query_manifest.ebx,
        aarch64_trap_query_manifest.ecx, aarch64_trap_query_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_riscv_trap_query_manifest =
      poly_cpuid_expected_escape_leaf14();
    struct poly_cpuid_regs riscv_trap_query_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 14);
    if (riscv_trap_query_manifest.eax !=
          expected_riscv_trap_query_manifest.eax ||
        riscv_trap_query_manifest.ebx !=
          expected_riscv_trap_query_manifest.ebx ||
        riscv_trap_query_manifest.ecx !=
          expected_riscv_trap_query_manifest.ecx ||
        riscv_trap_query_manifest.edx !=
          expected_riscv_trap_query_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID riscv trap query manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        riscv_trap_query_manifest.eax, riscv_trap_query_manifest.ebx,
        riscv_trap_query_manifest.ecx, riscv_trap_query_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_landing_policy_manifest =
      poly_cpuid_expected_escape_leaf15();
    struct poly_cpuid_regs landing_policy_manifest =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 15);
    if (landing_policy_manifest.eax != expected_landing_policy_manifest.eax ||
        landing_policy_manifest.ebx != expected_landing_policy_manifest.ebx ||
        landing_policy_manifest.ecx != expected_landing_policy_manifest.ecx ||
        landing_policy_manifest.edx != expected_landing_policy_manifest.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID landing policy manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        landing_policy_manifest.eax, landing_policy_manifest.ebx,
        landing_policy_manifest.ecx, landing_policy_manifest.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_landing_policy_manifest2 =
      poly_cpuid_expected_escape_leaf16();
    struct poly_cpuid_regs landing_policy_manifest2 =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 16);
    if (landing_policy_manifest2.eax !=
          expected_landing_policy_manifest2.eax ||
        landing_policy_manifest2.ebx !=
          expected_landing_policy_manifest2.ebx ||
        landing_policy_manifest2.ecx !=
          expected_landing_policy_manifest2.ecx ||
        landing_policy_manifest2.edx !=
          expected_landing_policy_manifest2.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID landing policy details mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        landing_policy_manifest2.eax, landing_policy_manifest2.ebx,
        landing_policy_manifest2.ecx, landing_policy_manifest2.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_signature_manifest2 =
      poly_cpuid_expected_escape_leaf17();
    struct poly_cpuid_regs signature_manifest2 =
      poly_read_cpuid(POLY_CPUID_BASE + 2, 17);
    if (signature_manifest2.eax != expected_signature_manifest2.eax ||
        signature_manifest2.ebx != expected_signature_manifest2.ebx ||
        signature_manifest2.ecx != expected_signature_manifest2.ecx ||
        signature_manifest2.edx != expected_signature_manifest2.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID extended ABI signature manifest mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        signature_manifest2.eax, signature_manifest2.ebx,
        signature_manifest2.ecx, signature_manifest2.edx);
      return 1;
    }
    if (check_poly_abi_signature_slot_default(
          pcall_imm_manifest.ecx & 0xffU,
          pcall_imm_manifest.edx & 0xffU, "exchange") != 0 ||
        check_poly_abi_signature_slot_default(
          (pcall_imm_manifest.ecx >> 8) & 0xffU,
          (pcall_imm_manifest.edx >> 8) & 0xffU, "x86-sysv-regs") != 0 ||
        check_poly_abi_signature_slot_default(
          (pcall_imm_manifest.ecx >> 16) & 0xffU,
          (pcall_imm_manifest.edx >> 16) & 0xffU,
          "x86-sysv-regs-i128") != 0 ||
        check_poly_abi_signature_slot_default(
          (pcall_imm_manifest.ecx >> 24) & 0xffU,
          (pcall_imm_manifest.edx >> 24) & 0xffU, "native-regs") != 0 ||
        check_poly_abi_signature_slot_default(
          signature_manifest2.eax, signature_manifest2.ebx,
          "native-regs-i128") != 0 ||
        check_poly_abi_signature_slot_default(
          signature_manifest2.ecx, signature_manifest2.edx,
          "native-regs-vec128-u32") != 0)
      return 1;
    struct poly_cpuid_regs expected_state = poly_cpuid_expected_state_leaf();
    struct poly_cpuid_regs state = poly_read_cpuid(POLY_CPUID_BASE + 3, 0);
    uint32_t forbidden_state_bits = POLY_CPUID_STATE_SYNTHETIC_BANKS |
      POLY_CPUID_STATE_KEY_CR3 |
      POLY_CPUID_STATE_KEY_FSBASE |
      POLY_CPUID_STATE_KEY_STACK_REGION |
      POLY_CPUID_STATE_KEY_EXPLICIT;
    if ((state.eax & forbidden_state_bits) != 0) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: hidden-bank CPUID state bits advertised eax=0x%x forbidden=0x%x\n",
        state.eax, forbidden_state_bits);
      return 1;
    }
    if (state.eax != expected_state.eax ||
        state.ebx != expected_state.ebx ||
        state.ecx != expected_state.ecx ||
        state.edx != expected_state.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID state leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        state.eax, state.ebx, state.ecx, state.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_arch_state =
      poly_cpuid_expected_arch_state_leaf();
    struct poly_cpuid_regs arch_state =
      poly_read_cpuid(POLY_CPUID_BASE + 4, 0);
    if (arch_state.eax != expected_arch_state.eax ||
        arch_state.ebx != expected_arch_state.ebx ||
        arch_state.ecx != expected_arch_state.ecx ||
        arch_state.edx != expected_arch_state.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID arch state leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        arch_state.eax, arch_state.ebx, arch_state.ecx, arch_state.edx);
      return 1;
    }
    struct poly_cpuid_regs xsave0 = poly_read_cpuid(0x0d, 0);
    if ((xsave0.eax & (1U << POLY_STATE_XSAVE_COMPONENT_ARCH)) == 0) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: CPUID.0xD missing poly xstate bit eax=0x%x\n",
        xsave0.eax);
      return 1;
    }
    if (xsave0.ecx < POLY_STATE_XSAVE_OFFSET_ARCH + POLY_STATE_XSAVE_BYTES_ARCH) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: CPUID.0xD max xsave area too small ecx=0x%x\n",
        xsave0.ecx);
      return 1;
    }
    struct poly_cpuid_regs poly_xsave =
      poly_read_cpuid(0x0d, POLY_STATE_XSAVE_COMPONENT_ARCH);
    if (poly_xsave.eax != POLY_STATE_XSAVE_BYTES_ARCH ||
        poly_xsave.ebx != POLY_STATE_XSAVE_OFFSET_ARCH ||
        (poly_xsave.ecx & (1U << 1)) == 0 ||
        poly_xsave.edx != 0) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: CPUID.0xD poly component mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        poly_xsave.eax, poly_xsave.ebx, poly_xsave.ecx,
        poly_xsave.edx);
      return 1;
    }
    uint64_t xcr0 = read_xcr0();
    if ((xcr0 & (1ULL << POLY_STATE_XSAVE_COMPONENT_ARCH)) != 0)
      puts("NATIVE_POLY_XSAVE_OS_ENABLED");
    else
      puts("NATIVE_POLY_XSAVE_OS_DISABLED");
    request_poly_xsave_permission(&xcr0);
    struct poly_cpuid_regs expected_trap =
      poly_cpuid_expected_trap_leaf();
    struct poly_cpuid_regs trap =
      poly_read_cpuid(POLY_CPUID_BASE + 5, 0);
    if (trap.eax != expected_trap.eax ||
        trap.ebx != expected_trap.ebx ||
        trap.ecx != expected_trap.ecx ||
        trap.edx != expected_trap.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID trap leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        trap.eax, trap.ebx, trap.ecx, trap.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_interrupt =
      poly_cpuid_expected_interrupt_leaf();
    struct poly_cpuid_regs interrupt =
      poly_read_cpuid(POLY_CPUID_BASE + 6, 0);
    if (interrupt.eax != expected_interrupt.eax ||
        interrupt.ebx != expected_interrupt.ebx ||
        interrupt.ecx != expected_interrupt.ecx ||
        interrupt.edx != expected_interrupt.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID interrupt leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        interrupt.eax, interrupt.ebx, interrupt.ecx, interrupt.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_memory =
      poly_cpuid_expected_memory_leaf();
    struct poly_cpuid_regs memory =
      poly_read_cpuid(POLY_CPUID_BASE + 7, 0);
    if (memory.eax != expected_memory.eax ||
        memory.ebx != expected_memory.ebx ||
        memory.ecx != expected_memory.ecx ||
        memory.edx != expected_memory.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID memory leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        memory.eax, memory.ebx, memory.ecx, memory.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_transition =
      poly_cpuid_expected_transition_leaf();
    struct poly_cpuid_regs transition =
      poly_read_cpuid(POLY_CPUID_BASE + 8, 0);
    if (transition.eax != expected_transition.eax ||
        transition.ebx != expected_transition.ebx ||
        transition.ecx != expected_transition.ecx ||
        transition.edx != expected_transition.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID transition leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        transition.eax, transition.ebx, transition.ecx, transition.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_frontends =
      poly_cpuid_expected_frontend_leaf();
    struct poly_cpuid_regs frontends =
      poly_read_cpuid(POLY_CPUID_BASE + 8, 1);
    if (frontends.eax != expected_frontends.eax ||
        frontends.ebx != expected_frontends.ebx ||
        frontends.ecx != expected_frontends.ecx ||
        frontends.edx != expected_frontends.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID frontend leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        frontends.eax, frontends.ebx, frontends.ecx, frontends.edx);
      return 1;
    }
    struct poly_cpuid_regs expected_abi_bridge =
      poly_cpuid_expected_abi_bridge_leaf();
    struct poly_cpuid_regs abi_bridge =
      poly_read_cpuid(POLY_CPUID_BASE + 9, 0);
    const uint32_t forbidden_abi_bridge_flags =
      poly_cpuid_forbidden_abi_bridge_mask();
    if ((abi_bridge.ebx & forbidden_abi_bridge_flags) != 0) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: non-register ABI work advertised as bridge hardware ebx=0x%x forbidden=0x%x\n",
        abi_bridge.ebx, forbidden_abi_bridge_flags);
      return 1;
    }
    if ((abi_bridge.ebx & POLY_ABI_BRIDGE_FLAG_NATIVE_I128_SIGNATURES) == 0) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: native i128 signature support not advertised ebx=0x%x\n",
        abi_bridge.ebx);
      return 1;
    }
    if (abi_bridge.eax != expected_abi_bridge.eax ||
        abi_bridge.ebx != expected_abi_bridge.ebx ||
        abi_bridge.ecx != expected_abi_bridge.ecx ||
        abi_bridge.edx != expected_abi_bridge.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID ABI bridge leaf mismatch eax=0x%x/%x ebx=0x%x/%x ecx=0x%x/%x edx=0x%x/%x\n",
        abi_bridge.eax, expected_abi_bridge.eax,
        abi_bridge.ebx, expected_abi_bridge.ebx,
        abi_bridge.ecx, expected_abi_bridge.ecx,
        abi_bridge.edx, expected_abi_bridge.edx);
      return 1;
    }
    puts("NATIVE_CPUID_POLY_PRESENT");
    if (run_poly_trap_vector_probe() != 0)
      return 1;
    if (run_poly_no_vector_signal_probe() != 0)
      return 1;
    if (run_poly_invalid_generic_control_signal_probe() != 0)
      return 1;
    if (run_poly_landing_policy_probe() != 0)
      return 1;
    if (run_poly_state_key_probe() != 0)
      return 1;
    if (run_poly_state_save_restore_probe() != 0)
      return 1;
    if (run_poly_real_xsave_probe(xcr0) != 0)
      return 1;
    if (run_poly_import_return_xsave_probe() != 0)
      return 1;
    if (run_poly_direct_x86_pcall_probe() != 0)
      return 1;
    if (run_poly_foreign_signature_pcall_probe() != 0)
      return 1;
    if (run_poly_state_register_bank_probe() != 0)
      return 1;
  }
  puts("NATIVE_CHECK_OK");
  return 0;
}
