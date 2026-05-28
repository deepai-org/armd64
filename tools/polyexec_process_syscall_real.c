#include <stdint.h>

enum {
  POLY_PROT_READ = 1,
  POLY_PROT_WRITE = 2,
  POLY_MAP_PRIVATE = 2,
  POLY_MAP_ANONYMOUS = 0x20,
  POLY_S_IFMT = 0170000,
  POLY_S_IFREG = 0100000,

  POLY_SYS_GETCWD = 17,
  POLY_SYS_OPENAT = 56,
  POLY_SYS_CLOSE = 57,
  POLY_SYS_READ = 63,
  POLY_SYS_WRITE = 64,
  POLY_SYS_READV = 65,
  POLY_SYS_WRITEV = 66,
  POLY_SYS_NEWFSTATAT = 79,
  POLY_SYS_FSTAT = 80,
  POLY_SYS_EXIT = 93,
  POLY_SYS_GETPID = 172,
  POLY_SYS_GETPPID = 173,
  POLY_SYS_GETUID = 174,
  POLY_SYS_GETEUID = 175,
  POLY_SYS_GETGID = 176,
  POLY_SYS_GETEGID = 177,
  POLY_SYS_GETTID = 178,
  POLY_SYS_MUNMAP = 215,
  POLY_SYS_MMAP = 222,
  POLY_SYS_MPROTECT = 226
};

struct poly_linux_generic_stat {
  uint64_t dev;
  uint64_t ino;
  uint32_t mode;
  uint32_t nlink;
  uint32_t uid;
  uint32_t gid;
  uint64_t rdev;
  uint64_t pad1;
  int64_t size;
  int32_t blksize;
  int32_t pad2;
  int64_t blocks;
  int64_t atime_sec;
  uint64_t atime_nsec;
  int64_t mtime_sec;
  uint64_t mtime_nsec;
  int64_t ctime_sec;
  uint64_t ctime_nsec;
  uint32_t unused4;
  uint32_t unused5;
};

struct poly_iovec {
  uint64_t base;
  uint64_t len;
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

  char file_bytes[4];
  long fd = poly_syscall3(POLY_SYS_OPENAT, -100,
    (long) "/usr/bin/polyexec", 0);
  if (fd < 0)
    return 29;
  if (poly_syscall3(POLY_SYS_READ, fd, (long) file_bytes,
        sizeof(file_bytes)) != (long) sizeof(file_bytes)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 30;
  }
  struct poly_linux_generic_stat path_stat;
  struct poly_linux_generic_stat fd_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, -100,
        (long) "/usr/bin/polyexec", (long) &path_stat, 0) != 0)
    return 33;
  if ((path_stat.mode & POLY_S_IFMT) != POLY_S_IFREG ||
      path_stat.ino == 0 || path_stat.size <= 0)
    return 34;
  if (poly_syscall2(POLY_SYS_FSTAT, fd, (long) &fd_stat) != 0)
    return 35;
  if ((fd_stat.mode & POLY_S_IFMT) != POLY_S_IFREG ||
      fd_stat.ino != path_stat.ino || fd_stat.size != path_stat.size)
    return 36;
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 31;
  if (file_bytes[0] != 0x7f || file_bytes[1] != 'E' ||
      file_bytes[2] != 'L' || file_bytes[3] != 'F')
    return 32;

  char vec0[2];
  char vec1[2];
  struct poly_iovec read_iov[2] = {
    { (uint64_t) (uintptr_t) vec0, sizeof(vec0) },
    { (uint64_t) (uintptr_t) vec1, sizeof(vec1) }
  };
  fd = poly_syscall3(POLY_SYS_OPENAT, -100, (long) "/usr/bin/polyexec", 0);
  if (fd < 0)
    return 41;
  if (poly_syscall3(POLY_SYS_READV, fd, (long) read_iov, 2) != 4) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 43;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 44;
  if (vec0[0] != 0x7f || vec0[1] != 'E' || vec1[0] != 'L' ||
      vec1[1] != 'F')
    return 45;

  long page = poly_syscall6(POLY_SYS_MMAP, 0, 4096,
    POLY_PROT_READ | POLY_PROT_WRITE,
    POLY_MAP_PRIVATE | POLY_MAP_ANONYMOUS, -1, 0);
  if (page < 0)
    return 46;

  unsigned char *mapped = (unsigned char *) page;
  mapped[0] = 0x50;
  mapped[1] = 0x4f;
  mapped[2] = 0x4c;
  mapped[3] = 0x59;
  if (poly_syscall3(POLY_SYS_MPROTECT, page, 4096, POLY_PROT_READ) != 0) {
    poly_syscall2(POLY_SYS_MUNMAP, page, 4096);
    return 47;
  }
  if (mapped[0] != 0x50 || mapped[1] != 0x4f ||
      mapped[2] != 0x4c || mapped[3] != 0x59) {
    poly_syscall2(POLY_SYS_MUNMAP, page, 4096);
    return 48;
  }
  if (poly_syscall2(POLY_SYS_MUNMAP, page, 4096) != 0)
    return 49;

  static const char marker0[] = "POLY_PROCESS_";
  static const char marker1[] = "SYSCALL_OK\n";
  struct poly_iovec write_iov[2] = {
    { (uint64_t) (uintptr_t) marker0, sizeof(marker0) - 1 },
    { (uint64_t) (uintptr_t) marker1, sizeof(marker1) - 1 }
  };
  if (poly_syscall3(POLY_SYS_WRITEV, 1, (long) write_iov, 2) !=
      (long) (sizeof(marker0) + sizeof(marker1) - 2))
    return 50;

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
