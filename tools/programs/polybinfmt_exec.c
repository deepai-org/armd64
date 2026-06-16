#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/polycpuid.h"

#define POLY_OP_TRAP_VECTOR_SET POLY_X86_CTRL_TRAP_VECTOR_SET_ASM
#define POLY_OP_TRAP_VECTOR_MODE_SET POLY_X86_CTRL_TRAP_VECTOR_MODE_SET_ASM
#define POLY_OP_STATE_KEY_SET POLY_X86_CTRL_STATE_KEY_SET_ASM
#define POLY_OP_STATE_EXPORT POLY_X86_CTRL_STATE_EXPORT_ASM
#define POLY_OP_STATE_IMPORT POLY_X86_CTRL_STATE_IMPORT_ASM
#define POLY_OP_ABI_SIGNATURE_SET POLY_X86_CTRL_ABI_SIGNATURE_SET_ASM
#define POLY_OP_EVENT_PTR_SET POLY_X86_CTRL_EVENT_PTR_SET_ASM
#define POLY_OP_SPILL_DESC_SET POLY_X86_CTRL_SPILL_DESC_SET_ASM

static void clear_poly_xsave_state(void) {
  struct poly_xsave_state state __attribute__((aligned(POLY_STATE_XSAVE_ALIGN_ARCH)));
  memset(&state, 0, sizeof(state));

  asm volatile(POLY_OP_STATE_EXPORT :: "a"(&state) : "r15", "memory");
  if (state.header.magic != POLY_STATE_XSAVE_MAGIC ||
      state.header.total_bytes != POLY_STATE_XSAVE_BYTES_ARCH ||
      state.header.header_bytes != POLY_STATE_XSAVE_HEADER_BYTES ||
      state.header.layout_version != POLY_STATE_XSAVE_LAYOUT_VERSION)
    return;

  memset(&state.trap, 0, sizeof(state.trap));
  memset(state.trap_args, 0, sizeof(state.trap_args));
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
  state.header.monitor_packet_addr = 0;

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
  address = 0;
  bytes = 0;
  asm volatile(POLY_OP_SPILL_DESC_SET
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
    setenv(start, equals + 1, 0);
  }
  fclose(file);
}

static void log_fd_kind(int fd) {
  struct stat st;
  if (fstat(fd, &st) != 0) {
    fprintf(stderr, " fd%d=err:%s", fd, strerror(errno));
    return;
  }
  if (S_ISFIFO(st.st_mode))
    fprintf(stderr, " fd%d=pipe", fd);
  else if (S_ISCHR(st.st_mode))
    fprintf(stderr, " fd%d=chr:%u:%u", fd, (unsigned) major(st.st_rdev),
      (unsigned) minor(st.st_rdev));
  else if (S_ISREG(st.st_mode))
    fprintf(stderr, " fd%d=reg", fd);
  else
    fprintf(stderr, " fd%d=mode:%o", fd, (unsigned) (st.st_mode & S_IFMT));
}

int main(int argc, char **argv) {
  clear_poly_cpu_state();

  const int trace = getenv("POLYBINFMT_TRACE") != NULL;
  if (trace) {
    fprintf(stderr, "POLYBINFMT_ARGV: argc=%d", argc);
    for (int i = 0; i < argc; i++)
      fprintf(stderr, " [%d]=%s", i, argv[i] ? argv[i] : "(null)");
    fprintf(stderr, "\n");
  }

  if (argc < 2 || argv[1] == NULL || argv[1][0] == '\0') {
    fprintf(stderr, "POLYBINFMT_EXEC_FAIL: missing foreign ELF path\n");
    return 2;
  }

  const char *path = argv[1];
  int first_arg = 2;
  /*
   * The binfmt_misc P flag preserves the foreign argv0 after the path.
   * polyexec --process synthesizes argv0 from the executable path, so only
   * forward the real arguments that follow the preserved argv0.
   */
  if (first_arg < argc)
    first_arg++;

  const int forwarded = argc - first_arg;
  char **exec_argv = calloc((size_t) forwarded + 4, sizeof(*exec_argv));
  if (!exec_argv) {
    fprintf(stderr, "POLYBINFMT_EXEC_FAIL: allocation failed\n");
    return 1;
  }

  setenv("POLY_PROCESS_REAL_INTERPRETER", "1", 0);
  load_env_file("/etc/polyexec-binfmt.env");

  int out = 0;
  exec_argv[out++] = (char *) "/usr/bin/polyexec";
  exec_argv[out++] = (char *) "--process";
  exec_argv[out++] = (char *) path;
  for (int i = first_arg; i < argc; i++)
    exec_argv[out++] = argv[i];
  exec_argv[out] = NULL;

  if (trace) {
    fprintf(stderr, "POLYBINFMT_EXECV: path=%s", path);
    log_fd_kind(STDIN_FILENO);
    log_fd_kind(STDOUT_FILENO);
    log_fd_kind(STDERR_FILENO);
    fprintf(stderr, "\n");
  }

  execv(exec_argv[0], exec_argv);
  fprintf(stderr, "POLYBINFMT_EXEC_FAIL: execv %s: %s\n",
    exec_argv[0], strerror(errno));
  free(exec_argv);
  return 127;
}
