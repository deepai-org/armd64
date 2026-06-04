#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/memfd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef F_SEAL_EXEC
#define F_SEAL_EXEC 0x0020
#endif

static int copy_fd(int dst, int src) {
  char buffer[4096];
  for (;;) {
    ssize_t n = read(src, buffer, sizeof(buffer));
    if (n == 0)
      return 0;
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    ssize_t off = 0;
    while (off < n) {
      ssize_t w = write(dst, buffer + off, (size_t) (n - off));
      if (w < 0) {
        if (errno == EINTR)
          continue;
        return -1;
      }
      off += w;
    }
  }
}

static int drain_fd(int fd) {
  char buffer[4096];
  for (;;) {
    ssize_t n = read(fd, buffer, sizeof(buffer));
    if (n > 0) {
      ssize_t off = 0;
      while (off < n) {
        ssize_t w = write(STDERR_FILENO, buffer + off, (size_t) (n - off));
        if (w < 0)
          return -1;
        off += w;
      }
      continue;
    }
    if (n == 0)
      return 0;
    if (errno == EINTR)
      continue;
    return -1;
  }
}

int main(void) {
  const char *trigger = "/tmp/poly-apk-scripts/busybox.trigger";
  int trigger_fd = open(trigger, O_RDONLY);
  if (trigger_fd < 0) {
    fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_FAIL: open %s: %s\n",
      trigger, strerror(errno));
    return 2;
  }
  int script_fd = (int) syscall(SYS_memfd_create,
    "lib/apk/exec/busybox.trigger", MFD_EXEC | MFD_ALLOW_SEALING);
  if (script_fd < 0) {
    fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_FAIL: memfd_create: %s\n",
      strerror(errno));
    return 2;
  }
  if (copy_fd(script_fd, trigger_fd) != 0) {
    fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_FAIL: copy script: %s\n",
      strerror(errno));
    return 2;
  }
  close(trigger_fd);
  (void) fcntl(script_fd, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_SHRINK |
    F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_FUTURE_WRITE | F_SEAL_EXEC);

  int output_pipe[2];
  if (pipe(output_pipe) != 0) {
    fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_FAIL: pipe: %s\n",
      strerror(errno));
    return 2;
  }

  char script_path[64];
  snprintf(script_path, sizeof(script_path), "/proc/self/fd/%d", script_fd);
  char *const argv[] = {
    script_path,
    "/bin",
    "/usr/bin",
    "/sbin",
    "/usr/sbin",
    NULL
  };
  char *const envp[] = {
    "APK_SCRIPT=trigger",
    "APK_PACKAGE=busybox",
    NULL
  };

  fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_START: script=%s\n", script_path);
  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_FAIL: fork: %s\n",
      strerror(errno));
    return 2;
  }

  if (pid == 0) {
    close(output_pipe[0]);
    if (dup2(output_pipe[1], STDOUT_FILENO) < 0 ||
        dup2(output_pipe[1], STDERR_FILENO) < 0) {
      fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_CHILD_FAIL: dup2: %s\n",
        strerror(errno));
      _exit(127);
    }
    close(output_pipe[1]);
    int root_fd = open("/", O_RDONLY | O_DIRECTORY);
    if (root_fd >= 0) {
      if (fchdir(root_fd) != 0)
        fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_CHILD_FAIL: fchdir: %s\n",
          strerror(errno));
      if (chroot(".") != 0)
        fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_CHILD_FAIL: chroot: %s\n",
          strerror(errno));
      close(root_fd);
    }
    execve(script_path, argv, envp);
    fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_CHILD_FAIL: execve: %s\n",
      strerror(errno));
    _exit(127);
  }

  close(script_fd);
  close(output_pipe[1]);
  (void) drain_fd(output_pipe[0]);
  close(output_pipe[0]);

  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno == EINTR)
      continue;
    fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_FAIL: waitpid: %s\n",
      strerror(errno));
    return 2;
  }

  fprintf(stderr, "POLY_APK_TRIGGER_MIMIC_STATUS: raw=0x%x", status);
  if (WIFEXITED(status))
    fprintf(stderr, " exit=%d", WEXITSTATUS(status));
  if (WIFSIGNALED(status))
    fprintf(stderr, " signal=%d", WTERMSIG(status));
  fputc('\n', stderr);
  return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
}
