#define _GNU_SOURCE

#include <errno.h>
#include <elf.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/polycpuid.h"

#ifndef EM_AARCH64
#define EM_AARCH64 183
#endif

#ifndef EM_RISCV
#define EM_RISCV 243
#endif

#define POLY_OP_TRAP_VECTOR_SET POLY_X86_CTRL_TRAP_VECTOR_SET_ASM
#define POLY_OP_TRAP_VECTOR_MODE_SET POLY_X86_CTRL_TRAP_VECTOR_MODE_SET_ASM
#define POLY_OP_STATE_KEY_SET POLY_X86_CTRL_STATE_KEY_SET_ASM
#define POLY_OP_STATE_EXPORT POLY_X86_CTRL_STATE_EXPORT_ASM
#define POLY_OP_STATE_IMPORT POLY_X86_CTRL_STATE_IMPORT_ASM
#define POLY_OP_ABI_SIGNATURE_SET POLY_X86_CTRL_ABI_SIGNATURE_SET_ASM
#define POLY_OP_EVENT_PTR_SET POLY_X86_CTRL_EVENT_PTR_SET_ASM

static void clear_poly_xsave_state(void) {
  struct poly_xsave_state state __attribute__((aligned(POLY_STATE_XSAVE_ALIGN_ARCH)));
  memset(&state, 0, sizeof(state));

  asm volatile(POLY_OP_STATE_EXPORT :: "a"(&state) : "r15", "memory");
  if (state.header.magic != POLY_STATE_XSAVE_MAGIC ||
      state.header.total_bytes != POLY_STATE_XSAVE_BYTES_ARCH ||
      state.header.header_bytes != POLY_STATE_XSAVE_HEADER_BYTES ||
      state.header.layout_version != POLY_STATE_XSAVE_LAYOUT_VERSION)
    return;

  memset(&state.event_record, 0, sizeof(state.event_record));
  memset(state.event_args, 0, sizeof(state.event_args));
  memset(&state.transition, 0, sizeof(state.transition));
  memset(state.aarch64_gpr, 0, sizeof(state.aarch64_gpr));
  memset(state.aarch64_fp, 0, sizeof(state.aarch64_fp));
  memset(&state.aarch64_status, 0, sizeof(state.aarch64_status));
  memset(state.riscv_gpr, 0, sizeof(state.riscv_gpr));
  memset(state.riscv_fp, 0, sizeof(state.riscv_fp));
  memset(&state.riscv_status, 0, sizeof(state.riscv_status));
  memset(&state.import_return, 0, sizeof(state.import_return));
  memset(&state.cross_return, 0, sizeof(state.cross_return));
  memset(&state.trap_restore, 0, sizeof(state.trap_restore));
  memset(&state.native_return, 0, sizeof(state.native_return));
  memset(state.pre_trap_restore_reserved, 0,
    sizeof(state.pre_trap_restore_reserved));

  state.header.total_bytes = POLY_STATE_XSAVE_BYTES_ARCH;
  state.header.current_mode = POLY_MODE_X86;
  state.header.foreign_pc = 0;
  state.header.foreign_tls_base = 0;
  state.header.trap_vector_pc = 0;
  state.header.trap_vector_mode = POLY_MODE_X86;
  state.header.spill_reason = 0;
  state.header.reserved0 = 0;

  state.import_return.depth = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH;
  state.abi_signature.slot_count = POLY_ABI_SIGNATURE_SLOT_COUNT;
  state.abi_signature.flags = 0;
  for (size_t slot = 0; slot < POLY_ABI_SIGNATURE_SLOT_COUNT; slot++) {
    state.abi_signature.slots[slot].kind =
      POLY_ABI_SIGNATURE_KIND_EXCHANGE;
    state.abi_signature.slots[slot].register_map = 0;
  }
  state.cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
  state.frontend_tls.flags = 1;
  state.frontend_tls.active_mode = POLY_MODE_X86;
  state.frontend_tls.aarch64_tls_base = 0;
  state.frontend_tls.riscv_tls_base = 0;
  state.landing_policy.flags = 0;
  state.landing_policy.supported_flags = POLY_LANDING_POLICY_SUPPORTED;
  state.state_key.flags = 0;
  state.state_key.explicit_key = 0;
  state.state_key.supported_flags = POLY_STATE_KEY_FLAG_EXPLICIT;
  state.native_return.depth = POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH;
  state.native_return.supported_flags =
    POLY_NATIVE_RETURN_FRAME_FLAGS_SUPPORTED;

  asm volatile(POLY_OP_STATE_IMPORT :: "a"(&state) : "r15", "memory");
}

static void clear_poly_cpu_state(void) {
  clear_poly_xsave_state();

  uint64_t value = 0;
  asm volatile(POLY_OP_TRAP_VECTOR_SET : "+a"(value) :: "memory");
  value = 0;
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET : "+a"(value) :: "memory");
  value = 0;
  asm volatile(POLY_OP_STATE_KEY_SET : "+a"(value) :: "memory");

  for (uint64_t slot = 0; slot < POLY_ABI_SIGNATURE_SLOT_COUNT; slot++) {
    uint64_t slot_arg = slot;
    uint64_t kind_arg = POLY_ABI_SIGNATURE_KIND_EXCHANGE;
    asm volatile(POLY_OP_ABI_SIGNATURE_SET
      : "+a"(slot_arg), "+d"(kind_arg)
      :
      : "memory");
  }

  uint64_t address = 0;
  uint64_t bytes = 0;
  asm volatile(POLY_OP_EVENT_PTR_SET
    : "+a"(address), "+d"(bytes)
    :
    : "memory");
}

static void load_env_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file)
    return;

  char line[512];
  while (fgets(line, sizeof(line), file)) {
    char *start = line;
    while (*start == ' ' || *start == '\t')
      start++;
    if (*start == '\0' || *start == '\n' || *start == '#')
      continue;
    char *end = start + strlen(start);
    while (end > start && (end[-1] == '\n' || end[-1] == '\r' ||
        end[-1] == ' ' || end[-1] == '\t'))
      *--end = '\0';
    char *equals = strchr(start, '=');
    if (!equals || equals == start)
      continue;
    *equals = '\0';
    const char *existing = getenv(start);
    if (!existing || existing[0] == '\0')
      setenv(start, equals + 1, 1);
  }
  fclose(file);
}

static int env_nonempty(const char *name) {
  const char *value = getenv(name);
  return value && value[0] != '\0';
}

static int env_enabled(const char *name) {
  const char *value = getenv(name);
  return value && value[0] != '\0' && strcmp(value, "0") != 0;
}

static void set_default_library_path(void) {
  if (env_nonempty("POLY_LD_LIBRARY_PATH") || env_nonempty("LD_LIBRARY_PATH"))
    return;

  /*
   * Nested container launchers can scrub the environment before invoking the
   * foreign OCI runtime. Keep the binfmt process-mode loader usable for normal
   * root filesystems without requiring every launcher to propagate Poly vars.
   */
  setenv("POLY_LD_LIBRARY_PATH", "/lib:/usr/lib", 1);
}

static int path_is_supported_foreign_elf(const char *path) {
  if (path == NULL || path[0] == '\0')
    return 0;

  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return 0;

  unsigned char ident[EI_NIDENT];
  ssize_t got = read(fd, ident, sizeof(ident));
  if (got != (ssize_t) sizeof(ident) ||
      ident[EI_MAG0] != ELFMAG0 ||
      ident[EI_MAG1] != ELFMAG1 ||
      ident[EI_MAG2] != ELFMAG2 ||
      ident[EI_MAG3] != ELFMAG3 ||
      ident[EI_CLASS] != ELFCLASS64 ||
      ident[EI_DATA] != ELFDATA2LSB) {
    close(fd);
    return 0;
  }

  if (lseek(fd, 0, SEEK_SET) != 0) {
    close(fd);
    return 0;
  }
  Elf64_Ehdr ehdr;
  got = read(fd, &ehdr, sizeof(ehdr));
  close(fd);
  if (got != (ssize_t) sizeof(ehdr))
    return 0;
  return ehdr.e_machine == EM_AARCH64 || ehdr.e_machine == EM_RISCV;
}

static FILE *open_trace_file(void) {
  int fd = open("/dev/ttyS0", O_WRONLY | O_CLOEXEC | O_NOCTTY);
  if (fd < 0)
    return stderr;
  FILE *file = fdopen(fd, "w");
  if (!file) {
    close(fd);
    return stderr;
  }
  return file;
}

static void close_trace_file(FILE *file) {
  if (file && file != stderr)
    fclose(file);
}

static void log_fd_kind(FILE *file, int fd) {
  struct stat st;
  if (fstat(fd, &st) != 0) {
    fprintf(file, " fd%d=err:%s", fd, strerror(errno));
    return;
  }
  if (S_ISFIFO(st.st_mode))
    fprintf(file, " fd%d=pipe", fd);
  else if (S_ISCHR(st.st_mode))
    fprintf(file, " fd%d=chr:%u:%u", fd, (unsigned) major(st.st_rdev),
      (unsigned) minor(st.st_rdev));
  else if (S_ISREG(st.st_mode))
    fprintf(file, " fd%d=reg", fd);
  else
    fprintf(file, " fd%d=mode:%o", fd, (unsigned) (st.st_mode & S_IFMT));
}

static void log_env_value(FILE *file, const char *name) {
  const char *value = getenv(name);
  if (value && value[0] != '\0')
    fprintf(file, " %s=%s", name, value);
}

static void log_binfmt_argv(FILE *file, const char *label, int argc,
    char **argv) {
  fprintf(file, "%s: argc=%d", label, argc);
  for (int i = 0; i < argc; i++)
    fprintf(file, " [%d]=%s", i, argv[i] ? argv[i] : "(null)");
  fprintf(file, "\n");
}

static void log_binfmt_fds(FILE *file) {
  fprintf(file, "POLYBINFMT_FDS:");
  for (int fd = 0; fd <= 20; fd++)
    log_fd_kind(file, fd);
  fprintf(file, "\n");
}

static void log_binfmt_env(FILE *file) {
  fprintf(file, "POLYBINFMT_ENV:");
  log_env_value(file, "_OCI_STARTPIPE");
  log_env_value(file, "_OCI_SYNCPIPE");
  log_env_value(file, "_OCI_ATTACHPIPE");
  log_env_value(file, "POLYEXEC_STDOUT_PASSTHROUGH");
  log_env_value(file, "POLYEXEC_PROCESS_NO_FORK");
  log_env_value(file, "POLY_LD_LIBRARY_PATH");
  log_env_value(file, "LD_LIBRARY_PATH");
  fprintf(file, "\n");
}

int main(int argc, char **argv) {
  clear_poly_cpu_state();

  setenv("POLY_PROCESS_REAL_INTERPRETER", "1", 0);
  setenv("POLYEXEC_STDOUT_PASSTHROUGH", "1", 0);
  load_env_file("/etc/polyexec-binfmt.env");
  set_default_library_path();

  const int trace = env_enabled("POLYBINFMT_TRACE");
  FILE *trace_file = NULL;
  if (trace) {
    trace_file = open_trace_file();
    log_binfmt_argv(trace_file, "POLYBINFMT_ARGV", argc, argv);
    log_binfmt_fds(trace_file);
    log_binfmt_env(trace_file);
  }

  if (argc < 2 || argv[1] == NULL || argv[1][0] == '\0') {
    fprintf(stderr, "POLYBINFMT_EXEC_FAIL: missing foreign ELF path\n");
    return 2;
  }

  int path_index = 1;
  int argv0_index = argc > 2 ? 2 : -1;
  /*
   * The binfmt_misc P flag preserves the foreign argv0 after the path.
   * Some container re-exec paths are easier to diagnose if we defensively
   * validate the image argument instead of blindly trusting argv positions.
   */
  if (!path_is_supported_foreign_elf(argv[path_index]) && argc > 2 &&
      path_is_supported_foreign_elf(argv[2])) {
    path_index = 2;
    argv0_index = 1;
  }
  const char *path = argv[path_index];
  if (argv0_index > 0 && argv[argv0_index] != NULL &&
      argv[argv0_index][0] != '\0')
    setenv("POLYEXEC_GUEST_ARGV0", argv[argv0_index], 1);

  const int first_arg = argc > 2 ? 3 : 2;

  const int forwarded = argc - first_arg;
  char **exec_argv = calloc((size_t) forwarded + 4, sizeof(*exec_argv));
  if (!exec_argv) {
    fprintf(stderr, "POLYBINFMT_EXEC_FAIL: allocation failed\n");
    return 1;
  }

  int out = 0;
  exec_argv[out++] = (char *) "/usr/bin/polyexec";
  exec_argv[out++] = (char *) "--process";
  exec_argv[out++] = (char *) path;
  for (int i = first_arg; i < argc; i++)
    exec_argv[out++] = argv[i];
  exec_argv[out] = NULL;

  if (trace) {
    fprintf(trace_file,
      "POLYBINFMT_EXECV: path_index=%d argv0_index=%d path=%s forwarded=%d",
      path_index, argv0_index, path, forwarded);
    for (int i = 0; exec_argv[i]; i++)
      fprintf(trace_file, " argv%d=%s", i, exec_argv[i]);
    fprintf(trace_file, "\n");
    log_binfmt_fds(trace_file);
    log_binfmt_env(trace_file);
    close_trace_file(trace_file);
    trace_file = NULL;
  }

  execv(exec_argv[0], exec_argv);
  fprintf(stderr, "POLYBINFMT_EXEC_FAIL: execv %s: %s\n",
    exec_argv[0], strerror(errno));
  free(exec_argv);
  return 127;
}
