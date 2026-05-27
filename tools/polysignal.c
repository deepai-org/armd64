#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "polycpuid.h"

#define POLY_OP_STATE_EXPORT ".byte 0x0f,0x24,0x67,0x50,0x4f,0x4c,0x59,0x21\n"

enum {
  POLYSIGNAL_LOOP_COUNT = 200000
};

static volatile sig_atomic_t signal_count;
static volatile sig_atomic_t signal_transition_count;
static volatile sig_atomic_t signal_transition_bad;
static volatile sig_atomic_t signal_expected_mode;
static struct poly_xsave_state signal_snapshot __attribute__((aligned(64)));

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

static inline void poly_state_export(struct poly_xsave_state *state) {
  asm volatile(POLY_OP_STATE_EXPORT :: "a"(state) : "memory");
}

static void handle_alarm(int signo) {
  (void) signo;
  signal_count++;
  poly_state_export(&signal_snapshot);

  uint16_t flags = signal_snapshot.transition.active.flags;
  uint32_t target_mode = signal_snapshot.transition.active.target_mode;
  if ((flags & POLY_TRANSITION_FLAG_INTERRUPTED_RAW) != 0 &&
      target_mode == (uint32_t) signal_expected_mode)
    signal_transition_count++;
  else
    signal_transition_bad++;
}

static int arm_alarm(void) {
  struct itimerval timer;
  memset(&timer, 0, sizeof(timer));
  timer.it_value.tv_usec = 1000;

  if (setitimer(ITIMER_REAL, &timer, 0) != 0) {
    fprintf(stderr, "POLYSIGNAL_FAIL: setitimer: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

static uint64_t pcall_aarch64_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
    "1:\n"
    ".long 0xf1000421\n" // subs x1,x1,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result), "+D"(seed), "+S"(loops)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_aarch64_hidden_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
    "1:\n"
    ".long 0x91000014\n" // add x20,x0,#0
    ".long 0xf1000421\n" // subs x1,x1,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x91001e80\n" // add x0,x20,#7
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result), "+D"(seed), "+S"(loops)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_riscv_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
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
    : "=a"(result), "+D"(seed), "+S"(loops)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_riscv_hidden_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
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
    : "=a"(result), "+D"(seed), "+S"(loops)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_aarch64_hidden_fp_signal(uint64_t left_bits,
    uint64_t right_bits, uint64_t loops) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x10,0x50,0x4f,0x4c,0x59,0x21\n"
    "1:\n"
    ".long 0x1e604014\n" // fmov d20,d0
    ".long 0xf1000421\n" // subs x1,x1,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x1e612a80\n" // fadd d0,d20,d1
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "+S"(loops)
    :
    : "rax", "rcx", "rdx", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_riscv_hidden_fp_signal(uint64_t left_bits,
    uint64_t right_bits, uint64_t loops) {
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
    : "+S"(loops)
    :
    : "rax", "rcx", "rdx", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static int check_arch(const char *name, uint64_t seed, uint64_t expected_delta,
  uint64_t (*pcall)(uint64_t, uint64_t))
{
  sig_atomic_t before = signal_count;
  sig_atomic_t transition_before = signal_transition_count;
  sig_atomic_t bad_before = signal_transition_bad;

  if (arm_alarm() != 0)
    return -1;

  uint64_t result = pcall(seed, POLYSIGNAL_LOOP_COUNT);
  if (result != seed + expected_delta) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s got=%llu expected=%llu\n",
      name, (unsigned long long) result,
      (unsigned long long) (seed + expected_delta));
    return -1;
  }

  if (signal_count == before) {
    fprintf(stderr, "POLYSIGNAL_FAIL: arch=%s signal not delivered\n", name);
    return -1;
  }
  if (signal_transition_count == transition_before ||
      signal_transition_bad != bad_before) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s interrupted raw transition missing count=%d bad=%d\n",
      name, (int) (signal_transition_count - transition_before),
      (int) (signal_transition_bad - bad_before));
    return -1;
  }

  return 0;
}

static int check_arch_fp(const char *name, uint64_t seed,
  uint64_t (*pcall)(uint64_t, uint64_t, uint64_t))
{
  sig_atomic_t before = signal_count;
  sig_atomic_t transition_before = signal_transition_count;
  sig_atomic_t bad_before = signal_transition_bad;
  uint64_t seven_bits = double_to_bits(7.0);

  if (arm_alarm() != 0)
    return -1;

  uint64_t result = pcall(double_to_bits((double) seed), seven_bits,
    POLYSIGNAL_LOOP_COUNT);
  uint64_t expected = double_to_bits((double) seed + 7.0);
  if (result != expected) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s got=0x%llx expected=0x%llx\n",
      name, (unsigned long long) result, (unsigned long long) expected);
    return -1;
  }

  if (signal_count == before) {
    fprintf(stderr, "POLYSIGNAL_FAIL: arch=%s signal not delivered\n", name);
    return -1;
  }
  if (signal_transition_count == transition_before ||
      signal_transition_bad != bad_before) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s interrupted raw transition missing count=%d bad=%d\n",
      name, (int) (signal_transition_count - transition_before),
      (int) (signal_transition_bad - bad_before));
    return -1;
  }

  return 0;
}

int main(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_alarm;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGALRM, &sa, 0) != 0) {
    fprintf(stderr, "POLYSIGNAL_FAIL: sigaction: %s\n", strerror(errno));
    return 1;
  }

  printf("POLYSIGNAL_START\n");
  signal_expected_mode = POLY_MODE_RAW_AARCH64;
  if (check_arch("aarch64", 0x51000000ULL, 1, pcall_aarch64_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_RISCV;
  if (check_arch("riscv", 0x52000000ULL, 1, pcall_riscv_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_AARCH64;
  if (check_arch("aarch64-hidden", 0x53000000ULL, 7,
      pcall_aarch64_hidden_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_RISCV;
  if (check_arch("riscv-hidden", 0x54000000ULL, 7,
      pcall_riscv_hidden_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_AARCH64;
  if (check_arch_fp("aarch64-hidden-fp", 0x55000000ULL,
      pcall_aarch64_hidden_fp_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_RISCV;
  if (check_arch_fp("riscv-hidden-fp", 0x56000000ULL,
      pcall_riscv_hidden_fp_signal) != 0)
    return 1;

  printf("POLYSIGNAL_OK\n");
  return 0;
}
