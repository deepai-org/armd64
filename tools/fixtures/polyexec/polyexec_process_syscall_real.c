#include <stdint.h>

#define POLY_RLIM_INFINITY UINT64_MAX

enum {
  POLY_AF_UNIX = 1,
  POLY_CLOCK_MONOTONIC = 1,
  POLY_SOL_SOCKET = 1,
  POLY_PROT_READ = 1,
  POLY_PROT_WRITE = 2,
  POLY_SO_REUSEADDR = 2,
  POLY_RENAME_NOREPLACE = 1,
  POLY_SHUT_WR = 1,
  POLY_SOCK_CLOEXEC = 02000000,
  POLY_SOCK_DGRAM = 2,
  POLY_SOCK_STREAM = 1,
  POLY_GRND_NONBLOCK = 1,
  POLY_MFD_CLOEXEC = 1,
  POLY_POSIX_FADV_NORMAL = 0,
  POLY_MADV_NORMAL = 0,
  POLY_MREMAP_MAYMOVE = 1,
  POLY_MAP_PRIVATE = 2,
  POLY_MAP_ANONYMOUS = 0x20,
  POLY_EPOLLIN = 1,
  POLY_EPOLL_CTL_ADD = 1,
  POLY_IN_OPEN = 0x20,
  POLY_POLLIN = 1,
  POLY_O_CREAT = 0100,
  POLY_O_RDWR = 02,
  POLY_O_TRUNC = 01000,
  POLY_O_DIRECTORY = 0200000,
  POLY_O_CLOEXEC = 02000000,
  POLY_F_GETFD = 1,
  POLY_F_SETFD = 2,
  POLY_FD_CLOEXEC = 1,
  POLY_LOCK_EX = 2,
  POLY_LOCK_NB = 4,
  POLY_LOCK_UN = 8,
  POLY_FUTEX_32 = 2,
  POLY_FUTEX_WAIT_PRIVATE = 128,
  POLY_FUTEX_WAKE_PRIVATE = 129,
  POLY_PRIO_PROCESS = 0,
  POLY_IOPRIO_WHO_PROCESS = 1,
  POLY_RLIMIT_STACK = 3,
  POLY_RUSAGE_SELF = 0,
  POLY_SCHED_OTHER = 0,
  POLY_MEMBARRIER_CMD_QUERY = 0,
  POLY_SIG_BLOCK = 0,
  POLY_SIG_SETMASK = 2,
  POLY_SIGUSR1 = 10,
  POLY_PR_SET_NAME = 15,
  POLY_PR_GET_NAME = 16,
  POLY_PERSONALITY_QUERY = 0xffffffffUL,
  POLY_S_IFMT = 0170000,
  POLY_S_IFDIR = 0040000,
  POLY_S_IFIFO = 0010000,
  POLY_S_IFREG = 0100000,
  POLY_S_IFLNK = 0120000,
  POLY_DIRENT64_NAME_OFFSET = 19,
  POLY_KERNEL_SIGSET_SIZE = 8,
  POLY_AT_FDCWD = -100,
  POLY_AT_SYMLINK_NOFOLLOW = 0x100,
  POLY_AT_REMOVEDIR = 0x200,
  POLY_STATX_BASIC_STATS = 0x7ff,

  POLY_SYS_SETXATTR = 5,
  POLY_SYS_FSETXATTR = 7,
  POLY_SYS_GETXATTR = 8,
  POLY_SYS_FGETXATTR = 10,
  POLY_SYS_LISTXATTR = 11,
  POLY_SYS_FLISTXATTR = 13,
  POLY_SYS_REMOVEXATTR = 14,
  POLY_SYS_FREMOVEXATTR = 16,
  POLY_SYS_GETCWD = 17,
  POLY_SYS_EVENTFD2 = 19,
  POLY_SYS_EPOLL_CREATE1 = 20,
  POLY_SYS_EPOLL_CTL = 21,
  POLY_SYS_EPOLL_PWAIT = 22,
  POLY_SYS_EPOLL_PWAIT2 = 441,
  POLY_SYS_DUP3 = 24,
  POLY_SYS_FCNTL = 25,
  POLY_SYS_INOTIFY_INIT1 = 26,
  POLY_SYS_INOTIFY_ADD_WATCH = 27,
  POLY_SYS_INOTIFY_RM_WATCH = 28,
  POLY_SYS_IOPRIO_GET = 31,
  POLY_SYS_FLOCK = 32,
  POLY_SYS_MKNODAT = 33,
  POLY_SYS_MKDIRAT = 34,
  POLY_SYS_UNLINKAT = 35,
  POLY_SYS_SYMLINKAT = 36,
  POLY_SYS_LINKAT = 37,
  POLY_SYS_RENAMEAT = 38,
  POLY_SYS_STATFS = 43,
  POLY_SYS_FSTATFS = 44,
  POLY_SYS_TRUNCATE = 45,
  POLY_SYS_FTRUNCATE = 46,
  POLY_SYS_FALLOCATE = 47,
  POLY_SYS_FACCESSAT = 48,
  POLY_SYS_CHDIR = 49,
  POLY_SYS_FCHDIR = 50,
  POLY_SYS_FCHMOD = 52,
  POLY_SYS_FCHMODAT = 53,
  POLY_SYS_FCHOWNAT = 54,
  POLY_SYS_FCHOWN = 55,
  POLY_SYS_OPENAT = 56,
  POLY_SYS_CLOSE = 57,
  POLY_SYS_PIPE2 = 59,
  POLY_SYS_GETDENTS64 = 61,
  POLY_SYS_LSEEK = 62,
  POLY_SYS_READ = 63,
  POLY_SYS_WRITE = 64,
  POLY_SYS_READV = 65,
  POLY_SYS_WRITEV = 66,
  POLY_SYS_PREAD64 = 67,
  POLY_SYS_PWRITE64 = 68,
  POLY_SYS_PREADV = 69,
  POLY_SYS_PWRITEV = 70,
  POLY_SYS_PSELECT6 = 72,
  POLY_SYS_PPOLL = 73,
  POLY_SYS_SIGNALFD4 = 74,
  POLY_SYS_READLINKAT = 78,
  POLY_SYS_NEWFSTATAT = 79,
  POLY_SYS_FSTAT = 80,
  POLY_SYS_SYNC = 81,
  POLY_SYS_FSYNC = 82,
  POLY_SYS_FDATASYNC = 83,
  POLY_SYS_SYNC_FILE_RANGE = 84,
  POLY_SYS_TIMERFD_CREATE = 85,
  POLY_SYS_TIMERFD_SETTIME = 86,
  POLY_SYS_TIMERFD_GETTIME = 87,
  POLY_SYS_PERSONALITY = 92,
  POLY_SYS_EXIT = 93,
  POLY_SYS_FUTEX = 98,
  POLY_SYS_SET_TID_ADDRESS = 96,
  POLY_SYS_SET_ROBUST_LIST = 99,
  POLY_SYS_GET_ROBUST_LIST = 100,
  POLY_SYS_NANOSLEEP = 101,
  POLY_SYS_GETITIMER = 102,
  POLY_SYS_SETITIMER = 103,
  POLY_SYS_CLOCK_GETTIME = 113,
  POLY_SYS_CLOCK_GETRES = 114,
  POLY_SYS_CLOCK_NANOSLEEP = 115,
  POLY_SYS_SCHED_GETSCHEDULER = 120,
  POLY_SYS_SCHED_GETPARAM = 121,
  POLY_SYS_SCHED_GETAFFINITY = 123,
  POLY_SYS_SCHED_YIELD = 124,
  POLY_SYS_SCHED_GET_PRIORITY_MAX = 125,
  POLY_SYS_SCHED_GET_PRIORITY_MIN = 126,
  POLY_SYS_KILL = 129,
  POLY_SYS_RT_SIGPROCMASK = 135,
  POLY_SYS_GETPRIORITY = 141,
  POLY_SYS_GETRESUID = 148,
  POLY_SYS_GETRESGID = 150,
  POLY_SYS_TIMES = 153,
  POLY_SYS_GETPGID = 155,
  POLY_SYS_GETSID = 156,
  POLY_SYS_GETGROUPS = 158,
  POLY_SYS_GETRLIMIT = 163,
  POLY_SYS_UMASK = 166,
  POLY_SYS_PRCTL = 167,
  POLY_SYS_GETCPU = 168,
  POLY_SYS_GETPID = 172,
  POLY_SYS_GETPPID = 173,
  POLY_SYS_GETUID = 174,
  POLY_SYS_GETEUID = 175,
  POLY_SYS_GETGID = 176,
  POLY_SYS_GETEGID = 177,
  POLY_SYS_GETTID = 178,
  POLY_SYS_SOCKET = 198,
  POLY_SYS_SOCKETPAIR = 199,
  POLY_SYS_BIND = 200,
  POLY_SYS_LISTEN = 201,
  POLY_SYS_ACCEPT = 202,
  POLY_SYS_CONNECT = 203,
  POLY_SYS_GETSOCKNAME = 204,
  POLY_SYS_GETPEERNAME = 205,
  POLY_SYS_SENDTO = 206,
  POLY_SYS_RECVFROM = 207,
  POLY_SYS_SETSOCKOPT = 208,
  POLY_SYS_GETSOCKOPT = 209,
  POLY_SYS_SHUTDOWN = 210,
  POLY_SYS_SENDMSG = 211,
  POLY_SYS_RECVMSG = 212,
  POLY_SYS_RECVMMSG = 243,
  POLY_SYS_SENDMMSG = 269,
  POLY_SYS_ACCEPT4 = 242,
  POLY_SYS_RENAMEAT2 = 276,
  POLY_SYS_PIDFD_SEND_SIGNAL = 424,
  POLY_SYS_GETTIMEOFDAY = 169,
  POLY_SYS_UNAME = 160,
  POLY_SYS_GETRUSAGE = 165,
  POLY_SYS_SYSINFO = 179,
  POLY_SYS_BRK = 214,
  POLY_SYS_MUNMAP = 215,
  POLY_SYS_MREMAP = 216,
  POLY_SYS_MMAP = 222,
  POLY_SYS_FADVISE64 = 223,
  POLY_SYS_MPROTECT = 226,
  POLY_SYS_MLOCK = 228,
  POLY_SYS_MUNLOCK = 229,
  POLY_SYS_MADVISE = 233,
  POLY_SYS_PRLIMIT64 = 261,
  POLY_SYS_GETRANDOM = 278,
  POLY_SYS_MEMFD_CREATE = 279,
  POLY_SYS_MEMBARRIER = 283,
  POLY_SYS_MLOCK2 = 284,
  POLY_SYS_STATX = 291,
  POLY_SYS_PIDFD_OPEN = 434,
  POLY_SYS_CLOSE_RANGE = 436,
  POLY_SYS_OPENAT2 = 437,
  POLY_SYS_PIDFD_GETFD = 438,
  POLY_SYS_FACCESSAT2 = 439,
  POLY_SYS_FUTEX_WAITV = 449,
  POLY_SYS_FCHMODAT2 = 452,
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

struct poly_futex_waitv {
  uint64_t val;
  uint64_t uaddr;
  uint32_t flags;
  uint32_t reserved;
};

struct poly_sockaddr_un {
  uint16_t family;
  char path[108];
};

struct poly_msghdr {
  uint64_t name;
  uint32_t namelen;
  uint32_t pad0;
  uint64_t iov;
  uint64_t iovlen;
  uint64_t control;
  uint64_t controllen;
  int32_t flags;
  int32_t pad1;
};

struct poly_mmsghdr {
  struct poly_msghdr hdr;
  uint32_t len;
  uint32_t pad;
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

struct poly_itimerval {
  struct poly_timeval interval;
  struct poly_timeval value;
};

struct poly_tms {
  int64_t utime;
  int64_t stime;
  int64_t cutime;
  int64_t cstime;
};

struct poly_sched_param {
  int32_t priority;
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
  long uid = poly_syscall0(POLY_SYS_GETUID);
  if (uid < 0)
    return 24;
  long euid = poly_syscall0(POLY_SYS_GETEUID);
  if (euid < 0)
    return 25;
  long gid = poly_syscall0(POLY_SYS_GETGID);
  if (gid < 0)
    return 26;
  long egid = poly_syscall0(POLY_SYS_GETEGID);
  if (egid < 0)
    return 27;
  if (poly_syscall2(POLY_SYS_GETPGID, 0, 0) < 0)
    return 229;
  if (poly_syscall2(POLY_SYS_GETSID, 0, 0) < 0)
    return 230;
  uint32_t resuid[3];
  if (poly_syscall3(POLY_SYS_GETRESUID, (long) &resuid[0],
        (long) &resuid[1], (long) &resuid[2]) != 0)
    return 231;
  if (resuid[0] != (uint32_t) uid || resuid[1] != (uint32_t) euid)
    return 232;
  uint32_t resgid[3];
  if (poly_syscall3(POLY_SYS_GETRESGID, (long) &resgid[0],
        (long) &resgid[1], (long) &resgid[2]) != 0)
    return 233;
  if (resgid[0] != (uint32_t) gid || resgid[1] != (uint32_t) egid)
    return 234;
  if (poly_syscall2(POLY_SYS_GETGROUPS, 0, 0) < 0)
    return 235;
  if (poly_syscall2(POLY_SYS_GETPRIORITY, POLY_PRIO_PROCESS, 0) < 0)
    return 236;
  if (poly_syscall2(POLY_SYS_IOPRIO_GET, POLY_IOPRIO_WHO_PROCESS, 0) < 0)
    return 354;
  if (poly_syscall2(POLY_SYS_PERSONALITY, POLY_PERSONALITY_QUERY, 0) < 0)
    return 345;
  struct poly_sched_param sched_param;
  if (poly_syscall2(POLY_SYS_SCHED_GETSCHEDULER, 0, 0) < 0)
    return 346;
  if (poly_syscall2(POLY_SYS_SCHED_GETPARAM, 0,
        (long) &sched_param) != 0)
    return 347;
  if (sched_param.priority < 0)
    return 348;
  if (poly_syscall2(POLY_SYS_SCHED_GET_PRIORITY_MAX,
        POLY_SCHED_OTHER, 0) < 0)
    return 349;
  if (poly_syscall2(POLY_SYS_SCHED_GET_PRIORITY_MIN,
        POLY_SCHED_OTHER, 0) < 0)
    return 350;
  if (poly_syscall5(POLY_SYS_PRCTL, POLY_PR_SET_NAME,
        (long) "poly-proc", 0, 0, 0) != 0)
    return 351;
  char task_name[16];
  for (int n = 0; n < 16; n++)
    task_name[n] = 0;
  if (poly_syscall5(POLY_SYS_PRCTL, POLY_PR_GET_NAME,
        (long) task_name, 0, 0, 0) != 0)
    return 352;
  if (!poly_streq(task_name, "poly-proc"))
    return 353;
  uint32_t cpu = UINT32_MAX;
  uint32_t node = UINT32_MAX;
  if (poly_syscall3(POLY_SYS_GETCPU, (long) &cpu, (long) &node, 0) != 0)
    return 237;
  if (cpu == UINT32_MAX)
    return 238;
  struct poly_tms tms;
  if (poly_syscall2(POLY_SYS_TIMES, (long) &tms, 0) < 0)
    return 239;
  if (tms.utime < 0 || tms.stime < 0 || tms.cutime < 0 || tms.cstime < 0)
    return 240;
  long old_umask = poly_syscall2(POLY_SYS_UMASK, 077, 0);
  if (old_umask < 0)
    return 241;
  if (poly_syscall2(POLY_SYS_UMASK, old_umask, 0) != 077)
    return 242;

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
  struct poly_futex_waitv futex_waiters[1];
  futex_waiters[0].val = 2;
  futex_waiters[0].uaddr = (uint64_t) (uintptr_t) &futex_word;
  futex_waiters[0].flags = POLY_FUTEX_32;
  futex_waiters[0].reserved = 0;
  if (poly_syscall5(POLY_SYS_FUTEX_WAITV, (long) futex_waiters, 1, 0,
        (long) &futex_timeout, POLY_CLOCK_MONOTONIC) != -11)
    return 291;

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

  struct poly_timespec zero_sleep;
  zero_sleep.sec = 0;
  zero_sleep.nsec = 0;
  if (poly_syscall2(POLY_SYS_NANOSLEEP, (long) &zero_sleep, 0) != 0)
    return 221;
  struct poly_timespec clock_res;
  if (poly_syscall2(POLY_SYS_CLOCK_GETRES, POLY_CLOCK_MONOTONIC,
        (long) &clock_res) != 0)
    return 222;
  if (clock_res.sec < 0 || clock_res.nsec <= 0 ||
      clock_res.nsec >= 1000000000)
    return 223;
  if (poly_syscall4(POLY_SYS_CLOCK_NANOSLEEP, POLY_CLOCK_MONOTONIC, 0,
        (long) &zero_sleep, 0) != 0)
    return 224;
  struct poly_itimerval current_timer;
  if (poly_syscall2(POLY_SYS_GETITIMER, 0, (long) &current_timer) != 0)
    return 225;
  struct poly_itimerval disabled_timer;
  disabled_timer.interval.sec = 0;
  disabled_timer.interval.usec = 0;
  disabled_timer.value.sec = 0;
  disabled_timer.value.usec = 0;
  struct poly_itimerval old_timer;
  if (poly_syscall3(POLY_SYS_SETITIMER, 0, (long) &disabled_timer,
        (long) &old_timer) != 0)
    return 226;
  if (old_timer.interval.usec < 0 || old_timer.interval.usec >= 1000000 ||
      old_timer.value.usec < 0 || old_timer.value.usec >= 1000000)
    return 227;
  if (poly_syscall0(POLY_SYS_SCHED_YIELD) != 0)
    return 228;

  struct poly_rlimit64 getrlimit_stack;
  if (poly_syscall2(POLY_SYS_GETRLIMIT, POLY_RLIMIT_STACK,
        (long) &getrlimit_stack) != 0)
    return 355;
  if (getrlimit_stack.cur == 0 ||
      (getrlimit_stack.max != POLY_RLIM_INFINITY &&
       getrlimit_stack.max < getrlimit_stack.cur))
    return 356;
  struct poly_rlimit64 stack_limit;
  if (poly_syscall4(POLY_SYS_PRLIMIT64, 0, POLY_RLIMIT_STACK, 0,
        (long) &stack_limit) != 0)
    return 59;
  if (stack_limit.cur == 0 ||
      (stack_limit.max != POLY_RLIM_INFINITY &&
       stack_limit.max < stack_limit.cur))
    return 60;
  if (stack_limit.cur != getrlimit_stack.cur ||
      stack_limit.max != getrlimit_stack.max)
    return 357;

  unsigned char random_bytes[16];
  if (poly_syscall3(POLY_SYS_GETRANDOM, 0, 0, 0) != 0)
    return 61;
  long random_len = poly_syscall3(POLY_SYS_GETRANDOM, (long) random_bytes,
    sizeof(random_bytes), POLY_GRND_NONBLOCK);
  if (random_len != (long) sizeof(random_bytes) && random_len != -11)
    return 62;
  if (poly_syscall2(POLY_SYS_MEMBARRIER, POLY_MEMBARRIER_CMD_QUERY, 0) < 0)
    return 358;

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

  if (poly_syscall4(POLY_SYS_FACCESSAT, POLY_AT_FDCWD,
        (long) "/usr/bin/polyexec", 0, 0) != 0)
    return 344;
  if (poly_syscall4(POLY_SYS_FACCESSAT2, POLY_AT_FDCWD,
        (long) "/usr/bin/polyexec", 0, 0) != 0)
    return 307;
  if (poly_syscall4(POLY_SYS_FCHMODAT2, POLY_AT_FDCWD,
        (long) "user.poly", 0600, 0) != 0)
    return 308;

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

  fd = poly_syscall2(POLY_SYS_MEMFD_CREATE, (long) "poly-vectored",
    POLY_MFD_CLOEXEC);
  if (fd < 0)
    return 208;
  if (poly_syscall2(POLY_SYS_FLOCK, fd, POLY_LOCK_EX | POLY_LOCK_NB) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 305;
  }
  if (poly_syscall2(POLY_SYS_FLOCK, fd, POLY_LOCK_UN) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 306;
  }
  if (poly_syscall4(POLY_SYS_FALLOCATE, fd, 0, 0, 4096) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 209;
  }
  if (poly_syscall2(POLY_SYS_FTRUNCATE, fd, 8192) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 210;
  }
  static const char vec_write0[] = "V0";
  static const char vec_write1[] = "V1";
  struct poly_iovec write_offset_iov[2] = {
    { (uint64_t) (uintptr_t) vec_write0, sizeof(vec_write0) - 1 },
    { (uint64_t) (uintptr_t) vec_write1, sizeof(vec_write1) - 1 }
  };
  if (poly_syscall4(POLY_SYS_PWRITEV, fd, (long) write_offset_iov, 2,
        128) != 4) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 211;
  }
  if (poly_syscall4(POLY_SYS_SYNC_FILE_RANGE, fd, 128, 4, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 212;
  }
  if (poly_syscall2(POLY_SYS_FDATASYNC, fd, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 213;
  }
  if (poly_syscall2(POLY_SYS_FSYNC, fd, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 214;
  }
  char vec_read0[2];
  char vec_read1[2];
  struct poly_iovec read_offset_iov[2] = {
    { (uint64_t) (uintptr_t) vec_read0, sizeof(vec_read0) },
    { (uint64_t) (uintptr_t) vec_read1, sizeof(vec_read1) }
  };
  if (poly_syscall4(POLY_SYS_PREADV, fd, (long) read_offset_iov, 2,
        128) != 4) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 215;
  }
  if (poly_syscall3(POLY_SYS_LSEEK, fd, 128, 0) != 128) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 216;
  }
  char seek_bytes[4];
  if (poly_syscall3(POLY_SYS_READ, fd, (long) seek_bytes,
        sizeof(seek_bytes)) != 4) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 217;
  }
  if (poly_syscall2(POLY_SYS_SYNC, 0, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 218;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 219;
  if (vec_read0[0] != 'V' || vec_read0[1] != '0' ||
      vec_read1[0] != 'V' || vec_read1[1] != '1' ||
      seek_bytes[0] != 'V' || seek_bytes[1] != '0' ||
      seek_bytes[2] != 'V' || seek_bytes[3] != '1')
    return 220;

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

  long pid_fd = poly_syscall2(POLY_SYS_PIDFD_OPEN, pid0, 0);
  if (pid_fd < 0)
    return 277;
  long pidfd_source_fd = poly_syscall3(POLY_SYS_OPENAT, POLY_AT_FDCWD,
    (long) "/usr/bin/polyexec", 0);
  if (pidfd_source_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, pid_fd, 0);
    return 278;
  }
  long pidfd_dup_fd = poly_syscall3(POLY_SYS_PIDFD_GETFD, pid_fd,
    pidfd_source_fd, 0);
  if (pidfd_dup_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, pidfd_source_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, pid_fd, 0);
    return 287;
  }
  char pidfd_file_bytes[4];
  if (poly_syscall3(POLY_SYS_READ, pidfd_dup_fd,
        (long) pidfd_file_bytes, sizeof(pidfd_file_bytes)) !=
      (long) sizeof(pidfd_file_bytes)) {
    poly_syscall2(POLY_SYS_CLOSE, pidfd_dup_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, pidfd_source_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, pid_fd, 0);
    return 288;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, pidfd_dup_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, pidfd_source_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, pid_fd, 0) != 0)
    return 289;
  if (pidfd_file_bytes[0] != 0x7f || pidfd_file_bytes[1] != 'E' ||
      pidfd_file_bytes[2] != 'L' || pidfd_file_bytes[3] != 'F')
    return 290;

  if (poly_syscall2(POLY_SYS_PIPE2, (long) pipe_fds, 0) != 0)
    return 279;
  long close_range_fd0 = poly_syscall3(POLY_SYS_DUP3, pipe_fds[0], 101, 0);
  long close_range_fd1 = poly_syscall3(POLY_SYS_DUP3, pipe_fds[1], 102, 0);
  if (poly_syscall2(POLY_SYS_CLOSE, pipe_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, pipe_fds[1], 0) != 0 ||
      close_range_fd0 != 101 || close_range_fd1 != 102) {
    if (close_range_fd0 == 101)
      poly_syscall2(POLY_SYS_CLOSE, close_range_fd0, 0);
    if (close_range_fd1 == 102)
      poly_syscall2(POLY_SYS_CLOSE, close_range_fd1, 0);
    return 280;
  }
  if (poly_syscall3(POLY_SYS_CLOSE_RANGE, close_range_fd0,
        close_range_fd1, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, close_range_fd0, 0);
    poly_syscall2(POLY_SYS_CLOSE, close_range_fd1, 0);
    return 281;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, close_range_fd0, 0) != -9 ||
      poly_syscall2(POLY_SYS_CLOSE, close_range_fd1, 0) != -9)
    return 282;

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

  if (poly_syscall4(POLY_SYS_SOCKETPAIR, POLY_AF_UNIX,
        POLY_SOCK_STREAM, 0, (long) socket_fds) != 0)
    return 167;
  static const char send_message[] = "SEND";
  char recv_buffer[4];
  if (poly_syscall6(POLY_SYS_SENDTO, socket_fds[0],
        (long) send_message, sizeof(recv_buffer), 0, 0, 0) !=
      (long) sizeof(recv_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 168;
  }
  if (poly_syscall6(POLY_SYS_RECVFROM, socket_fds[1],
        (long) recv_buffer, sizeof(recv_buffer), 0, 0, 0) !=
      (long) sizeof(recv_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 169;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0) != 0)
    return 170;
  if (recv_buffer[0] != 'S' || recv_buffer[1] != 'E' ||
      recv_buffer[2] != 'N' || recv_buffer[3] != 'D')
    return 171;

  fd = poly_syscall3(POLY_SYS_SOCKET, POLY_AF_UNIX, POLY_SOCK_STREAM, 0);
  if (fd < 0)
    return 172;
  int socket_option = 1;
  if (poly_syscall5(POLY_SYS_SETSOCKOPT, fd, POLY_SOL_SOCKET,
        POLY_SO_REUSEADDR, (long) &socket_option,
        sizeof(socket_option)) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 173;
  }
  socket_option = 0;
  uint32_t socket_option_len = sizeof(socket_option);
  if (poly_syscall5(POLY_SYS_GETSOCKOPT, fd, POLY_SOL_SOCKET,
        POLY_SO_REUSEADDR, (long) &socket_option,
        (long) &socket_option_len) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 174;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 175;
  if (socket_option == 0 || socket_option_len != sizeof(socket_option))
    return 176;

  if (poly_syscall4(POLY_SYS_SOCKETPAIR, POLY_AF_UNIX,
        POLY_SOCK_STREAM, 0, (long) socket_fds) != 0)
    return 177;
  if (poly_syscall2(POLY_SYS_SHUTDOWN, socket_fds[0], POLY_SHUT_WR) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 178;
  }
  char eof_byte = 0;
  if (poly_syscall3(POLY_SYS_READ, socket_fds[1], (long) &eof_byte, 1) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 179;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0) != 0)
    return 180;

  if (poly_syscall4(POLY_SYS_SOCKETPAIR, POLY_AF_UNIX,
        POLY_SOCK_STREAM, 0, (long) socket_fds) != 0)
    return 181;
  struct poly_sockaddr_un local_name;
  uint32_t local_name_len = sizeof(local_name);
  if (poly_syscall3(POLY_SYS_GETSOCKNAME, socket_fds[0],
        (long) &local_name, (long) &local_name_len) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 182;
  }
  struct poly_sockaddr_un peer_name;
  uint32_t peer_name_len = sizeof(peer_name);
  if (poly_syscall3(POLY_SYS_GETPEERNAME, socket_fds[0],
        (long) &peer_name, (long) &peer_name_len) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 183;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0) != 0)
    return 184;
  if (local_name_len < sizeof(local_name.family) ||
      peer_name_len < sizeof(peer_name.family))
    return 185;
  if (local_name.family != POLY_AF_UNIX || peer_name.family != POLY_AF_UNIX)
    return 186;

  long server_fd = poly_syscall3(POLY_SYS_SOCKET, POLY_AF_UNIX,
    POLY_SOCK_STREAM, 0);
  if (server_fd < 0)
    return 187;
  struct poly_sockaddr_un server_name;
  server_name.family = POLY_AF_UNIX;
  server_name.path[0] = 0;
  server_name.path[1] = 'p';
  server_name.path[2] = 'o';
  server_name.path[3] = 'l';
  server_name.path[4] = 'y';
  server_name.path[5] = 'p';
  server_name.path[6] = 'r';
  server_name.path[7] = 'o';
  server_name.path[8] = 'c';
  uint32_t server_name_len = sizeof(server_name.family) + 9;
  if (poly_syscall3(POLY_SYS_BIND, server_fd, (long) &server_name,
        server_name_len) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 188;
  }
  if (poly_syscall2(POLY_SYS_LISTEN, server_fd, 1) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 189;
  }
  long client_fd = poly_syscall3(POLY_SYS_SOCKET, POLY_AF_UNIX,
    POLY_SOCK_STREAM, 0);
  if (client_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 190;
  }
  if (poly_syscall3(POLY_SYS_CONNECT, client_fd, (long) &server_name,
        server_name_len) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 191;
  }
  long accepted_fd = poly_syscall3(POLY_SYS_ACCEPT, server_fd, 0, 0);
  if (accepted_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 192;
  }
  static const char connect_message[] = "CONN";
  char accept_buffer[4];
  if (poly_syscall3(POLY_SYS_WRITE, client_fd,
        (long) connect_message, sizeof(accept_buffer)) !=
      (long) sizeof(accept_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, accepted_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 193;
  }
  if (poly_syscall3(POLY_SYS_READ, accepted_fd, (long) accept_buffer,
        sizeof(accept_buffer)) != (long) sizeof(accept_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, accepted_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 194;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, accepted_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, client_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, server_fd, 0) != 0)
    return 195;
  if (accept_buffer[0] != 'C' || accept_buffer[1] != 'O' ||
      accept_buffer[2] != 'N' || accept_buffer[3] != 'N')
    return 196;

  server_fd = poly_syscall3(POLY_SYS_SOCKET, POLY_AF_UNIX,
    POLY_SOCK_STREAM, 0);
  if (server_fd < 0)
    return 292;
  for (unsigned n = 0; n < sizeof(server_name.path); n++)
    server_name.path[n] = 0;
  server_name.family = POLY_AF_UNIX;
  server_name.path[0] = 0;
  server_name.path[1] = 'p';
  server_name.path[2] = 'o';
  server_name.path[3] = 'l';
  server_name.path[4] = 'y';
  server_name.path[5] = 'p';
  server_name.path[6] = 'r';
  server_name.path[7] = 'o';
  server_name.path[8] = 'c';
  server_name.path[9] = '4';
  server_name_len = sizeof(server_name.family) + 10;
  if (poly_syscall3(POLY_SYS_BIND, server_fd, (long) &server_name,
        server_name_len) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 293;
  }
  if (poly_syscall2(POLY_SYS_LISTEN, server_fd, 1) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 294;
  }
  client_fd = poly_syscall3(POLY_SYS_SOCKET, POLY_AF_UNIX,
    POLY_SOCK_STREAM, 0);
  if (client_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 295;
  }
  if (poly_syscall3(POLY_SYS_CONNECT, client_fd, (long) &server_name,
        server_name_len) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 296;
  }
  accepted_fd = poly_syscall4(POLY_SYS_ACCEPT4, server_fd, 0, 0,
    POLY_SOCK_CLOEXEC);
  if (accepted_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 297;
  }
  fd_flags = poly_syscall3(POLY_SYS_FCNTL, accepted_fd, POLY_F_GETFD, 0);
  if ((fd_flags & POLY_FD_CLOEXEC) == 0) {
    poly_syscall2(POLY_SYS_CLOSE, accepted_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 298;
  }
  static const char accept4_message[] = "AC4!";
  char accept4_buffer[4];
  if (poly_syscall3(POLY_SYS_WRITE, client_fd,
        (long) accept4_message, sizeof(accept4_buffer)) !=
      (long) sizeof(accept4_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, accepted_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 299;
  }
  if (poly_syscall3(POLY_SYS_READ, accepted_fd, (long) accept4_buffer,
        sizeof(accept4_buffer)) != (long) sizeof(accept4_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, accepted_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, client_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, server_fd, 0);
    return 300;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, accepted_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, client_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, server_fd, 0) != 0)
    return 301;
  if (accept4_buffer[0] != 'A' || accept4_buffer[1] != 'C' ||
      accept4_buffer[2] != '4' || accept4_buffer[3] != '!')
    return 302;

  if (poly_syscall4(POLY_SYS_SOCKETPAIR, POLY_AF_UNIX,
        POLY_SOCK_STREAM, 0, (long) socket_fds) != 0)
    return 197;
  static const char msg_message[] = "MSG!";
  struct poly_iovec send_iov;
  send_iov.base = (uint64_t) (uintptr_t) msg_message;
  send_iov.len = sizeof(msg_message) - 1;
  struct poly_msghdr send_msg;
  send_msg.name = 0;
  send_msg.namelen = 0;
  send_msg.pad0 = 0;
  send_msg.iov = (uint64_t) (uintptr_t) &send_iov;
  send_msg.iovlen = 1;
  send_msg.control = 0;
  send_msg.controllen = 0;
  send_msg.flags = 0;
  send_msg.pad1 = 0;
  if (poly_syscall3(POLY_SYS_SENDMSG, socket_fds[0],
        (long) &send_msg, 0) != (long) sizeof(msg_message) - 1) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 198;
  }
  char msg_buffer[4];
  struct poly_iovec recv_iov;
  recv_iov.base = (uint64_t) (uintptr_t) msg_buffer;
  recv_iov.len = sizeof(msg_buffer);
  struct poly_msghdr recv_msg;
  recv_msg.name = 0;
  recv_msg.namelen = 0;
  recv_msg.pad0 = 0;
  recv_msg.iov = (uint64_t) (uintptr_t) &recv_iov;
  recv_msg.iovlen = 1;
  recv_msg.control = 0;
  recv_msg.controllen = 0;
  recv_msg.flags = 0;
  recv_msg.pad1 = 0;
  if (poly_syscall3(POLY_SYS_RECVMSG, socket_fds[1],
        (long) &recv_msg, 0) != (long) sizeof(msg_buffer)) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 199;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0) != 0)
    return 200;
  if (msg_buffer[0] != 'M' || msg_buffer[1] != 'S' ||
      msg_buffer[2] != 'G' || msg_buffer[3] != '!')
    return 201;

  if (poly_syscall4(POLY_SYS_SOCKETPAIR, POLY_AF_UNIX,
        POLY_SOCK_DGRAM, 0, (long) socket_fds) != 0)
    return 202;
  static const char batch_message0[] = "B0";
  static const char batch_message1[] = "B1";
  struct poly_iovec batch_send_iov0;
  batch_send_iov0.base = (uint64_t) (uintptr_t) batch_message0;
  batch_send_iov0.len = sizeof(batch_message0) - 1;
  struct poly_iovec batch_send_iov1;
  batch_send_iov1.base = (uint64_t) (uintptr_t) batch_message1;
  batch_send_iov1.len = sizeof(batch_message1) - 1;
  struct poly_mmsghdr batch_send[2];
  batch_send[0].hdr.name = 0;
  batch_send[0].hdr.namelen = 0;
  batch_send[0].hdr.pad0 = 0;
  batch_send[0].hdr.iov = (uint64_t) (uintptr_t) &batch_send_iov0;
  batch_send[0].hdr.iovlen = 1;
  batch_send[0].hdr.control = 0;
  batch_send[0].hdr.controllen = 0;
  batch_send[0].hdr.flags = 0;
  batch_send[0].hdr.pad1 = 0;
  batch_send[0].len = 0;
  batch_send[0].pad = 0;
  batch_send[1].hdr.name = 0;
  batch_send[1].hdr.namelen = 0;
  batch_send[1].hdr.pad0 = 0;
  batch_send[1].hdr.iov = (uint64_t) (uintptr_t) &batch_send_iov1;
  batch_send[1].hdr.iovlen = 1;
  batch_send[1].hdr.control = 0;
  batch_send[1].hdr.controllen = 0;
  batch_send[1].hdr.flags = 0;
  batch_send[1].hdr.pad1 = 0;
  batch_send[1].len = 0;
  batch_send[1].pad = 0;
  if (poly_syscall4(POLY_SYS_SENDMMSG, socket_fds[0],
        (long) batch_send, 2, 0) != 2) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 203;
  }
  char batch_buffer0[2];
  char batch_buffer1[2];
  struct poly_iovec batch_recv_iov0;
  batch_recv_iov0.base = (uint64_t) (uintptr_t) batch_buffer0;
  batch_recv_iov0.len = sizeof(batch_buffer0);
  struct poly_iovec batch_recv_iov1;
  batch_recv_iov1.base = (uint64_t) (uintptr_t) batch_buffer1;
  batch_recv_iov1.len = sizeof(batch_buffer1);
  struct poly_mmsghdr batch_recv[2];
  batch_recv[0].hdr.name = 0;
  batch_recv[0].hdr.namelen = 0;
  batch_recv[0].hdr.pad0 = 0;
  batch_recv[0].hdr.iov = (uint64_t) (uintptr_t) &batch_recv_iov0;
  batch_recv[0].hdr.iovlen = 1;
  batch_recv[0].hdr.control = 0;
  batch_recv[0].hdr.controllen = 0;
  batch_recv[0].hdr.flags = 0;
  batch_recv[0].hdr.pad1 = 0;
  batch_recv[0].len = 0;
  batch_recv[0].pad = 0;
  batch_recv[1].hdr.name = 0;
  batch_recv[1].hdr.namelen = 0;
  batch_recv[1].hdr.pad0 = 0;
  batch_recv[1].hdr.iov = (uint64_t) (uintptr_t) &batch_recv_iov1;
  batch_recv[1].hdr.iovlen = 1;
  batch_recv[1].hdr.control = 0;
  batch_recv[1].hdr.controllen = 0;
  batch_recv[1].hdr.flags = 0;
  batch_recv[1].hdr.pad1 = 0;
  batch_recv[1].len = 0;
  batch_recv[1].pad = 0;
  if (poly_syscall5(POLY_SYS_RECVMMSG, socket_fds[1],
        (long) batch_recv, 2, 0, 0) != 2) {
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0);
    poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0);
    return 204;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, socket_fds[0], 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, socket_fds[1], 0) != 0)
    return 205;
  if (batch_send[0].len != sizeof(batch_message0) - 1 ||
      batch_send[1].len != sizeof(batch_message1) - 1 ||
      batch_recv[0].len != sizeof(batch_buffer0) ||
      batch_recv[1].len != sizeof(batch_buffer1))
    return 206;
  if (batch_buffer0[0] != 'B' || batch_buffer0[1] != '0' ||
      batch_buffer1[0] != 'B' || batch_buffer1[1] != '1')
    return 207;

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

  event_fd = poly_syscall2(POLY_SYS_EVENTFD2, 0, 0);
  if (event_fd < 0)
    return 308;
  epoll_fd = poly_syscall2(POLY_SYS_EPOLL_CREATE1, 0, 0);
  if (epoll_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, event_fd, 0);
    return 309;
  }
  event.events = POLY_EPOLLIN;
  event.pad = 0;
  event.data = 0x8877665544332211ULL;
  if (poly_syscall4(POLY_SYS_EPOLL_CTL, epoll_fd, POLY_EPOLL_CTL_ADD,
        event_fd, (long) &event) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, epoll_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, event_fd, 0);
    return 310;
  }
  event_value = 11;
  if (poly_syscall3(POLY_SYS_WRITE, event_fd, (long) &event_value,
        sizeof(event_value)) != (long) sizeof(event_value)) {
    poly_syscall2(POLY_SYS_CLOSE, epoll_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, event_fd, 0);
    return 311;
  }
  ready_event.events = 0;
  ready_event.pad = 0;
  ready_event.data = 0;
  struct poly_timespec epoll_timeout = { 0, 0 };
  if (poly_syscall6(POLY_SYS_EPOLL_PWAIT2, epoll_fd,
        (long) &ready_event, 1, (long) &epoll_timeout, 0, 0) != 1) {
    poly_syscall2(POLY_SYS_CLOSE, epoll_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, event_fd, 0);
    return 312;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, epoll_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, event_fd, 0) != 0)
    return 313;
  if ((ready_event.events & POLY_EPOLLIN) == 0 ||
      ready_event.data != 0x8877665544332211ULL)
    return 314;

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
  if (signal_info.signo != POLY_SIGUSR1 || signal_info.pid != (uint32_t) pid0)
    return 146;
  long pid_signal_fd = poly_syscall2(POLY_SYS_PIDFD_OPEN, pid0, 0);
  if (pid_signal_fd < 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 283;
  }
  if (poly_syscall4(POLY_SYS_PIDFD_SEND_SIGNAL, pid_signal_fd,
        POLY_SIGUSR1, 0, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, pid_signal_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 284;
  }
  struct poly_signalfd_siginfo pidfd_signal_info;
  if (poly_syscall3(POLY_SYS_READ, fd, (long) &pidfd_signal_info,
        sizeof(pidfd_signal_info)) != (long) sizeof(pidfd_signal_info)) {
    poly_syscall2(POLY_SYS_CLOSE, pid_signal_fd, 0);
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 285;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, pid_signal_fd, 0) != 0 ||
      poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 145;
  if (pidfd_signal_info.signo != POLY_SIGUSR1 ||
      pidfd_signal_info.pid != (uint32_t) pid0)
    return 286;
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

  static const char namespace_dir[] = "/polyproc-syscall-real";
  static const char namespace_file[] = "/polyproc-syscall-real/file";
  static const char namespace_fifo[] = "/polyproc-syscall-real/fifo";
  static const char namespace_renamed[] = "/polyproc-syscall-real/renamed";
  static const char namespace_temp[] = "/polyproc-syscall-real/temp";
  static const char namespace_hard[] = "/polyproc-syscall-real/hard";
  static const char namespace_symlink[] = "/polyproc-syscall-real/symlink";
  poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD, (long) namespace_hard, 0);
  poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD, (long) namespace_symlink, 0);
  poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD, (long) namespace_temp, 0);
  poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD, (long) namespace_renamed, 0);
  poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD, (long) namespace_fifo, 0);
  poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD, (long) namespace_file, 0);
  poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD, (long) namespace_dir,
    POLY_AT_REMOVEDIR);
  if (poly_syscall3(POLY_SYS_MKDIRAT, POLY_AT_FDCWD,
        (long) namespace_dir, 0700) != 0)
    return 243;
  if (poly_syscall4(POLY_SYS_MKNODAT, POLY_AT_FDCWD,
        (long) namespace_fifo, POLY_S_IFIFO | 0600, 0) != 0)
    return 331;
  struct poly_linux_generic_stat namespace_fifo_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_fifo, (long) &namespace_fifo_stat, 0) != 0)
    return 332;
  if ((namespace_fifo_stat.mode & POLY_S_IFMT) != POLY_S_IFIFO)
    return 333;
  struct poly_linux_generic_stat namespace_dir_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_dir, (long) &namespace_dir_stat, 0) != 0)
    return 244;
  if ((namespace_dir_stat.mode & POLY_S_IFMT) != POLY_S_IFDIR)
    return 245;
  long namespace_dir_fd = poly_syscall3(POLY_SYS_OPENAT, POLY_AT_FDCWD,
    (long) namespace_dir, POLY_O_DIRECTORY | POLY_O_CLOEXEC);
  if (namespace_dir_fd < 0)
    return 323;
  if (poly_syscall2(POLY_SYS_CHDIR, (long) namespace_dir, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, namespace_dir_fd, 0);
    return 324;
  }
  char namespace_cwd[128];
  long namespace_cwd_len = poly_syscall2(POLY_SYS_GETCWD,
    (long) namespace_cwd, sizeof(namespace_cwd));
  if (namespace_cwd_len <= 1 || !poly_streq(namespace_cwd, namespace_dir)) {
    poly_syscall2(POLY_SYS_CLOSE, namespace_dir_fd, 0);
    return 325;
  }
  if (poly_syscall2(POLY_SYS_CHDIR, (long) cwd, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, namespace_dir_fd, 0);
    return 326;
  }
  if (poly_syscall2(POLY_SYS_FCHDIR, namespace_dir_fd, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, namespace_dir_fd, 0);
    return 327;
  }
  namespace_cwd_len = poly_syscall2(POLY_SYS_GETCWD, (long) namespace_cwd,
    sizeof(namespace_cwd));
  if (namespace_cwd_len <= 1 || !poly_streq(namespace_cwd, namespace_dir)) {
    poly_syscall2(POLY_SYS_CLOSE, namespace_dir_fd, 0);
    return 328;
  }
  if (poly_syscall2(POLY_SYS_CHDIR, (long) cwd, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, namespace_dir_fd, 0);
    return 329;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, namespace_dir_fd, 0) != 0)
    return 330;
  fd = poly_syscall4(POLY_SYS_OPENAT, POLY_AT_FDCWD,
    (long) namespace_file,
    POLY_O_CREAT | POLY_O_RDWR | POLY_O_TRUNC | POLY_O_CLOEXEC, 0600);
  if (fd < 0)
    return 246;
  static const char namespace_message[] = "NS";
  if (poly_syscall3(POLY_SYS_WRITE, fd, (long) namespace_message,
        sizeof(namespace_message) - 1) !=
      (long) sizeof(namespace_message) - 1) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 247;
  }
  if (poly_syscall2(POLY_SYS_TRUNCATE, (long) namespace_file, 1) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 315;
  }
  struct poly_linux_generic_stat namespace_truncated_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_file, (long) &namespace_truncated_stat, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 316;
  }
  if (namespace_truncated_stat.size != 1) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 317;
  }
  if (poly_syscall3(POLY_SYS_LSEEK, fd, 0, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 318;
  }
  if (poly_syscall3(POLY_SYS_WRITE, fd, (long) namespace_message,
        sizeof(namespace_message) - 1) !=
      (long) sizeof(namespace_message) - 1) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 319;
  }
  if (poly_syscall2(POLY_SYS_FCHMOD, fd, 0640) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 320;
  }
  struct poly_linux_generic_stat namespace_fchmod_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_file, (long) &namespace_fchmod_stat, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 321;
  }
  if ((namespace_fchmod_stat.mode & 0777) != 0640) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 322;
  }
  if (poly_syscall4(POLY_SYS_FCHMODAT, POLY_AT_FDCWD,
        (long) namespace_file, 0600, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 335;
  }
  struct poly_linux_generic_stat namespace_fchmodat_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_file, (long) &namespace_fchmodat_stat, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 336;
  }
  if ((namespace_fchmodat_stat.mode & 0777) != 0600) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 337;
  }
  if (poly_syscall3(POLY_SYS_FCHOWN, fd, uid, gid) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 338;
  }
  struct poly_linux_generic_stat namespace_fchown_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_file, (long) &namespace_fchown_stat, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 339;
  }
  if (namespace_fchown_stat.uid != (uint32_t) uid ||
      namespace_fchown_stat.gid != (uint32_t) gid) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 340;
  }
  if (poly_syscall5(POLY_SYS_FCHOWNAT, POLY_AT_FDCWD,
        (long) namespace_file, uid, gid, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 341;
  }
  struct poly_linux_generic_stat namespace_fchownat_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_file, (long) &namespace_fchownat_stat, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 342;
  }
  if (namespace_fchownat_stat.uid != (uint32_t) uid ||
      namespace_fchownat_stat.gid != (uint32_t) gid) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 343;
  }
  static const char xattr_name[] = "user.poly";
  static const char xattr_value_path[] = "PXA";
  if (poly_syscall5(POLY_SYS_SETXATTR, (long) namespace_file,
        (long) xattr_name, (long) xattr_value_path,
        sizeof(xattr_value_path) - 1, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 266;
  }
  char xattr_read[4];
  if (poly_syscall4(POLY_SYS_GETXATTR, (long) namespace_file,
        (long) xattr_name, (long) xattr_read, sizeof(xattr_read)) !=
      (long) sizeof(xattr_value_path) - 1) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 267;
  }
  if (xattr_read[0] != 'P' || xattr_read[1] != 'X' ||
      xattr_read[2] != 'A') {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 268;
  }
  static const char xattr_value_fd[] = "FXA";
  if (poly_syscall5(POLY_SYS_FSETXATTR, fd, (long) xattr_name,
        (long) xattr_value_fd, sizeof(xattr_value_fd) - 1, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 269;
  }
  if (poly_syscall4(POLY_SYS_FGETXATTR, fd, (long) xattr_name,
        (long) xattr_read, sizeof(xattr_read)) !=
      (long) sizeof(xattr_value_fd) - 1) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 270;
  }
  if (xattr_read[0] != 'F' || xattr_read[1] != 'X' ||
      xattr_read[2] != 'A') {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 271;
  }
  char xattr_list[64];
  long xattr_list_len = poly_syscall3(POLY_SYS_LISTXATTR,
    (long) namespace_file, (long) xattr_list, sizeof(xattr_list));
  if (xattr_list_len <= 0 ||
      !poly_contains_len(xattr_list, xattr_list_len, xattr_name)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 272;
  }
  xattr_list_len = poly_syscall3(POLY_SYS_FLISTXATTR, fd,
    (long) xattr_list, sizeof(xattr_list));
  if (xattr_list_len <= 0 ||
      !poly_contains_len(xattr_list, xattr_list_len, xattr_name)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 273;
  }
  if (poly_syscall2(POLY_SYS_REMOVEXATTR, (long) namespace_file,
        (long) xattr_name) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 274;
  }
  if (poly_syscall5(POLY_SYS_FSETXATTR, fd, (long) xattr_name,
        (long) xattr_value_fd, sizeof(xattr_value_fd) - 1, 0) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 275;
  }
  if (poly_syscall2(POLY_SYS_FREMOVEXATTR, fd, (long) xattr_name) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 276;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 248;
  if (poly_syscall5(POLY_SYS_RENAMEAT2, POLY_AT_FDCWD,
        (long) namespace_file, POLY_AT_FDCWD, (long) namespace_temp,
        POLY_RENAME_NOREPLACE) != 0)
    return 303;
  if (poly_syscall5(POLY_SYS_RENAMEAT2, POLY_AT_FDCWD,
        (long) namespace_temp, POLY_AT_FDCWD, (long) namespace_file, 0) != 0)
    return 304;
  if (poly_syscall4(POLY_SYS_RENAMEAT, POLY_AT_FDCWD,
        (long) namespace_file, POLY_AT_FDCWD,
        (long) namespace_renamed) != 0)
    return 249;
  if (poly_syscall5(POLY_SYS_LINKAT, POLY_AT_FDCWD,
        (long) namespace_renamed, POLY_AT_FDCWD,
        (long) namespace_hard, 0) != 0)
    return 250;
  if (poly_syscall3(POLY_SYS_SYMLINKAT, (long) "renamed",
        POLY_AT_FDCWD, (long) namespace_symlink) != 0)
    return 251;
  char symlink_target[16];
  long symlink_len = poly_syscall4(POLY_SYS_READLINKAT, POLY_AT_FDCWD,
    (long) namespace_symlink, (long) symlink_target, sizeof(symlink_target));
  if (symlink_len != 7 || !poly_contains_len(symlink_target, symlink_len,
        "renamed"))
    return 252;
  struct poly_linux_generic_stat namespace_link_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_symlink, (long) &namespace_link_stat,
        POLY_AT_SYMLINK_NOFOLLOW) != 0)
    return 253;
  if ((namespace_link_stat.mode & POLY_S_IFMT) != POLY_S_IFLNK)
    return 254;
  struct poly_linux_generic_stat namespace_file_stat;
  struct poly_linux_generic_stat namespace_hard_stat;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_renamed, (long) &namespace_file_stat, 0) != 0)
    return 255;
  if (poly_syscall4(POLY_SYS_NEWFSTATAT, POLY_AT_FDCWD,
        (long) namespace_hard, (long) &namespace_hard_stat, 0) != 0)
    return 256;
  if ((namespace_file_stat.mode & POLY_S_IFMT) != POLY_S_IFREG ||
      namespace_file_stat.size != 2 ||
      namespace_file_stat.ino != namespace_hard_stat.ino)
    return 257;
  fd = poly_syscall3(POLY_SYS_OPENAT, POLY_AT_FDCWD,
    (long) namespace_hard, 0);
  if (fd < 0)
    return 258;
  char namespace_read[2];
  if (poly_syscall3(POLY_SYS_READ, fd, (long) namespace_read,
        sizeof(namespace_read)) != (long) sizeof(namespace_read)) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 259;
  }
  if (poly_syscall2(POLY_SYS_CLOSE, fd, 0) != 0)
    return 260;
  if (namespace_read[0] != 'N' || namespace_read[1] != 'S')
    return 261;
  if (poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD,
        (long) namespace_hard, 0) != 0)
    return 262;
  if (poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD,
        (long) namespace_symlink, 0) != 0)
    return 263;
  if (poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD,
        (long) namespace_renamed, 0) != 0)
    return 264;
  if (poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD,
        (long) namespace_fifo, 0) != 0)
    return 334;
  if (poly_syscall3(POLY_SYS_UNLINKAT, POLY_AT_FDCWD,
        (long) namespace_dir, POLY_AT_REMOVEDIR) != 0)
    return 265;

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
  if (poly_syscall4(POLY_SYS_FADVISE64, fd, 0, 0,
        POLY_POSIX_FADV_NORMAL) != 0) {
    poly_syscall2(POLY_SYS_CLOSE, fd, 0);
    return 359;
  }
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
  if (poly_syscall3(POLY_SYS_MADVISE, page, 4096, POLY_MADV_NORMAL) != 0) {
    poly_syscall2(POLY_SYS_MUNMAP, page, 4096);
    return 360;
  }
  if (poly_syscall3(POLY_SYS_MLOCK2, page, 4096, 0) != 0) {
    poly_syscall2(POLY_SYS_MUNMAP, page, 4096);
    return 361;
  }
  if (poly_syscall2(POLY_SYS_MUNLOCK, page, 4096) != 0) {
    poly_syscall2(POLY_SYS_MUNMAP, page, 4096);
    return 362;
  }
  if (poly_syscall2(POLY_SYS_MLOCK, page, 4096) != 0) {
    poly_syscall2(POLY_SYS_MUNMAP, page, 4096);
    return 363;
  }
  if (poly_syscall2(POLY_SYS_MUNLOCK, page, 4096) != 0) {
    poly_syscall2(POLY_SYS_MUNMAP, page, 4096);
    return 364;
  }
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
