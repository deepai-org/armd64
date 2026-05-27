#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>

enum {
  POLYTHREAD_THREADS = 4,
  POLYTHREAD_ROUNDS = 12,
  POLYTHREAD_BUSY = 20000
};

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

static void *worker_main(void *arg) {
  uintptr_t worker_id = (uintptr_t) arg;
  uint64_t base = 0x10000000ULL + worker_id * 0x10000ULL;

  for (unsigned round = 0; round < POLYTHREAD_ROUNDS; round++) {
    uint64_t aarch64_seed = base + round * 2;
    uint64_t riscv_seed = base + round * 2 + 1;
    uint64_t hidden_aarch64_seed = base + 0x4000ULL + round * 2;
    uint64_t hidden_riscv_seed = base + 0x4000ULL + round * 2 + 1;
    uint64_t aarch64_result = pcall_aarch64_busy(aarch64_seed);
    uint64_t riscv_result = pcall_riscv_busy(riscv_seed);
    uint64_t hidden_aarch64_result =
      pcall_aarch64_hidden_busy(hidden_aarch64_seed);
    uint64_t hidden_riscv_result =
      pcall_riscv_hidden_busy(hidden_riscv_seed);

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

    sched_yield();
  }

  return 0;
}

int main(void) {
  pthread_t threads[POLYTHREAD_THREADS];

  printf("POLYTHREAD_START\n");
  for (uintptr_t n = 0; n < POLYTHREAD_THREADS; n++) {
    if (pthread_create(&threads[n], 0, worker_main, (void *) n) != 0) {
      fprintf(stderr, "POLYTHREAD_FAIL: pthread_create %lu\n", (unsigned long) n);
      return 1;
    }
  }

  for (unsigned n = 0; n < POLYTHREAD_THREADS; n++) {
    void *status = 0;
    if (pthread_join(threads[n], &status) != 0 || status != 0) {
      fprintf(stderr, "POLYTHREAD_FAIL: pthread_join %u\n", n);
      return 1;
    }
  }

  printf("POLYTHREAD_OK\n");
  return 0;
}
