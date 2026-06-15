#include <stdarg.h>

#define POLY_AARCH64_NR_IOCTL 29
#define POLY_TIOCGWINSZ 0x5413UL

static long poly_aarch64_syscall3(long nr, long arg0, long arg1, long arg2) {
  register long x0 asm("x0") = arg0;
  register long x1 asm("x1") = arg1;
  register long x2 asm("x2") = arg2;
  register long x8 asm("x8") = nr;
  asm volatile("svc #0"
               : "+r"(x0)
               : "r"(x1), "r"(x2), "r"(x8)
               : "memory");
  return x0;
}

int isatty(int fd) {
  (void) fd;
  return 0;
}

int ioctl(int fd, unsigned long request, ...) {
  va_list ap;
  void *arg;

  va_start(ap, request);
  arg = va_arg(ap, void *);
  va_end(ap);

  if (request == POLY_TIOCGWINSZ)
    return -1;

  long status = poly_aarch64_syscall3(POLY_AARCH64_NR_IOCTL, fd,
    (long) request, (long) arg);
  return status < 0 ? -1 : (int) status;
}
