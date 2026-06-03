#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s command [args...]\n", argv[0]);
    return 125;
  }

  if (setgid(65534) != 0 || setuid(65534) != 0) {
    fprintf(stderr, "POLY_NONROOT_FAIL: unable to drop privileges: %s\n",
      strerror(errno));
    return 125;
  }

  if (chdir("/tmp") != 0) {
    fprintf(stderr, "POLY_NONROOT_FAIL: unable to enter /tmp: %s\n",
      strerror(errno));
    return 125;
  }

  printf("POLY_NONROOT_EXEC: uid=%ld euid=%ld gid=%ld egid=%ld command=%s\n",
    (long) getuid(), (long) geteuid(), (long) getgid(), (long) getegid(),
    argv[1]);
  fflush(stdout);

  setenv("POLYEXEC_REPORT_UID", "1", 1);
  execvp(argv[1], &argv[1]);
  fprintf(stderr, "POLY_NONROOT_FAIL: exec %s: %s\n", argv[1],
    strerror(errno));
  return 125;
}
