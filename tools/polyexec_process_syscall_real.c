#include <stdint.h>

enum {
  POLY_SYS_GETCWD = 17,
  POLY_SYS_WRITE = 64,
  POLY_SYS_EXIT = 93,
  POLY_SYS_GETPID = 172,
  POLY_SYS_GETPPID = 173,
  POLY_SYS_GETUID = 174,
  POLY_SYS_GETEUID = 175,
  POLY_SYS_GETGID = 176,
  POLY_SYS_GETEGID = 177,
  POLY_SYS_GETTID = 178
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

static int poly_streq(const char *left, const char *right) {
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == '\0' && *right == '\0';
}

uint64_t poly_process_main(uint64_t *initial_sp) {
  uint64_t argc = initial_sp[0];
  char **argv = (char **) &initial_sp[1];
  char cwd[128];

  if (argc != 2)
    return 10 + argc;
  if (!argv[1] || !poly_streq(argv[1], "probe"))
    return 20;

  long pid0 = poly_syscall0(POLY_SYS_GETPID);
  long pid1 = poly_syscall0(POLY_SYS_GETPID);
  if (pid0 <= 1 || pid0 != pid1)
    return 21;
  if (poly_syscall0(POLY_SYS_GETPPID) <= 0)
    return 22;
  if (poly_syscall0(POLY_SYS_GETTID) <= 0)
    return 23;
  if (poly_syscall0(POLY_SYS_GETUID) < 0)
    return 24;
  if (poly_syscall0(POLY_SYS_GETEUID) < 0)
    return 25;
  if (poly_syscall0(POLY_SYS_GETGID) < 0)
    return 26;
  if (poly_syscall0(POLY_SYS_GETEGID) < 0)
    return 27;

  long cwd_len = poly_syscall2(POLY_SYS_GETCWD, (long) cwd, sizeof(cwd));
  if (cwd_len <= 1 || cwd[0] != '/')
    return 28;

  static const char marker[] = "POLY_PROCESS_SYSCALL_OK\n";
  if (poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
        sizeof(marker) - 1) != (long) sizeof(marker) - 1)
    return 29;

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
