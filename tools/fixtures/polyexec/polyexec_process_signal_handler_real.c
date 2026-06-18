#include <stdint.h>

enum {
  POLY_SIG_BLOCK = 0,
  POLY_SIG_UNBLOCK = 1,
  POLY_SIGUSR1 = 10,
  POLY_KERNEL_SIGSET_SIZE = 8,

  POLY_SYS_WRITE = 64,
  POLY_SYS_KILL = 129,
  POLY_SYS_RT_SIGACTION = 134,
  POLY_SYS_RT_SIGPROCMASK = 135,
  POLY_SYS_GETPID = 172,
};

struct poly_linux_ksigaction {
  uint64_t handler;
  uint64_t flags;
  uint64_t restorer;
  uint64_t mask;
};

__attribute__((visibility("hidden")))
volatile uint64_t poly_handler_count;
__attribute__((visibility("hidden")))
volatile uint64_t poly_handler_signum;
__attribute__((visibility("hidden")))
volatile uint64_t poly_handler_x19_before;
__attribute__((visibility("hidden")))
volatile uint64_t poly_handler_x19_after;

static long poly_syscall6(long number, long arg0, long arg1, long arg2,
    long arg3, long arg4, long arg5) {
#if defined(__aarch64__)
  register long x0 __asm__("x0") = arg0;
  register long x1 __asm__("x1") = arg1;
  register long x2 __asm__("x2") = arg2;
  register long x3 __asm__("x3") = arg3;
  register long x4 __asm__("x4") = arg4;
  register long x5 __asm__("x5") = arg5;
  register long x8 __asm__("x8") = number;
  __asm__ volatile("svc #0"
      : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5),
        "+r"(x8)
      :
      : "x6", "x7", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
        "x16", "x17", "x18", "cc", "memory");
  return x0;
#elif defined(__riscv)
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a7 __asm__("a7") = number;
  __asm__ volatile("ecall"
      : "+r"(a0)
      : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7)
      : "memory");
  return a0;
#else
#error unsupported architecture
#endif
}

static long poly_syscall0(long number) {
  return poly_syscall6(number, 0, 0, 0, 0, 0, 0);
}

static long poly_syscall2(long number, long arg0, long arg1) {
  return poly_syscall6(number, arg0, arg1, 0, 0, 0, 0);
}

static long poly_syscall3(long number, long arg0, long arg1, long arg2) {
  return poly_syscall6(number, arg0, arg1, arg2, 0, 0, 0);
}

static long poly_syscall4(long number, long arg0, long arg1, long arg2,
    long arg3) {
  return poly_syscall6(number, arg0, arg1, arg2, arg3, 0, 0);
}

static long poly_strlen(const char *text) {
  long len = 0;
  while (text[len])
    len++;
  return len;
}

static char *poly_append_hex(char *out, uint64_t value) {
  static const char digits[] = "0123456789abcdef";
  for (int shift = 60; shift >= 0; shift -= 4)
    *out++ = digits[(value >> shift) & 0xf];
  return out;
}

static void poly_write_kill_failure(long value) {
  char line[] = "POLY_SIGNAL_HANDLER_KILL_FAIL result=0x0000000000000000\n";
  char *cursor = line + sizeof("POLY_SIGNAL_HANDLER_KILL_FAIL result=0x") - 1;
  cursor = poly_append_hex(cursor, (uint64_t) value);
  (void) cursor;
  (void) poly_syscall3(POLY_SYS_WRITE, 1, (long) line,
    (long) (sizeof(line) - 1));
}

void poly_rt_sigreturn_restorer(void);

void poly_sigusr1_handler(int signum);

__attribute__((visibility("hidden")))
uint64_t poly_process_main(uint64_t *initial_sp) {
  (void) initial_sp;

  uint64_t handler_addr = 0;
  uint64_t restorer_addr = 0;
#if defined(__aarch64__)
  __asm__ volatile(
    "adrp %0, poly_sigusr1_handler\n"
    "add %0, %0, :lo12:poly_sigusr1_handler\n"
    : "=r"(handler_addr));
  __asm__ volatile(
    "adrp %0, poly_rt_sigreturn_restorer\n"
    "add %0, %0, :lo12:poly_rt_sigreturn_restorer\n"
    : "=r"(restorer_addr));
#elif defined(__riscv)
  __asm__ volatile("lla %0, poly_sigusr1_handler" : "=r"(handler_addr));
  __asm__ volatile("lla %0, poly_rt_sigreturn_restorer" :
    "=r"(restorer_addr));
#else
#error unsupported architecture
#endif

  struct poly_linux_ksigaction action;
  action.handler = handler_addr;
  action.flags = 0;
  action.restorer = restorer_addr;
  action.mask = 0;
  if (poly_syscall4(POLY_SYS_RT_SIGACTION, POLY_SIGUSR1,
        (long) &action, 0, POLY_KERNEL_SIGSET_SIZE) != 0)
    return 10;

  const uint64_t x19_sentinel = 0x13579bdf2468ace0ULL;
#if defined(__aarch64__)
  __asm__ volatile("mov x19, %0" :: "r"(x19_sentinel) : "x19", "memory");
#elif defined(__riscv)
  __asm__ volatile("mv s3, %0" :: "r"(x19_sentinel) : "s3", "memory");
#else
#error unsupported architecture
#endif

  long pid = poly_syscall0(POLY_SYS_GETPID);
  if (pid <= 1)
    return 11;
  if (poly_syscall2(POLY_SYS_KILL, pid, POLY_SIGUSR1) != 0)
    return 12;

  uint64_t x19_restored = 0;
#if defined(__aarch64__)
  __asm__ volatile("mov %0, x19" : "=r"(x19_restored));
#elif defined(__riscv)
  __asm__ volatile("mv %0, s3" : "=r"(x19_restored));
#else
#error unsupported architecture
#endif
  if (poly_handler_count != 1)
    return 20;
  if (poly_handler_signum != POLY_SIGUSR1)
    return 21;
  if (poly_handler_x19_before == 0x2468ace02468ace0ULL)
    return 22;
  if (poly_handler_x19_after != 0x2468ace02468ace0ULL)
    return 23;
  if (x19_restored != poly_handler_x19_before)
    return 24;

  static const char marker[] =
    "POLY_SIGNAL_HANDLER_OK signum=10 count=1 x19=restored\n";
  if (poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
        poly_strlen(marker)) != (long) (sizeof(marker) - 1))
    return 25;

  const uint64_t sigusr1_mask = 1ULL << (POLY_SIGUSR1 - 1);
  if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_BLOCK,
        (long) &sigusr1_mask, 0, POLY_KERNEL_SIGSET_SIZE) != 0)
    return 26;
  long masked_pid = poly_syscall0(POLY_SYS_GETPID);
  if (masked_pid <= 1)
    return 26;
  long masked_kill_result =
    poly_syscall2(POLY_SYS_KILL, masked_pid, POLY_SIGUSR1);
  if (masked_kill_result != 0) {
    poly_write_kill_failure(masked_kill_result);
    return 27;
  }
  for (int iteration = 0; iteration < 4; iteration++) {
    if (poly_syscall0(POLY_SYS_GETPID) != masked_pid)
      return 28;
    if (poly_handler_count != 1)
      return 29;
  }
  if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_UNBLOCK,
        (long) &sigusr1_mask, 0, POLY_KERNEL_SIGSET_SIZE) != 0)
    return 30;
  if (poly_handler_count != 2)
    return 31;
  if (poly_handler_signum != POLY_SIGUSR1)
    return 32;

  static const char mask_marker[] =
    "POLY_SIGNAL_HANDLER_MASK_OK blocked=1 delivered_after_unblock=1\n";
  if (poly_syscall3(POLY_SYS_WRITE, 1, (long) mask_marker,
        poly_strlen(mask_marker)) != (long) (sizeof(mask_marker) - 1))
    return 33;
  return 42;
}

#if defined(__aarch64__)
__asm__(
  ".type poly_rt_sigreturn_restorer, %function\n"
  "poly_rt_sigreturn_restorer:\n"
  "mov x8, #139\n"
  "svc #0\n"
  "ret\n"
  ".size poly_rt_sigreturn_restorer, . - poly_rt_sigreturn_restorer\n"
  "\n"
  ".type poly_sigusr1_handler, %function\n"
  "poly_sigusr1_handler:\n"
  "adrp x9, poly_handler_signum\n"
  "str x0, [x9, :lo12:poly_handler_signum]\n"
  "adrp x9, poly_handler_x19_before\n"
  "str x19, [x9, :lo12:poly_handler_x19_before]\n"
  "movz x10, #0xace0\n"
  "movk x10, #0x2468, lsl #16\n"
  "movk x10, #0xace0, lsl #32\n"
  "movk x10, #0x2468, lsl #48\n"
  "mov x19, x10\n"
  "adrp x9, poly_handler_x19_after\n"
  "str x19, [x9, :lo12:poly_handler_x19_after]\n"
  "adrp x9, poly_handler_count\n"
  "ldr x10, [x9, :lo12:poly_handler_count]\n"
  "add x10, x10, #1\n"
  "str x10, [x9, :lo12:poly_handler_count]\n"
  "ret\n"
  ".size poly_sigusr1_handler, . - poly_sigusr1_handler\n"
  "\n"
  ".global _start\n"
  ".type _start, %function\n"
  "_start:\n"
  "mov x0, sp\n"
  "bl poly_process_main\n"
  "mov x8, #93\n"
  "svc #0\n");
#elif defined(__riscv)
__asm__(
  ".type poly_rt_sigreturn_restorer, @function\n"
  "poly_rt_sigreturn_restorer:\n"
  "li a7, 139\n"
  "ecall\n"
  "ret\n"
  ".size poly_rt_sigreturn_restorer, . - poly_rt_sigreturn_restorer\n"
  "\n"
  ".type poly_sigusr1_handler, @function\n"
  "poly_sigusr1_handler:\n"
  ".option push\n"
  ".option norelax\n"
  "lla t0, poly_handler_signum\n"
  "sd a0, 0(t0)\n"
  "lla t0, poly_handler_x19_before\n"
  "sd s3, 0(t0)\n"
  "li t1, 0x2468ace02468ace0\n"
  "mv s3, t1\n"
  "lla t0, poly_handler_x19_after\n"
  "sd s3, 0(t0)\n"
  "lla t0, poly_handler_count\n"
  "ld t1, 0(t0)\n"
  "addi t1, t1, 1\n"
  "sd t1, 0(t0)\n"
  ".option pop\n"
  "ret\n"
  ".size poly_sigusr1_handler, . - poly_sigusr1_handler\n"
  "\n"
  ".global _start\n"
  ".type _start, @function\n"
  "_start:\n"
  "mv a0, sp\n"
  "call poly_process_main\n"
  "li a7, 93\n"
  "ecall\n");
#else
#error unsupported architecture
#endif
