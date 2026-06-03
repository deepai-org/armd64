#include <stdint.h>

enum {
  POLY_SIG_BLOCK = 0,
  POLY_SIG_SETMASK = 2,
  POLY_SIGUSR1 = 10,
  POLY_KERNEL_SIGSET_SIZE = 8,

  POLY_SYS_READ = 63,
  POLY_SYS_WRITE = 64,
  POLY_SYS_SIGNALFD4 = 74,
  POLY_SYS_EXIT = 93,
  POLY_SYS_KILL = 129,
  POLY_SYS_RT_SIGPROCMASK = 135,
  POLY_SYS_RT_SIGPENDING = 136,
  POLY_SYS_GETPID = 172,
  POLY_SYS_CLOSE = 57,
};

struct poly_signalfd_siginfo {
  uint32_t signo;
  uint32_t errno_value;
  uint32_t code;
  uint32_t pid;
  uint32_t uid;
  uint32_t fd;
  uint32_t tid;
  uint32_t band;
  uint32_t overrun;
  uint32_t trapno;
  uint32_t status;
  uint32_t int_value;
  uint64_t ptr;
  uint64_t utime;
  uint64_t stime;
  uint64_t addr;
  uint16_t addr_lsb;
  uint8_t pad[46];
};

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
      : "+r"(x0)
      : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
      : "memory");
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

static int poly_streq(const char *left, const char *right) {
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == '\0' && *right == '\0';
}

static long poly_strlen(const char *text) {
  long len = 0;
  while (text[len])
    len++;
  return len;
}

__attribute__((visibility("hidden")))
uint64_t poly_process_main(uint64_t *initial_sp) {
  uint64_t argc = initial_sp[0];
  char **argv = (char **) &initial_sp[1];

  if (argc != 2)
    return 10 + argc;
  if (!argv[1] || !poly_streq(argv[1], "mask-edge"))
    return 20;

  long pid = poly_syscall0(POLY_SYS_GETPID);
  if (pid <= 1)
    return 21;

  const uint64_t sigusr1_mask = 1ULL << (POLY_SIGUSR1 - 1);
  uint64_t saved_mask = 0;
  if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_BLOCK, 0,
        (long) &saved_mask, POLY_KERNEL_SIGSET_SIZE) != 0)
    return 22;

  for (int iteration = 0; iteration < 8; iteration++) {
    uint64_t previous_mask = 0;
    if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_BLOCK,
          (long) &sigusr1_mask, (long) &previous_mask,
          POLY_KERNEL_SIGSET_SIZE) != 0)
      return 30 + iteration;

    uint64_t current_mask = 0;
    if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_BLOCK, 0,
          (long) &current_mask, POLY_KERNEL_SIGSET_SIZE) != 0)
      return 40 + iteration;
    if ((current_mask & sigusr1_mask) == 0)
      return 50 + iteration;

    uint64_t pending_mask = 0;
    if (poly_syscall2(POLY_SYS_RT_SIGPENDING, (long) &pending_mask,
          POLY_KERNEL_SIGSET_SIZE) != 0)
      return 60 + iteration;
    if ((pending_mask & sigusr1_mask) != 0)
      return 70 + iteration;

    if (poly_syscall2(POLY_SYS_KILL, pid, POLY_SIGUSR1) != 0)
      return 80 + iteration;

    pending_mask = 0;
    if (poly_syscall2(POLY_SYS_RT_SIGPENDING, (long) &pending_mask,
          POLY_KERNEL_SIGSET_SIZE) != 0)
      return 90 + iteration;
    if ((pending_mask & sigusr1_mask) == 0)
      return 100 + iteration;

    long fd = poly_syscall4(POLY_SYS_SIGNALFD4, -1, (long) &sigusr1_mask,
      POLY_KERNEL_SIGSET_SIZE, 0);
    if (fd < 0)
      return 110 + iteration;

    struct poly_signalfd_siginfo info;
    long got = poly_syscall3(POLY_SYS_READ, fd, (long) &info, sizeof(info));
    if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
      return 120 + iteration;
    if (got != (long) sizeof(info))
      return 130 + iteration;
    if (info.signo != POLY_SIGUSR1 || info.pid != (uint32_t) pid)
      return 140 + iteration;

    pending_mask = 0;
    if (poly_syscall2(POLY_SYS_RT_SIGPENDING, (long) &pending_mask,
          POLY_KERNEL_SIGSET_SIZE) != 0)
      return 150 + iteration;
    if ((pending_mask & sigusr1_mask) != 0)
      return 160 + iteration;

    if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_SETMASK,
          (long) &previous_mask, 0, POLY_KERNEL_SIGSET_SIZE) != 0)
      return 170 + iteration;

    current_mask = 0;
    if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_BLOCK, 0,
          (long) &current_mask, POLY_KERNEL_SIGSET_SIZE) != 0)
      return 180 + iteration;
    if (current_mask != previous_mask)
      return 190 + iteration;
  }

  if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_SETMASK,
        (long) &saved_mask, 0, POLY_KERNEL_SIGSET_SIZE) != 0)
    return 200;

  static const char marker[] = "POLY_SIGNAL_MASK_EDGE_OK iterations=8\n";
  if (poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
        poly_strlen(marker)) != (long) (sizeof(marker) - 1))
    return 201;
  return 42;
}

#if defined(__aarch64__)
__asm__(
  ".global _start\n"
  ".type _start, %function\n"
  "_start:\n"
  "mov x0, sp\n"
  "bl poly_process_main\n"
  "mov x8, #93\n"
  "svc #0\n");
#elif defined(__riscv)
__asm__(
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
