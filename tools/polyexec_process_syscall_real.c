#include <stdint.h>

#define POLY_RLIM_INFINITY UINT64_MAX

enum {
  POLY_AF_UNIX = 1,
  POLY_CLOCK_MONOTONIC = 1,
  POLY_PROT_READ = 1,
  POLY_PROT_WRITE = 2,
  POLY_SOCK_STREAM = 1,
  POLY_GRND_NONBLOCK = 1,
  POLY_MFD_CLOEXEC = 1,
  POLY_MREMAP_MAYMOVE = 1,
  POLY_MAP_PRIVATE = 2,
  POLY_MAP_ANONYMOUS = 0x20,
  POLY_EPOLLIN = 1,
  POLY_EPOLL_CTL_ADD = 1,
  POLY_IN_OPEN = 0x20,
  POLY_POLLIN = 1,
  POLY_O_DIRECTORY = 0200000,
  POLY_O_CLOEXEC = 02000000,
  POLY_F_GETFD = 1,
  POLY_F_SETFD = 2,
  POLY_FD_CLOEXEC = 1,
  POLY_FUTEX_WAIT_PRIVATE = 128,
  POLY_FUTEX_WAKE_PRIVATE = 129,
  POLY_RLIMIT_STACK = 3,
  POLY_RUSAGE_SELF = 0,
  POLY_SIG_BLOCK = 0,
  POLY_SIG_SETMASK = 2,
  POLY_SIGUSR1 = 10,
  POLY_S_IFMT = 0170000,
  POLY_S_IFREG = 0100000,
  POLY_DIRENT64_NAME_OFFSET = 19,
  POLY_KERNEL_SIGSET_SIZE = 8,
  POLY_AT_FDCWD = -100,
  POLY_STATX_BASIC_STATS = 0x7ff,

  POLY_SYS_GETCWD = 17,
  POLY_SYS_EVENTFD2 = 19,
  POLY_SYS_EPOLL_CREATE1 = 20,
  POLY_SYS_EPOLL_CTL = 21,
  POLY_SYS_EPOLL_PWAIT = 22,
  POLY_SYS_DUP3 = 24,
  POLY_SYS_FCNTL = 25,
  POLY_SYS_INOTIFY_INIT1 = 26,
  POLY_SYS_INOTIFY_ADD_WATCH = 27,
  POLY_SYS_INOTIFY_RM_WATCH = 28,
  POLY_SYS_STATFS = 43,
  POLY_SYS_FSTATFS = 44,
  POLY_SYS_OPENAT = 56,
  POLY_SYS_CLOSE = 57,
  POLY_SYS_PIPE2 = 59,
  POLY_SYS_GETDENTS64 = 61,
  POLY_SYS_READ = 63,
  POLY_SYS_WRITE = 64,
  POLY_SYS_READV = 65,
  POLY_SYS_WRITEV = 66,
  POLY_SYS_PREAD64 = 67,
  POLY_SYS_PWRITE64 = 68,
  POLY_SYS_PSELECT6 = 72,
  POLY_SYS_PPOLL = 73,
  POLY_SYS_SIGNALFD4 = 74,
  POLY_SYS_READLINKAT = 78,
  POLY_SYS_NEWFSTATAT = 79,
  POLY_SYS_FSTAT = 80,
  POLY_SYS_TIMERFD_CREATE = 85,
  POLY_SYS_TIMERFD_SETTIME = 86,
  POLY_SYS_TIMERFD_GETTIME = 87,
  POLY_SYS_EXIT = 93,
  POLY_SYS_FUTEX = 98,
  POLY_SYS_SET_TID_ADDRESS = 96,
  POLY_SYS_SET_ROBUST_LIST = 99,
  POLY_SYS_GET_ROBUST_LIST = 100,
  POLY_SYS_CLOCK_GETTIME = 113,
  POLY_SYS_SCHED_GETAFFINITY = 123,
  POLY_SYS_KILL = 129,
  POLY_SYS_RT_SIGPROCMASK = 135,
  POLY_SYS_GETPID = 172,
  POLY_SYS_GETPPID = 173,
  POLY_SYS_GETUID = 174,
  POLY_SYS_GETEUID = 175,
  POLY_SYS_GETGID = 176,
  POLY_SYS_GETEGID = 177,
  POLY_SYS_GETTID = 178,
  POLY_SYS_SOCKETPAIR = 199,
  POLY_SYS_GETTIMEOFDAY = 169,
  POLY_SYS_UNAME = 160,
  POLY_SYS_GETRUSAGE = 165,
  POLY_SYS_SYSINFO = 179,
  POLY_SYS_BRK = 214,
  POLY_SYS_MUNMAP = 215,
  POLY_SYS_MREMAP = 216,
  POLY_SYS_MMAP = 222,
  POLY_SYS_MPROTECT = 226,
  POLY_SYS_PRLIMIT64 = 261,
  POLY_SYS_GETRANDOM = 278,
  POLY_SYS_MEMFD_CREATE = 279,
  POLY_SYS_STATX = 291,
  POLY_SYS_OPENAT2 = 437
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

struct poly_linux_generic_statfs {
  int64_t type;
  int64_t bsize;
  uint64_t blocks;
  uint64_t bfree;
  uint64_t bavail;
  uint64_t files;
  uint64_t ffree;
  int32_t fsid[2];
  int64_t namelen;
  int64_t frsize;
  int64_t flags;
  int64_t spare[4];
};

struct poly_iovec {
  uint64_t base;
  uint64_t len;
};

struct poly_pollfd {
  int32_t fd;
  int16_t events;
  int16_t revents;
};

struct poly_epoll_event {
  uint32_t events;
  uint32_t pad;
  uint64_t data;
};

struct poly_inotify_event {
  int32_t wd;
  uint32_t mask;
  uint32_t cookie;
  uint32_t len;
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

struct poly_open_how {
  uint64_t flags;
  uint64_t mode;
  uint64_t resolve;
};

struct poly_timespec {
  int64_t sec;
  int64_t nsec;
};

struct poly_itimerspec {
  struct poly_timespec interval;
  struct poly_timespec value;
};

struct poly_timeval {
  int64_t sec;
  int64_t usec;
};

struct poly_rlimit64 {
  uint64_t cur;
  uint64_t max;
};

struct poly_utsname {
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
  char domainname[65];
};

struct poly_rusage {
  struct poly_timeval utime;
  struct poly_timeval stime;
  int64_t maxrss;
  int64_t ixrss;
  int64_t idrss;
  int64_t isrss;
  int64_t minflt;
  int64_t majflt;
  int64_t nswap;
  int64_t inblock;
  int64_t oublock;
  int64_t msgsnd;
  int64_t msgrcv;
  int64_t nsignals;
  int64_t nvcsw;
  int64_t nivcsw;
};

struct poly_sysinfo {
  int64_t uptime;
  uint64_t loads[3];
  uint64_t totalram;
  uint64_t freeram;
  uint64_t sharedram;
  uint64_t bufferram;
  uint64_t totalswap;
  uint64_t freeswap;
  uint16_t procs;
  uint16_t pad;
  uint64_t totalhigh;
  uint64_t freehigh;
  uint32_t mem_unit;
};

struct poly_robust_list_head {
  uint64_t list_next;
  int64_t futex_offset;
  uint64_t list_op_pending;
};

struct poly_statx_timestamp {
  int64_t sec;
  uint32_t nsec;
  int32_t reserved;
};

struct poly_statx {
  uint32_t mask;
  uint32_t blksize;
  uint64_t attributes;
  uint32_t nlink;
  uint32_t uid;
  uint32_t gid;
  uint16_t mode;
  uint16_t reserved0;
  uint64_t ino;
  uint64_t size;
  uint64_t blocks;
  uint64_t attributes_mask;
  struct poly_statx_timestamp atime;
  struct poly_statx_timestamp btime;
  struct poly_statx_timestamp ctime;
  struct poly_statx_timestamp mtime;
  uint32_t rdev_major;
  uint32_t rdev_minor;
  uint32_t dev_major;
  uint32_t dev_minor;
  uint64_t mnt_id;
  uint32_t dio_mem_align;
  uint32_t dio_offset_align;
  uint64_t spare3[12];
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

static long poly_syscall5(long number, long arg0, long arg1, long arg2,
    long arg3, long arg4) {
  return poly_syscall6(number, arg0, arg1, arg2, arg3, arg4, 0);
}

static int poly_streq(const char *left, const char *right) {
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == '\0' && *right == '\0';
}

static int poly_contains_len(const char *text, long text_len,
    const char *needle) {
  if (!*needle)
    return 1;
  for (long offset = 0; offset < text_len; offset++) {
    long t = offset;
    const char *n = needle;
    while (t < text_len && *n && text[t] == *n) {
      t++;
      n++;
    }
    if (!*n)
      return 1;
  }
  return 0;
}

static int poly_dirents_contain(char *buffer, long length, const char *name) {
  long offset = 0;
  while (offset + POLY_DIRENT64_NAME_OFFSET < length) {
    uint16_t reclen = *(uint16_t *) (buffer + offset + 16);
    if (reclen <= POLY_DIRENT64_NAME_OFFSET || offset + reclen > length)
      return 0;
    if (poly_streq(buffer + offset + POLY_DIRENT64_NAME_OFFSET, name))
      return 1;
    offset += reclen;
  }
  return 0;
}

static int poly_any_byte_set(const unsigned char *bytes, long length) {
  for (long offset = 0; offset < length; offset++) {
    if (bytes[offset] != 0)
      return 1;
  }
  return 0;
}

__attribute__((visibility("hidden")))
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
  long tid = poly_syscall0(POLY_SYS_GETTID);
  if (tid <= 0)
    return 23;
  if (poly_syscall0(POLY_SYS_GETUID) < 0)
    return 24;
  if (poly_syscall0(POLY_SYS_GETEUID) < 0)
    return 25;
  if (poly_syscall0(POLY_SYS_GETGID) < 0)
    return 26;
  if (poly_syscall0(POLY_SYS_GETEGID) < 0)
    return 27;

  int clear_child_tid = 0;
  if (poly_syscall2(POLY_SYS_SET_TID_ADDRESS,
        (long) &clear_child_tid, 0) != tid)
    return 79;

  struct poly_robust_list_head robust_head;
  robust_head.list_next = (uint64_t) (uintptr_t) &robust_head;
  robust_head.futex_offset = 0;
  robust_head.list_op_pending = 0;
  if (poly_syscall2(POLY_SYS_SET_ROBUST_LIST,
        (long) &robust_head, sizeof(robust_head)) != 0)
    return 80;
  uint64_t robust_head_ptr = 0;
  uint64_t robust_len = 0;
  if (poly_syscall3(POLY_SYS_GET_ROBUST_LIST, 0,
        (long) &robust_head_ptr, (long) &robust_len) != 0)
    return 81;
  if (robust_head_ptr != (uint64_t) (uintptr_t) &robust_head ||
      robust_len != sizeof(robust_head))
    return 82;

  int futex_word = 1;
  struct poly_timespec futex_timeout;
  futex_timeout.sec = 0;
  futex_timeout.nsec = 0;
  if (poly_syscall6(POLY_SYS_FUTEX, (long) &futex_word,
        POLY_FUTEX_WAIT_PRIVATE, 2, (long) &futex_timeout, 0, 0) != -11)
    return 148;
  if (poly_syscall6(POLY_SYS_FUTEX, (long) &futex_word,
        POLY_FUTEX_WAIT_PRIVATE, 1, (long) &futex_timeout, 0, 0) != -110)
    return 149;
  if (poly_syscall6(POLY_SYS_FUTEX, (long) &futex_word,
        POLY_FUTEX_WAKE_PRIVATE, 1, 0, 0, 0) != 0)
    return 150;

  long cwd_len = poly_syscall2(POLY_SYS_GETCWD, (long) cwd, sizeof(cwd));
  if (cwd_len <= 1 || cwd[0] != '/')
    return 28;

  struct poly_timespec mono_time;
  if (poly_syscall2(POLY_SYS_CLOCK_GETTIME, 1, (long) &mono_time) != 0)
    return 51;
  if (mono_time.sec < 0 || mono_time.nsec < 0 ||
      mono_time.nsec >= 1000000000)
    return 52;

  struct poly_timeval wall_time;
  if (poly_syscall2(POLY_SYS_GETTIMEOFDAY, (long) &wall_time, 0) != 0)
    return 53;
  if (wall_time.sec <= 0 || wall_time.usec < 0 || wall_time.usec >= 1000000)
    return 54;

  struct poly_rlimit64 stack_limit;
  if (poly_syscall4(POLY_SYS_PRLIMIT64, 0, POLY_RLIMIT_STACK, 0,
        (long) &stack_limit) != 0)
    return 59;
  if (stack_limit.cur == 0 ||
      (stack_limit.max != POLY_RLIM_INFINITY &&
       stack_limit.max < stack_limit.cur))
    return 60;

  unsigned char random_bytes[16];
  if (poly_syscall3(POLY_SYS_GETRANDOM, 0, 0, 0) != 0)
    return 61;
  long random_len = poly_syscall3(POLY_SYS_GETRANDOM, (long) random_bytes,
    sizeof(random_bytes), POLY_GRND_NONBLOCK);
  if (random_len != (long) sizeof(random_bytes) && random_len != -11)
    return 62;

  struct poly_utsname uts;
  if (poly_syscall2(POLY_SYS_UNAME, (long) &uts, 0) != 0)
    return 63;
#if defined(__aarch64__)
  if (!poly_streq(uts.sysname, "Linux") || !poly_streq(uts.machine, "aarch64"))
    return 64;
#elif defined(__riscv)
  if (!poly_streq(uts.sysname, "Linux") || !poly_streq(uts.machine, "riscv64"))
    return 64;
#endif

  struct poly_rusage usage;
  if (poly_syscall2(POLY_SYS_GETRUSAGE, POLY_RUSAGE_SELF,
        (long) &usage) != 0)
    return 73;
  if (usage.utime.sec < 0 || usage.utime.usec < 0 ||
      usage.utime.usec >= 1000000 || usage.stime.sec < 0 ||
      usage.stime.usec < 0 || usage.stime.usec >= 1000000 ||
      usage.maxrss < 0 || usage.minflt < 0 || usage.majflt < 0)
    return 74;

  struct poly_sysinfo info;
  if (poly_syscall2(POLY_SYS_SYSINFO, (long) &info, 0) != 0)
    return 75;
  if (info.uptime < 0 || info.totalram == 0 || info.mem_unit == 0 ||
      info.procs == 0)
    return 76;

  long program_break = poly_syscall2(POLY_SYS_BRK, 0, 0);
  if (program_break <= 0)
    return 83;
  long grown_break = program_break + 4096;
  if (poly_syscall2(POLY_SYS_BRK, grown_break, 0) != grown_break)
    return 84;
  unsigned char *heap_byte = (unsigned char *) (uintptr_t) program_break;
  heap_byte[0] = 0xa5;
  heap_byte[4095] = 0x5a;
  if (heap_byte[0] != 0xa5 || heap_byte[4095] != 0x5a)
    return 85;
  if (poly_syscall2(POLY_SYS_BRK, program_break, 0) != program_break)
    return 86;

  unsigned char affinity[128];
  long affinity_len = poly_syscall3(POLY_SYS_SCHED_GETAFFINITY, 0,
    sizeof(affinity), (long) affinity);
  if (affinity_len <= 0 || affinity_len > (long) sizeof(affinity))
    return 77;
  if (!poly_any_byte_set(affinity, affinity_len))
    return 78;

  char exe_path[128];
  long exe_len = poly_syscall4(POLY_SYS_READLINKAT, POLY_AT_FDCWD,
    (long) "/proc/self/exe", (long) exe_path, sizeof(exe_path));
  if (exe_len <= 0 || exe_len >= (long) sizeof(exe_path))
    return 55;
  if (!poly_contains_len(exe_path, exe_len, "polyexec"))
    return 56;

  char file_bytes[4];
  long fd = poly_syscall3(POLY_SYS_OPENAT, POLY_AT_FDCWD,
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

  struct poly_linux_generic_statfs path_statfs;
  struct poly_linux_generic_statfs fd_statfs;
  if (poly_syscall2(POLY_SYS_STATFS, (long) "/usr/bin/polyexec",
        (long) &path_statfs) != 0)
    return 65;
  if (path_statfs.type == 0 || path_statfs.bsize <= 0 ||
      path_statfs.namelen <= 0)
    return 66;
  if (poly_syscall2(POLY_SYS_FSTATFS, fd, (long) &fd_statfs) != 0)
    return 67;
  if (fd_statfs.type != path_statfs.type ||
      fd_statfs.bsize != path_statfs.bsize ||
      fd_statfs.namelen != path_statfs.namelen)
    return 68;

  long fd_flags = poly_syscall3(POLY_SYS_FCNTL, fd, POLY_F_GETFD, 0);
  if (fd_flags < 0)
    return 91;
  if (poly_syscall3(POLY_SYS_FCNTL, fd, POLY_F_SETFD,
        fd_flags | POLY_FD_CLOEXEC) != 0)
    return 92;
  long cloexec_flags = poly_syscall3(POLY_SYS_FCNTL, fd, POLY_F_GETFD, 0);
  if ((cloexec_flags & POLY_FD_CLOEXEC) == 0)
    return 93;

  struct poly_statx statx_result;
  if (poly_syscall6(POLY_SYS_STATX, POLY_AT_FDCWD,
        (long) "/usr/bin/polyexec", 0, POLY_STATX_BASIC_STATS,
        (long) &statx_result, 0) != 0)
    return 57;
  if ((statx_result.mode & POLY_S_IFMT) != POLY_S_IFREG ||
      statx_result.ino != path_stat.ino ||
      (int64_t) statx_result.size != path_stat.size)
    return 58;

  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 31;
  if (file_bytes[0] != 0x7f || file_bytes[1] != 'E' ||
      file_bytes[2] != 'L' || file_bytes[3] != 'F')
    return 32;

  fd = poly_syscall3(POLY_SYS_OPENAT, POLY_AT_FDCWD,
    (long) "/usr/bin/polyexec", 0);
  if (fd < 0)
    return 105;
  char offset_bytes[3];
  if (poly_syscall4(POLY_SYS_PREAD64, fd, (long) offset_bytes,
        sizeof(offset_bytes), 1) != (long) sizeof(offset_bytes)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 106;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 107;
  if (offset_bytes[0] != 'E' || offset_bytes[1] != 'L' ||
      offset_bytes[2] != 'F')
    return 108;

  fd = poly_syscall2(POLY_SYS_MEMFD_CREATE, (long) "poly-pwrite",
    POLY_MFD_CLOEXEC);
  if (fd < 0)
    return 109;
  static const char write_offset_message[] = "OFFSET";
  if (poly_syscall4(POLY_SYS_PWRITE64, fd, (long) write_offset_message,
        sizeof(write_offset_message) - 1, 3) !=
      (long) (sizeof(write_offset_message) - 1)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 110;
  }
  char read_offset_message[6];
  if (poly_syscall4(POLY_SYS_PREAD64, fd, (long) read_offset_message,
        sizeof(read_offset_message), 3) !=
      (long) sizeof(read_offset_message)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 111;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 112;
  if (read_offset_message[0] != 'O' || read_offset_message[1] != 'F' ||
      read_offset_message[2] != 'F' || read_offset_message[3] != 'S' ||
      read_offset_message[4] != 'E' || read_offset_message[5] != 'T')
    return 113;

  struct poly_open_how open_how;
  open_how.flags = 0;
  open_how.mode = 0;
  open_how.resolve = 0;
  fd = poly_syscall4(POLY_SYS_OPENAT2, POLY_AT_FDCWD,
    (long) "/usr/bin/polyexec", (long) &open_how, sizeof(open_how));
  if (fd < 0)
    return 87;
  if (poly_syscall3(POLY_SYS_READ, fd, (long) file_bytes,
        sizeof(file_bytes)) != (long) sizeof(file_bytes)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 88;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 89;
  if (file_bytes[0] != 0x7f || file_bytes[1] != 'E' ||
      file_bytes[2] != 'L' || file_bytes[3] != 'F')
    return 90;

  int pipe_fds[2];
  if (poly_syscall2(POLY_SYS_PIPE2, (long) pipe_fds, 0) != 0)
    return 99;
  long dup_fd = poly_syscall3(POLY_SYS_DUP3, pipe_fds[0], 100,
    POLY_O_CLOEXEC);
  if (dup_fd != 100) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    return 100;
  }
  static const char pipe_message[] = "PX";
  char pipe_buffer[2];
  if (poly_syscall3(POLY_SYS_WRITE, pipe_fds[1],
        (long) pipe_message, sizeof(pipe_buffer)) !=
      (long) sizeof(pipe_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    poly_syscall2(POLY_SYS_CLOSE, dup_fd, 0);
    return 101;
  }
  if (poly_syscall3(POLY_SYS_READ, dup_fd, (long) pipe_buffer,
        sizeof(pipe_buffer)) != (long) sizeof(pipe_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    poly_syscall2(POLY_SYS_CLOSE, dup_fd, 0);
    return 102;
  }
  if (pipe_buffer[0] != 'P' || pipe_buffer[1] != 'X') {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    poly_syscall2(POLY_SYS_CLOSE, dup_fd, 0);
    return 103;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, dup_fd, 0) != 0)
    return 104;

  int socket_fds[2];
  if (poly_syscall4(POLY_SYS_SOCKETPAIR, POLY_AF_UNIX,
        POLY_SOCK_STREAM, 0, (long) socket_fds) != 0)
    return 151;
  static const char socket_message[] = "SOCK";
  char socket_buffer[4];
  if (poly_syscall3(POLY_SYS_WRITE, socket_fds[0],
        (long) socket_message, sizeof(socket_buffer)) !=
      (long) sizeof(socket_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 152;
  }
  if (poly_syscall3(POLY_SYS_READ, socket_fds[1], (long) socket_buffer,
        sizeof(socket_buffer)) != (long) sizeof(socket_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 153;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0) != 0)
    return 154;
  if (socket_buffer[0] != 'S' || socket_buffer[1] != 'O' ||
      socket_buffer[2] != 'C' || socket_buffer[3] != 'K')
    return 155;

  if (poly_syscall2(POLY_SYS_PIPE2, (long) pipe_fds, 0) != 0)
    return 156;
  static const char poll_message[] = "P";
  if (poly_syscall3(POLY_SYS_WRITE, pipe_fds[1],
        (long) poll_message, sizeof(poll_message) - 1) !=
      (long) sizeof(poll_message) - 1) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    return 157;
  }
  struct poly_pollfd poll_fd = { pipe_fds[0], POLY_POLLIN, 0 };
  struct poly_timespec poll_timeout = { 0, 0 };
  if (poly_syscall5(POLY_SYS_PPOLL, (long) &poll_fd, 1,
        (long) &poll_timeout, 0, 0) != 1) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    return 158;
  }
  if ((poll_fd.revents & POLY_POLLIN) == 0) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    return 159;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0) != 0)
    return 160;

  if (poly_syscall2(POLY_SYS_PIPE2, (long) pipe_fds, 0) != 0)
    return 161;
  static const char select_message[] = "S";
  if (poly_syscall3(POLY_SYS_WRITE, pipe_fds[1],
        (long) select_message, sizeof(select_message) - 1) !=
      (long) sizeof(select_message) - 1) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    return 162;
  }
  if (pipe_fds[0] < 0 || pipe_fds[0] >= 64) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    return 163;
  }
  uint64_t select_readfds = 1ull << (uint32_t) pipe_fds[0];
  struct poly_timespec select_timeout = { 0, 0 };
  if (poly_syscall6(POLY_SYS_PSELECT6, pipe_fds[0] + 1,
        (long) &select_readfds, 0, 0, (long) &select_timeout, 0) != 1) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    return 164;
  }
  if ((select_readfds & (1ull << (uint32_t) pipe_fds[0])) == 0) {
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0);
    return 165;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0) != 0)
    return 166;

  fd = poly_syscall2(POLY_SYS_EVENTFD2, 0, 0);
  if (fd < 0)
    return 114;
  uint64_t event_value = 7;
  if (poly_syscall3(POLY_SYS_WRITE, fd, (long) &event_value,
        sizeof(event_value)) != (long) sizeof(event_value)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 115;
  }
  event_value = 0;
  if (poly_syscall3(POLY_SYS_READ, fd, (long) &event_value,
        sizeof(event_value)) != (long) sizeof(event_value)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 116;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 117;
  if (event_value != 7)
    return 118;

  long event_fd = poly_syscall2(POLY_SYS_EVENTFD2, 0, 0);
  if (event_fd < 0)
    return 126;
  long epoll_fd = poly_syscall2(POLY_SYS_EPOLL_CREATE1, 0, 0);
  if (epoll_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, event_fd, 0);
    return 127;
  }
  struct poly_epoll_event event;
  event.events = POLY_EPOLLIN;
  event.pad = 0;
  event.data = 0x1122334455667788ULL;
  if (poly_syscall4(POLY_SYS_EPOLL_CTL, epoll_fd, POLY_EPOLL_CTL_ADD,
        event_fd, (long) &event) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, epoll_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, event_fd, 0);
    return 128;
  }
  event_value = 9;
  if (poly_syscall3(POLY_SYS_WRITE, event_fd, (long) &event_value,
        sizeof(event_value)) != (long) sizeof(event_value)) {
    poly_syscall2(POLY_SYS_CLOSE, epoll_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, event_fd, 0);
    return 129;
  }
  struct poly_epoll_event ready_event;
  ready_event.events = 0;
  ready_event.pad = 0;
  ready_event.data = 0;
  if (poly_syscall6(POLY_SYS_EPOLL_PWAIT, epoll_fd,
        (long) &ready_event, 1, 0, 0, 0) != 1) {
    poly_syscall2(POLY_SYS_CLOSE, epoll_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, event_fd, 0);
    return 130;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, epoll_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, event_fd, 0) != 0)
    return 131;
  if ((ready_event.events & POLY_EPOLLIN) == 0 ||
      ready_event.data != 0x1122334455667788ULL)
    return 132;

  long inotify_fd = poly_syscall2(POLY_SYS_INOTIFY_INIT1, 0, 0);
  if (inotify_fd < 0)
    return 133;
  long watch_id = poly_syscall3(POLY_SYS_INOTIFY_ADD_WATCH, inotify_fd,
    (long) "/usr/bin/polyexec", POLY_IN_OPEN);
  if (watch_id < 0) {
    poly_syscall2(POLY_SYS_CLOSE, inotify_fd, 0);
    return 134;
  }
  long watched_fd = poly_syscall3(POLY_SYS_OPENAT, POLY_AT_FDCWD,
    (long) "/usr/bin/polyexec", 0);
  if (watched_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, inotify_fd, 0);
    return 135;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, watched_fd, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, inotify_fd, 0);
    return 136;
  }
  struct poly_inotify_event notify_event;
  if (poly_syscall3(POLY_SYS_READ, inotify_fd, (long) &notify_event,
        sizeof(notify_event)) < (long) sizeof(notify_event)) {
    poly_syscall2(POLY_SYS_CLOSE, inotify_fd, 0);
    return 137;
  }
  if (poly_syscall2(POLY_SYS_INOTIFY_RM_WATCH, inotify_fd,
        watch_id) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, inotify_fd, 0);
    return 138;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, inotify_fd, 0) != 0)
    return 139;
  if (notify_event.wd != (int32_t) watch_id ||
      (notify_event.mask & POLY_IN_OPEN) == 0)
    return 140;

  uint64_t sigusr1_mask = 1ULL << (POLY_SIGUSR1 - 1);
  uint64_t old_signal_mask = 0;
  if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_BLOCK,
        (long) &sigusr1_mask, (long) &old_signal_mask,
        POLY_KERNEL_SIGSET_SIZE) != 0)
    return 141;
  fd = poly_syscall4(POLY_SYS_SIGNALFD4, -1, (long) &sigusr1_mask,
    POLY_KERNEL_SIGSET_SIZE, 0);
  if (fd < 0) {
    poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_SETMASK,
      (long) &old_signal_mask, 0, POLY_KERNEL_SIGSET_SIZE);
    return 142;
  }
  if (poly_syscall2(POLY_SYS_KILL, pid0, POLY_SIGUSR1) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 143;
  }
  struct poly_signalfd_siginfo signal_info;
  if (poly_syscall3(POLY_SYS_READ, fd, (long) &signal_info,
        sizeof(signal_info)) != (long) sizeof(signal_info)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 144;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 145;
  if (signal_info.signo != POLY_SIGUSR1 || signal_info.pid != (uint32_t) pid0)
    return 146;
  if (poly_syscall4(POLY_SYS_RT_SIGPROCMASK, POLY_SIG_SETMASK,
        (long) &old_signal_mask, 0, POLY_KERNEL_SIGSET_SIZE) != 0)
    return 147;

  fd = poly_syscall2(POLY_SYS_TIMERFD_CREATE, POLY_CLOCK_MONOTONIC, 0);
  if (fd < 0)
    return 119;
  struct poly_itimerspec timer_new;
  timer_new.interval.sec = 0;
  timer_new.interval.nsec = 0;
  timer_new.value.sec = 60;
  timer_new.value.nsec = 0;
  struct poly_itimerspec timer_old;
  if (poly_syscall4(POLY_SYS_TIMERFD_SETTIME, fd, 0, (long) &timer_new,
        (long) &timer_old) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 120;
  }
  struct poly_itimerspec timer_current;
  if (poly_syscall2(POLY_SYS_TIMERFD_GETTIME, fd,
        (long) &timer_current) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 121;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 122;
  if (timer_old.value.sec != 0 || timer_old.value.nsec != 0 ||
      timer_old.interval.sec != 0 || timer_old.interval.nsec != 0)
    return 123;
  if (timer_current.interval.sec != 0 || timer_current.interval.nsec != 0)
    return 124;
  if (timer_current.value.sec <= 0 || timer_current.value.sec > 60 ||
      timer_current.value.nsec < 0 || timer_current.value.nsec >= 1000000000)
    return 125;

  char dirents[4096];
  fd = poly_syscall3(POLY_SYS_OPENAT, POLY_AT_FDCWD, (long) "/usr/bin",
    POLY_O_DIRECTORY);
  if (fd < 0)
    return 69;
  int found_polyexec = 0;
  for (;;) {
    long dirent_len = poly_syscall3(POLY_SYS_GETDENTS64, fd, (long) dirents,
      sizeof(dirents));
    if (dirent_len < 0) {
      poly_syscall2(POLY_SYS_CLOSE, fd, 0);
      return 71;
    }
    if (dirent_len == 0)
      break;
    if (poly_dirents_contain(dirents, dirent_len, "polyexec")) {
      found_polyexec = 1;
      break;
    }
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 70;
  if (!found_polyexec)
    return 72;

  char vec0[2];
  char vec1[2];
  struct poly_iovec read_iov[2] = {
    { (uint64_t) (uintptr_t) vec0, sizeof(vec0) },
    { (uint64_t) (uintptr_t) vec1, sizeof(vec1) }
  };
  fd = poly_syscall3(POLY_SYS_OPENAT, POLY_AT_FDCWD,
    (long) "/usr/bin/polyexec", 0);
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

  page = poly_syscall6(POLY_SYS_MMAP, 0, 4096,
    POLY_PROT_READ | POLY_PROT_WRITE,
    POLY_MAP_PRIVATE | POLY_MAP_ANONYMOUS, -1, 0);
  if (page < 0)
    return 94;
  mapped = (unsigned char *) page;
  mapped[0] = 0x6d;
  mapped[4095] = 0x72;
  long remapped_page = poly_syscall6(POLY_SYS_MREMAP, page, 4096, 8192,
    POLY_MREMAP_MAYMOVE, 0, 0);
  if (remapped_page < 0) {
    poly_syscall2(POLY_SYS_MUNMAP, page, 4096);
    return 95;
  }
  mapped = (unsigned char *) remapped_page;
  if (mapped[0] != 0x6d || mapped[4095] != 0x72) {
    poly_syscall2(POLY_SYS_MUNMAP, remapped_page, 8192);
    return 96;
  }
  mapped[8191] = 0x21;
  if (mapped[8191] != 0x21) {
    poly_syscall2(POLY_SYS_MUNMAP, remapped_page, 8192);
    return 97;
  }
  if (poly_syscall2(POLY_SYS_MUNMAP, remapped_page, 8192) != 0)
    return 98;

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
