#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
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

static int parse_id(const char *value, uid_t *id_out) {
  char *end = NULL;
  errno = 0;
  unsigned long id = strtoul(value, &end, 10);
  if (errno != 0 || end == value || *end != '\0' || id > 65535UL) {
    fprintf(stderr, "POLYCONTAINER_FAIL: invalid id: %s\n", value);
    return -1;
  }
  *id_out = (uid_t) id;
  return 0;
}

static int run_container_init(const char *rootfs, char **cmd_argv,
    const char *cwd,
    uid_t run_uid, gid_t run_gid, int has_uid, int has_gid) {
  if (sethostname("poly-alpine", strlen("poly-alpine")) != 0)
    fprintf(stderr, "POLYCONTAINER_WARN: sethostname: %s\n", strerror(errno));

  if (prepare_mounts(rootfs) != 0)
    _exit(125);
  if (chroot(rootfs) != 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: chroot %s: %s\n", rootfs,
      strerror(errno));
    _exit(125);
  }
  if (chdir(cwd) != 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: chdir %s: %s\n", cwd,
      strerror(errno));
    _exit(125);
  }

  if (has_gid) {
    if (setgroups(0, NULL) != 0)
      fprintf(stderr, "POLYCONTAINER_WARN: clear groups: %s\n",
        strerror(errno));
    if (setgid(run_gid) != 0) {
      fprintf(stderr, "POLYCONTAINER_FAIL: setgid %lu: %s\n",
        (unsigned long) run_gid, strerror(errno));
      _exit(125);
    }
  }
  if (has_uid && setuid(run_uid) != 0) {
    fprintf(stderr, "POLYCONTAINER_FAIL: setuid %lu: %s\n",
      (unsigned long) run_uid, strerror(errno));
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
      "usage: %s [--uid UID] [--gid GID] [--cwd DIR] ROOTFS COMMAND [ARG...]\n",
      argv[0] ? argv[0] :
      "polycontainer-run");
    return 2;
  }

  const char *cwd = "/";
  uid_t run_uid = 0;
  gid_t run_gid = 0;
  int has_uid = 0;
  int has_gid = 0;
  int argi = 1;
  while (argi < argc && strncmp(argv[argi], "--", 2) == 0) {
    if (strcmp(argv[argi], "--uid") == 0 && argi + 1 < argc) {
      uid_t parsed = 0;
      if (parse_id(argv[argi + 1], &parsed) != 0)
        return 2;
      run_uid = parsed;
      has_uid = 1;
      argi += 2;
      continue;
    }
    if (strcmp(argv[argi], "--gid") == 0 && argi + 1 < argc) {
      uid_t parsed = 0;
      if (parse_id(argv[argi + 1], &parsed) != 0)
        return 2;
      run_gid = (gid_t) parsed;
      has_gid = 1;
      argi += 2;
      continue;
    }
    if (strcmp(argv[argi], "--cwd") == 0 && argi + 1 < argc) {
      cwd = argv[argi + 1];
      argi += 2;
      continue;
    }
    fprintf(stderr, "POLYCONTAINER_FAIL: unknown option: %s\n", argv[argi]);
    return 2;
  }
  if (argc - argi < 2) {
    fprintf(stderr,
      "usage: %s [--uid UID] [--gid GID] [--cwd DIR] ROOTFS COMMAND [ARG...]\n",
      argv[0] ? argv[0] : "polycontainer-run");
    return 2;
  }

  const char *rootfs = argv[argi];
  char **cmd_argv = &argv[argi + 1];

  fprintf(stderr,
    "POLYCONTAINER_START: rootfs=%s namespaces=mount,pid,uts,ipc cwd=%s uid=%s%lu gid=%s%lu\n",
    rootfs, cwd,
    has_uid ? "" : "default:", (unsigned long) run_uid,
    has_gid ? "" : "default:", (unsigned long) run_gid);
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
    run_container_init(rootfs, cmd_argv, cwd, run_uid, run_gid, has_uid,
      has_gid);

  _exit(wait_for_child(init));
}
