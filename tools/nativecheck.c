#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "polycpuid.h"

#define POLY_OP_ENTER_A64 ".byte 0x0f,0x24,0x01,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_ENTER_RV64 ".byte 0x0f,0x24,0x02,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x24,0x60,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_GET ".byte 0x0f,0x24,0x61,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x24,0x62,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x24,0x63,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_VECTOR_MODE_GET ".byte 0x0f,0x24,0x64,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_TRAP_STATUS_REASON ".byte 0x0f,0x24,0x50,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_SYSCALL_STATUS_NUMBER ".byte 0x0f,0x24,0x31,0x50,0x4f,0x4c,0x59,0x21\n"
#define POLY_OP_SYSCALL_STATUS_MODE ".byte 0x0f,0x24,0x32,0x50,0x4f,0x4c,0x59,0x21\n"

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
}

static inline void write_rax(uint64_t value) {
  asm volatile("" :: "a"(value) : "memory");
}

static inline uint64_t read_xmm0_u64(void) {
  uint64_t value;
  asm volatile("movq %%xmm0,%0" : "=r"(value));
  return value;
}

static inline void write_xmm0_u64(uint64_t value) {
  asm volatile("movq %0,%%xmm0" :: "r"(value) : "xmm0", "memory");
}

static inline void poly_trap_vector_set(void) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET ::: "memory");
}

static inline void poly_trap_vector_get(void) {
  asm volatile(POLY_OP_TRAP_VECTOR_GET ::: "memory");
}

static inline void poly_trap_vector_mode_set(void) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET ::: "memory");
}

static inline void poly_trap_vector_mode_get(void) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_GET ::: "memory");
}

static inline uint64_t poly_trap_status_reason(void) {
  uint64_t value;
  asm volatile(POLY_OP_TRAP_STATUS_REASON : "=a"(value) :: "memory");
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
    "jne 9f\n"
    "cmpq $3, %rbx\n"
    "jne 5f\n"
    "cmpq $5, %rcx\n"
    "jne 9f\n"
    "cmpq $5, %rsi\n"
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
    "movq $4545, %rax\n"
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
extern const unsigned char poly_riscv_trap_vector_raw[];

__asm__(
  ".pushsection .text\n"
  ".balign 4\n"
  ".globl poly_aarch64_trap_vector_raw\n"
  ".type poly_aarch64_trap_vector_raw,@function\n"
  "poly_aarch64_trap_vector_raw:\n"
  ".long 0xaa0a03e0\n" // mov x0,x10, return trap arg5
  ".long 0xd42fff20\n" // brk #0x7ff9, architectural trap return
  "ud2\n"
  ".size poly_aarch64_trap_vector_raw, .-poly_aarch64_trap_vector_raw\n"
  ".balign 4\n"
  ".globl poly_riscv_trap_vector_raw\n"
  ".type poly_riscv_trap_vector_raw,@function\n"
  "poly_riscv_trap_vector_raw:\n"
  ".long 0x00038513\n" // addi a0,t2,0, return trap arg5
  ".long 0x0000407b\n" // custom trap return
  "ud2\n"
  ".size poly_riscv_trap_vector_raw, .-poly_riscv_trap_vector_raw\n"
  ".popsection\n");

static int run_poly_trap_vector_probe(void) {
  void *handler = (void *) poly_trap_vector_handler;
  uint64_t expected_pid = (uint64_t) getpid();
  write_rax(POLY_MODE_X86);
  poly_trap_vector_mode_set();
  write_rax((uint64_t) handler);
  poly_trap_vector_set();
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

  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
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
      poly_syscall_status_mode() != POLY_MODE_RAW_AARCH64) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly parent syscall status mismatch number=%llu mode=%llu\n",
      (unsigned long long) poly_syscall_status_number(),
      (unsigned long long) poly_syscall_status_mode());
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

  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x0ac00893\n" // addi x17,x0,172
    ".long 0x00000073\n" // ecall
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
  result = read_rax();
  if (result != expected_pid) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv ecall trap vector result mismatch got=%llu expected=%llu\n",
      (unsigned long long) result, (unsigned long long) expected_pid);
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
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
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
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
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
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
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
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
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
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
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
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
  fp_result = read_xmm0_u64();
  if (fp_result != 0x4010000000000000ULL) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv trap return preserved fp register mismatch got=0x%llx\n",
      (unsigned long long) fp_result);
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd42000a0\n" // brk #5
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
  result = read_rax();
  if (result != 4444) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64 brk trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00500893\n" // addi x17,x0,5
    ".long 0x00100073\n" // ebreak
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
  result = read_rax();
  if (result != 4545) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv ebreak trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  write_rax(POLY_MODE_RAW_AARCH64);
  poly_trap_vector_mode_set();
  write_rax((uint64_t) (void *) poly_aarch64_trap_vector_raw);
  poly_trap_vector_set();
  asm volatile(
    POLY_OP_ENTER_RV64
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x0ac00893\n" // addi a7,zero,172
    ".long 0x00000073\n" // ecall
    ".long 0x0000000b\n" // custom-0 x86 escape
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
  result = read_rax();
  if (result != 6) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly riscv-to-aarch64 trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  write_rax(POLY_MODE_RAW_RISCV);
  poly_trap_vector_mode_set();
  write_rax((uint64_t) (void *) poly_riscv_trap_vector_raw);
  poly_trap_vector_set();
  asm volatile(
    POLY_OP_ENTER_A64
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd2801588\n" // movz x8,#172
    ".long 0xd40000e1\n" // svc #7
    ".long 0xd42fffe0\n" // brk #0x7fff
    ::: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "memory");
  result = read_rax();
  if (result != 6) {
    fprintf(stderr, "NATIVE_CHECK_FAIL: poly aarch64-to-riscv trap vector result mismatch got=%llu\n",
      (unsigned long long) result);
    return 1;
  }

  write_rax(0);
  poly_trap_vector_set();
  write_rax(POLY_MODE_X86);
  poly_trap_vector_mode_set();
  puts("NATIVE_POLY_TRAP_VECTOR_OK");
  return 0;
}

int main(void) {
  const char *expect_poly_cpuid = getenv("EXPECT_POLY_CPUID");
  const char *expect_poly_compat_traps = getenv("EXPECT_POLY_COMPAT_TRAPS");

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
    int compat_traps = expect_poly_compat_traps == NULL ||
      strcmp(expect_poly_compat_traps, "0") != 0;
    struct poly_cpuid_regs features = poly_read_cpuid(POLY_CPUID_BASE + 1, 0);
    uint32_t expected_features =
      poly_cpuid_expected_feature_mask_for_compat(compat_traps);
    if (features.eax != POLY_CPUID_ABI_VERSION ||
        features.ebx != poly_cpuid_expected_mode_mask() ||
        features.ecx != expected_features ||
        features.edx != 0) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID feature leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x expected_ecx=0x%x\n",
        features.eax, features.ebx, features.ecx, features.edx,
        expected_features);
      return 1;
    }
    struct poly_cpuid_regs expected_state = poly_cpuid_expected_state_leaf();
    struct poly_cpuid_regs state = poly_read_cpuid(POLY_CPUID_BASE + 3, 0);
    if (state.eax != expected_state.eax ||
        state.ebx != expected_state.ebx ||
        state.ecx != expected_state.ecx ||
        state.edx != expected_state.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID state leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        state.eax, state.ebx, state.ecx, state.edx);
      return 1;
    }
    puts("NATIVE_CPUID_POLY_PRESENT");
    if (!compat_traps && run_poly_trap_vector_probe() != 0)
      return 1;
  }
  puts("NATIVE_CHECK_OK");
  return 0;
}
