#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

enum {
  POLYSIGNAL_LOOP_COUNT = 200000
};

static volatile sig_atomic_t signal_count;

static void handle_alarm(int signo) {
  (void) signo;
  signal_count++;
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
    ".byte 0x40,0x0f,0x0b,0x50,0x43,0x41,0x36,0x34\n"
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

static uint64_t pcall_riscv_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x40,0x0f,0x0b,0x50,0x43,0x52,0x56,0x36\n"
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

static int check_arch(const char *name, uint64_t seed,
  uint64_t (*pcall)(uint64_t, uint64_t))
{
  sig_atomic_t before = signal_count;

  if (arm_alarm() != 0)
    return -1;

  uint64_t result = pcall(seed, POLYSIGNAL_LOOP_COUNT);
  if (result != seed + 1) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s got=%llu expected=%llu\n",
      name, (unsigned long long) result, (unsigned long long) (seed + 1));
    return -1;
  }

  if (signal_count == before) {
    fprintf(stderr, "POLYSIGNAL_FAIL: arch=%s signal not delivered\n", name);
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
  if (check_arch("aarch64", 0x51000000ULL, pcall_aarch64_signal) != 0)
    return 1;
  if (check_arch("riscv", 0x52000000ULL, pcall_riscv_signal) != 0)
    return 1;

  printf("POLYSIGNAL_OK\n");
  return 0;
}
