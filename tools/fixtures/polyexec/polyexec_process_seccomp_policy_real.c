#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef PR_SET_SECCOMP
#define PR_SET_SECCOMP 22
#endif

#ifndef PR_GET_SECCOMP
#define PR_GET_SECCOMP 21
#endif

#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#endif

#ifndef PR_GET_NO_NEW_PRIVS
#define PR_GET_NO_NEW_PRIVS 39
#endif

#define POLY_SECCOMP_MODE_FILTER 2
#define POLY_SECCOMP_RET_ERRNO 0x00050000U
#define POLY_SECCOMP_RET_ALLOW 0x7fff0000U
#define POLY_BPF_LD_W_ABS 0x20
#define POLY_BPF_JMP_JEQ_K 0x15
#define POLY_BPF_RET_K 0x06
#define POLY_SECCOMP_NR_OFFSET 0
#define POLY_SECCOMP_ARCH_OFFSET 4

#if defined(__aarch64__)
#define POLY_AUDIT_ARCH 0xc00000b7U
#elif defined(__riscv) && __riscv_xlen == 64
#define POLY_AUDIT_ARCH 0xc00000f3U
#else
#error "unsupported seccomp policy fixture architecture"
#endif

#define POLY_SYS_MKDIRAT 34

struct poly_bpf_insn {
  uint16_t code;
  uint8_t jt;
  uint8_t jf;
  uint32_t k;
};

struct poly_sock_fprog {
  uint16_t len;
  struct poly_bpf_insn *filter;
};

static long poly_prctl(long option, long arg2, long arg3, long arg4,
    long arg5) {
  return syscall(SYS_prctl, option, arg2, arg3, arg4, arg5);
}

int main(void) {
  struct poly_bpf_insn filter[] = {
    { POLY_BPF_LD_W_ABS, 0, 0, POLY_SECCOMP_ARCH_OFFSET },
    { POLY_BPF_JMP_JEQ_K, 0, 3, POLY_AUDIT_ARCH },
    { POLY_BPF_LD_W_ABS, 0, 0, POLY_SECCOMP_NR_OFFSET },
    { POLY_BPF_JMP_JEQ_K, 1, 0, POLY_SYS_MKDIRAT },
    { POLY_BPF_RET_K, 0, 0, POLY_SECCOMP_RET_ALLOW },
    { POLY_BPF_RET_K, 0, 0, POLY_SECCOMP_RET_ERRNO | EPERM },
  };
  struct poly_sock_fprog program = {
    .len = sizeof(filter) / sizeof(filter[0]),
    .filter = filter,
  };

  if (poly_prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0)
    return 10;
  if (poly_prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) != 1)
    return 11;
  if (poly_prctl(PR_SET_SECCOMP, POLY_SECCOMP_MODE_FILTER,
        (long) &program, 0, 0) != 0)
    return 12;
  if (poly_prctl(PR_GET_SECCOMP, 0, 0, 0, 0) != POLY_SECCOMP_MODE_FILTER)
    return 13;

  errno = 0;
  if (mkdir("/tmp/poly-seccomp-denied", 0700) == 0 || errno != EPERM)
    return 14;
  if (getpid() <= 0)
    return 15;

  puts("POLY_SECCOMP_POLICY_OK");
  return 42;
}
