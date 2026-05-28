#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>

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
