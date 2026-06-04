#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

static int mkdir_if_missing(const char *path, mode_t mode) {
  if (mkdir(path, mode) == 0 || errno == EEXIST)
    return 0;
  fprintf(stderr, "POLYCONTAINER_FAIL: mkdir %s: %s\n", path,
    strerror(errno));
  return -1;
}

static int join_path(char *out, size_t out_size, const char *root,
    const char *name) {
  int written = snprintf(out, out_size, "%s/%s", root, name);
  if (written < 0 || (size_t) written >= out_size) {
    fprintf(stderr, "POLYCONTAINER_FAIL: path too long: %s/%s\n", root, name);
    return -1;
  }
  return 0;
}

static int prepare_mounts(const char *rootfs) {
  if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: make mounts private: %s\n",
      strerror(errno));
    return -1;
  }
  if (mount(rootfs, rootfs, NULL, MS_BIND | MS_REC, NULL) != 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: bind rootfs %s: %s\n", rootfs,
      strerror(errno));
    return -1;
  }

  char path[4096];
  if (join_path(path, sizeof(path), rootfs, "proc") != 0 ||
      mkdir_if_missing(path, 0555) != 0)
    return -1;
  if (mount("proc", path, "proc", 0, "") != 0 && errno != EBUSY) {
    fprintf(stderr, "POLYCONTAINER_FAIL: mount proc: %s\n", strerror(errno));
    return -1;
  }

  if (join_path(path, sizeof(path), rootfs, "sys") != 0 ||
      mkdir_if_missing(path, 0555) != 0)
    return -1;
  if (mount("sysfs", path, "sysfs", 0, "") != 0 && errno != EBUSY)
    fprintf(stderr, "POLYCONTAINER_WARN: mount sysfs: %s\n", strerror(errno));

  if (join_path(path, sizeof(path), rootfs, "dev") != 0 ||
      mkdir_if_missing(path, 0755) != 0)
    return -1;
  if (mount("/dev", path, NULL, MS_BIND | MS_REC, NULL) != 0 &&
      errno != EBUSY) {
    fprintf(stderr, "POLYCONTAINER_WARN: bind /dev: %s\n", strerror(errno));
  }
  return 0;
}

static int run_container_init(const char *rootfs, char **cmd_argv) {
  if (sethostname("poly-alpine", strlen("poly-alpine")) != 0)
    fprintf(stderr, "POLYCONTAINER_WARN: sethostname: %s\n", strerror(errno));

  if (prepare_mounts(rootfs) != 0)
    _exit(125);
  if (chroot(rootfs) != 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: chroot %s: %s\n", rootfs,
      strerror(errno));
    _exit(125);
  }
  if (chdir("/") != 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: chdir /: %s\n", strerror(errno));
    _exit(125);
  }

  execv(cmd_argv[0], cmd_argv);
  fprintf(stderr, "POLYCONTAINER_FAIL: exec %s: %s\n", cmd_argv[0],
    strerror(errno));
  _exit(127);
}

static int wait_for_child(pid_t pid) {
  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno == EINTR)
      continue;
    fprintf(stderr, "POLYCONTAINER_FAIL: waitpid: %s\n", strerror(errno));
    return 125;
  }

  fprintf(stderr, "POLYCONTAINER_STATUS: raw=0x%x", status);
  if (WIFEXITED(status))
    fprintf(stderr, " exit=%d", WEXITSTATUS(status));
  if (WIFSIGNALED(status))
    fprintf(stderr, " signal=%d", WTERMSIG(status));
  fputc('\n', stderr);

  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);
  return 125;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr,
      "usage: %s ROOTFS COMMAND [ARG...]\n", argv[0] ? argv[0] :
      "polycontainer-run");
    return 2;
  }

  const char *rootfs = argv[1];
  char **cmd_argv = &argv[2];

  fprintf(stderr,
    "POLYCONTAINER_START: rootfs=%s namespaces=mount,pid,uts,ipc\n",
    rootfs);
  pid_t supervisor = fork();
  if (supervisor < 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: fork supervisor: %s\n",
      strerror(errno));
    return 125;
  }
  if (supervisor != 0)
    return wait_for_child(supervisor);

  if (unshare(CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID) != 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: unshare namespaces: %s\n",
      strerror(errno));
    _exit(125);
  }

  pid_t init = fork();
  if (init < 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: fork container init: %s\n",
      strerror(errno));
    _exit(125);
  }
  if (init == 0)
    run_container_init(rootfs, cmd_argv);

  _exit(wait_for_child(init));
}
