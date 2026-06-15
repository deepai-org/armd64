#define _GNU_SOURCE

#include <errno.h>
#include <elf.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <ucontext.h>
#include <sys/vfs.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../include/polycpuid.h"
#include "../include/polyruntime_imports.h"
#include "abi/poly_abi_legacy_bridge.h"
#include "bridge/poly_process_bridge_kind.h"

extern char **environ;

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

#ifndef MAP_POPULATE
#define MAP_POPULATE 0x8000
#endif

#ifndef MAP_STACK
#define MAP_STACK 0
#endif

#ifndef O_DIRECT
#define O_DIRECT 040000
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#ifndef O_NOATIME
#ifdef __O_NOATIME
#define O_NOATIME __O_NOATIME
#else
#define O_NOATIME 0
#endif
#endif

#ifndef O_PATH
#ifdef __O_PATH
#define O_PATH __O_PATH
#else
#define O_PATH 0
#endif
#endif

#ifndef O_TMPFILE
#ifdef __O_TMPFILE
#define O_TMPFILE __O_TMPFILE
#else
#define O_TMPFILE 0
#endif
#endif

#define POLY_OP_TRAP_VECTOR_SET POLY_X86_CTRL_TRAP_VECTOR_SET_ASM
#define POLY_OP_TRAP_VECTOR_MODE_SET POLY_X86_CTRL_TRAP_VECTOR_MODE_SET_ASM
#define POLY_OP_TRAP_RETURN POLY_X86_CTRL_TRAP_RETURN_ASM
#define POLY_OP_STATE_KEY_SET POLY_X86_CTRL_STATE_KEY_SET_ASM
#define POLY_OP_STATE_KEY_GET POLY_X86_CTRL_STATE_KEY_GET_ASM
#define POLY_OP_STATE_EXPORT POLY_X86_CTRL_STATE_EXPORT_ASM
#define POLY_OP_STATE_IMPORT POLY_X86_CTRL_STATE_IMPORT_ASM
#define POLY_OP_MONITOR_PACKET_SET POLY_X86_CTRL_MONITOR_PACKET_SET_ASM
#define POLY_OP_MONITOR_PACKET_GET POLY_X86_CTRL_MONITOR_PACKET_GET_ASM
#define POLY_OP_ABI_SIGNATURE_SET POLY_X86_CTRL_ABI_SIGNATURE_SET_ASM
#define POLY_OP_AUTO_SPILL_COUNT_STATUS \
  POLY_X86_CTRL_AUTO_SPILL_COUNT_STATUS_ASM
#define POLY_OP_AUTO_SPILL_BYTES_STATUS \
  POLY_X86_CTRL_AUTO_SPILL_BYTES_STATUS_ASM
#define POLY_OP_AUTO_SPILL_CYCLES_STATUS \
  POLY_X86_CTRL_AUTO_SPILL_CYCLES_STATUS_ASM
#define POLY_OP_PRESTORE POLY_X86_CTRL_PRESTORE_ASM
#define POLY_OP_EVENT_PTR_SET POLY_X86_CTRL_EVENT_PTR_SET_ASM
#define POLY_OP_SPILL_DESC_SET POLY_X86_CTRL_SPILL_DESC_SET_ASM

#define POLYEXEC_SYSCALL_SUMMARY_MAX 512
#define POLY_AUTO_SPILL_RESUME_STACK_SIZE (64U * 1024U)
#define POLY_RUNTIME_SIGNAL_ALT_STACK_SIZE (64U * 1024U)
#define POLY_TRAP_VECTOR_STACK_SIZE (64U * 1024U)
#define POLY_TRAP_VECTOR_STACK_COUNT 8U
#define POLY_AUTO_SPILL_RESUME_STACK_PTR_OFFSET \
  POLY_STATE_XSAVE_RESERVED_OFFSET
#define POLY_V2_EVENT_MAGIC \
  ((((uint64_t) POLY_V2_EVENT_MAGIC_HI) << 32) | POLY_V2_EVENT_MAGIC_LO)
#define POLY_V2_SPILL_DESC_MAGIC \
  ((((uint64_t) POLY_V2_SPILL_DESC_MAGIC_HI) << 32) | \
    POLY_V2_SPILL_DESC_MAGIC_LO)

#ifndef TIOCGWINSZ
#define TIOCGWINSZ 0x5413
#endif

#ifndef R_AARCH64_NONE
#define R_AARCH64_NONE 0
#endif

#ifndef R_AARCH64_RELATIVE
#define R_AARCH64_RELATIVE 1027
#endif

#ifndef R_AARCH64_ABS64
#define R_AARCH64_ABS64 257
#endif

#ifndef R_AARCH64_GLOB_DAT
#define R_AARCH64_GLOB_DAT 1025
#endif

#ifndef R_AARCH64_JUMP_SLOT
#define R_AARCH64_JUMP_SLOT 1026
#endif

#ifndef R_AARCH64_COPY
#define R_AARCH64_COPY 1024
#endif

#ifndef R_AARCH64_IRELATIVE
#define R_AARCH64_IRELATIVE 1032
#endif

#ifndef R_AARCH64_TLS_DTPMOD64
#define R_AARCH64_TLS_DTPMOD64 1028
#endif

#ifndef R_AARCH64_TLS_DTPREL64
#define R_AARCH64_TLS_DTPREL64 1029
#endif

#ifndef R_AARCH64_TLS_TPREL64
#define R_AARCH64_TLS_TPREL64 1030
#endif

#ifndef R_AARCH64_TLSDESC
#define R_AARCH64_TLSDESC 1031
#endif

#ifndef R_RISCV_NONE
#define R_RISCV_NONE 0
#endif

#ifndef R_RISCV_RELATIVE
#define R_RISCV_RELATIVE 3
#endif

#ifndef R_RISCV_COPY
#define R_RISCV_COPY 4
#endif

#ifndef R_RISCV_64
#define R_RISCV_64 2
#endif

#ifndef R_RISCV_JUMP_SLOT
#define R_RISCV_JUMP_SLOT 5
#endif

#ifndef R_RISCV_TLS_DTPMOD64
#define R_RISCV_TLS_DTPMOD64 7
#endif

#ifndef R_RISCV_TLS_DTPREL64
#define R_RISCV_TLS_DTPREL64 9
#endif

#ifndef R_RISCV_IRELATIVE
#define R_RISCV_IRELATIVE 58
#endif

#ifndef R_RISCV_TLS_TPREL64
#define R_RISCV_TLS_TPREL64 11
#endif

#ifndef R_X86_64_NONE
#define R_X86_64_NONE 0
#endif

#ifndef R_X86_64_64
#define R_X86_64_64 1
#endif

#ifndef R_X86_64_COPY
#define R_X86_64_COPY 5
#endif

#ifndef R_X86_64_GLOB_DAT
#define R_X86_64_GLOB_DAT 6
#endif

#ifndef R_X86_64_JUMP_SLOT
#define R_X86_64_JUMP_SLOT 7
#endif

#ifndef R_X86_64_RELATIVE
#define R_X86_64_RELATIVE 8
#endif

#ifndef R_X86_64_DTPMOD64
#define R_X86_64_DTPMOD64 16
#endif

#ifndef R_X86_64_DTPOFF64
#define R_X86_64_DTPOFF64 17
#endif

#ifndef R_X86_64_TPOFF64
#define R_X86_64_TPOFF64 18
#endif

#ifndef R_X86_64_IRELATIVE
#define R_X86_64_IRELATIVE 37
#endif

#ifndef DT_RELR
#define DT_RELR 36
#endif

#ifndef DT_RELRSZ
#define DT_RELRSZ 35
#endif

#ifndef DT_RELRENT
#define DT_RELRENT 37
#endif

#ifndef DT_GNU_HASH
#define DT_GNU_HASH 0x6ffffef5
#endif

#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 0x0001
#endif

#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif

#ifndef AT_SYSINFO_EHDR
#define AT_SYSINFO_EHDR 33
#endif

#ifndef ARCH_SET_FS
#define ARCH_SET_FS 0x1002
#endif

#ifndef ARCH_GET_FS
#define ARCH_GET_FS 0x1003
#endif

#define POLY_RISCV_SYS_HWPROBE 258
#define POLY_RISCV_HWPROBE_KEY_MVENDORID 0
#define POLY_RISCV_HWPROBE_KEY_MARCHID 1
#define POLY_RISCV_HWPROBE_KEY_MIMPID 2
#define POLY_RISCV_HWPROBE_KEY_BASE_BEHAVIOR 3
#define POLY_RISCV_HWPROBE_BASE_BEHAVIOR_IMA (1ULL << 0)
#define POLY_RISCV_HWPROBE_KEY_IMA_EXT_0 4
#define POLY_RISCV_HWPROBE_IMA_FD (1ULL << 0)
#define POLY_RISCV_HWPROBE_IMA_C (1ULL << 1)
#define POLY_RISCV_HWPROBE_EXT_ZBA (1ULL << 3)
#define POLY_RISCV_HWPROBE_EXT_ZBB (1ULL << 4)
#define POLY_RISCV_HWPROBE_EXT_ZBS (1ULL << 5)
#define POLY_RISCV_HWPROBE_EXT_ZICOND (1ULL << 35)
#define POLY_RISCV_HWPROBE_EXT_ZICNTR (1ULL << 50)
#define POLY_RISCV_HWPROBE_KEY_CPUPERF_0 5

#define POLY_AARCH64_O_DIRECTORY 040000ULL
#define POLY_AARCH64_O_NOFOLLOW 0100000ULL
#define POLY_AARCH64_O_DIRECT 0200000ULL
#define POLY_AARCH64_O_LARGEFILE 0400000ULL
#define POLY_AARCH64_O_NOATIME 01000000ULL
#define POLY_AARCH64_O_CLOEXEC 02000000ULL
#define POLY_AARCH64_O_PATH 010000000ULL
#define POLY_AARCH64___O_TMPFILE 020000000ULL
#define POLY_AARCH64_O_TMPFILE \
  (POLY_AARCH64___O_TMPFILE | POLY_AARCH64_O_DIRECTORY)
#define POLY_RISCV_HWPROBE_KEY_ZICBOZ_BLOCK_SIZE 6
#define POLY_RISCV_HWPROBE_KEY_HIGHEST_VIRT_ADDRESS 7
#define POLY_RISCV_HWPROBE_KEY_TIME_CSR_FREQ 8
#define POLY_RISCV_HWPROBE_KEY_MISALIGNED_SCALAR_PERF 9
#define POLY_RISCV_HWPROBE_KEY_MISALIGNED_VECTOR_PERF 10
#define POLY_RISCV_HWPROBE_KEY_VENDOR_EXT_THEAD_0 11
#define POLY_RISCV_HWPROBE_KEY_ZICBOM_BLOCK_SIZE 12
#define POLY_RISCV_HWPROBE_KEY_VENDOR_EXT_SIFIVE_0 13

#define POLY_AARCH64_HWCAP_FP (1ULL << 0)
#define POLY_AARCH64_HWCAP_ASIMD (1ULL << 1)
#define POLY_AARCH64_HWCAP_ATOMICS (1ULL << 8)
#define POLY_AARCH64_HWCAP_CPUID (1ULL << 11)

#define POLY_RISCV_HWCAP_ISA_A (1ULL << ('A' - 'A'))
#define POLY_RISCV_HWCAP_ISA_C (1ULL << ('C' - 'A'))
#define POLY_RISCV_HWCAP_ISA_D (1ULL << ('D' - 'A'))
#define POLY_RISCV_HWCAP_ISA_F (1ULL << ('F' - 'A'))
#define POLY_RISCV_HWCAP_ISA_I (1ULL << ('I' - 'A'))
#define POLY_RISCV_HWCAP_ISA_M (1ULL << ('M' - 'A'))

#ifndef STT_GNU_IFUNC
#define STT_GNU_IFUNC 10
#endif

#ifndef DT_PLTRELSZ
#define DT_PLTRELSZ 2
#endif

#ifndef DT_PLTREL
#define DT_PLTREL 20
#endif

#ifndef DT_JMPREL
#define DT_JMPREL 23
#endif

#ifndef DT_RUNPATH
#define DT_RUNPATH 29
#endif

#ifndef DT_RPATH
#define DT_RPATH 15
#endif

#ifndef DT_PREINIT_ARRAY
#define DT_PREINIT_ARRAY 32
#endif

#ifndef DT_PREINIT_ARRAYSZ
#define DT_PREINIT_ARRAYSZ 33
#endif

#ifndef DT_VERSYM
#define DT_VERSYM 0x6ffffff0
#endif

#ifndef DT_VERDEF
#define DT_VERDEF 0x6ffffffc
#endif

#ifndef DT_VERDEFNUM
#define DT_VERDEFNUM 0x6ffffffd
#endif

#ifndef DT_VERNEED
#define DT_VERNEED 0x6ffffffe
#endif

#ifndef DT_VERNEEDNUM
#define DT_VERNEEDNUM 0x6fffffff
#endif

enum {
  POLY_ARCH_X86 = POLY_FRONTEND_X86,
  POLY_ARCH_AARCH64 = POLY_FRONTEND_AARCH64,
  POLY_ARCH_RISCV = POLY_FRONTEND_RISCV,
  POLY_ARCH_COUNT = 3,
  POLY_X86_CONTROL_OPCODE_SIZE = POLY_X86_CTRL_TOTAL_BYTES,
  POLY_X86_PENTER_BASE_SIZE = POLY_X86_CTRL_PENTER_FRONTEND_TOTAL_BYTES,
  POLY_X86_TRAMPOLINE_SIZE = 14,
  MAX_PROGRAM_BYTES = 128 * 1024 * 1024,
  MAX_LOAD_SEGMENTS = 16,
  MAX_PROCESS_DEPS = 8,
  MAX_PROCESS_DEP_DEPTH = 4,
  MAX_PROCESS_BRIDGE_SPECS = 16,
  MAX_DEP_PATH = 160,
  MAX_PROCESS_TLS_BYTES = 64 * 1024,
  PROCESS_CROSS_STUB_BYTES = 64 * 1024,
  SCRATCH_SECOND_PATH_OFFSET = 128
};

struct poly_load_segment {
  uint64_t vaddr;
  uint64_t memsz;
  uint32_t flags;
};

struct poly_program;

struct poly_process_bridge_spec {
  char symbol[80];
  uint8_t bridge_kind;
};

struct poly_process_dependency {
  char path[MAX_DEP_PATH];
  struct poly_program *program;
  struct poly_process_dependency *shared_from;
  uint8_t *mapping;
  size_t mapping_size;
  uint8_t *loaded_image;
};

struct poly_program {
  const char *path;
  const char *arch_name;
  int arch;
  int is_et_exec;
  uint64_t base_vaddr;
  size_t entry_offset;
  uint64_t phdr_vaddr;
  uint16_t phent;
  uint16_t phnum;
  int has_interp;
  char interp_path[160];
  size_t dynamic_offset;
  size_t dynamic_size;
  struct poly_load_segment load_segments[MAX_LOAD_SEGMENTS];
  size_t load_segment_count;
  uint64_t relro_vaddr;
  uint64_t relro_size;
  uint64_t tls_vaddr;
  uint64_t tls_filesz;
  uint64_t tls_memsz;
  uint64_t tls_align;
  size_t tls_offset;
  size_t tls_total_size;
  uint64_t init_vaddr;
  uint64_t preinit_array_vaddr;
  uint64_t preinit_array_size;
  uint64_t init_array_vaddr;
  uint64_t init_array_size;
  uint64_t fini_vaddr;
  uint64_t fini_array_vaddr;
  uint64_t fini_array_size;
  uint8_t *code_bytes;
  size_t code_size;
  char soname[MAX_DEP_PATH];
  struct poly_process_bridge_spec bridge_specs[MAX_PROCESS_BRIDGE_SPECS];
  size_t bridge_spec_count;
  struct poly_process_dependency deps[MAX_PROCESS_DEPS];
  size_t dep_count;
  const struct poly_program *scope_root_program;
  uint8_t *scope_root_loaded_image;
};

struct poly_cross_stub_arena {
  uint8_t *mapping;
  size_t size;
  size_t offset;
};

static struct poly_cross_stub_arena process_cross_stubs;
static uint64_t process_runtime_x86_tls_base;
static uint64_t process_runtime_host_fs_base;
static int process_runtime_needs_x86_tls_wrapper;
static size_t process_cross_state_key_stub_count;
static size_t process_cross_aarch64_to_riscv_stub_count;
static size_t process_cross_riscv_to_aarch64_stub_count;
static size_t process_cross_aarch64_to_x86_stub_count;
static size_t process_cross_riscv_to_x86_stub_count;
static size_t process_cross_x86_to_aarch64_stub_count;
static size_t process_cross_x86_to_riscv_stub_count;
static size_t process_cross_signature_slot_stub_count;
static size_t process_cross_register_signature_stub_count;
static size_t process_cross_stack_bridge_stub_count;
static size_t process_cross_compact_shuffle_stub_count;
static size_t process_cross_x86_wrapper_stub_count;
static int process_cross_state_key_stub_reported;
static int polyexec_use_explicit_state_key;
static int polyexec_use_auto_spill;
static int polyexec_dump_maps_on_fault = -1;
static int polyexec_dump_aarch64_node_chain_on_fault = -1;
static int polyexec_protect_runtime_signals = -1;
static const char *process_cross_report_path;
static __thread uint64_t poly_mode_x86_saved_rsp;
static __thread struct poly_xsave_state poly_auto_spill_state
  __attribute__((aligned(POLY_STATE_XSAVE_ALIGN_ARCH)));
static __thread struct poly_xsave_state *poly_auto_spill_state_active;
static __thread struct poly_v2_event_frame poly_auto_spill_event_frame
  __attribute__((aligned(POLY_V2_EVENT_ALIGN)));
static __thread struct poly_v2_spill_descriptor poly_auto_spill_descriptor
  __attribute__((aligned(POLY_V2_SPILL_DESC_ALIGN)));
static __thread uint64_t poly_v2_last_event_sequence;
static __thread volatile sig_atomic_t poly_auto_spill_installed;
static __thread volatile sig_atomic_t poly_auto_spill_signal_signo;
static __thread volatile sig_atomic_t poly_auto_spill_signal_code;
static __thread volatile uintptr_t poly_auto_spill_signal_addr;
uint8_t poly_auto_spill_resume_stack[POLY_AUTO_SPILL_RESUME_STACK_SIZE]
  __attribute__((used, aligned(16)));
static __thread uint8_t
  poly_runtime_signal_alt_stack[POLY_RUNTIME_SIGNAL_ALT_STACK_SIZE]
  __attribute__((aligned(16)));
static __thread int poly_runtime_signal_alt_stack_installed;
static volatile uint32_t poly_auto_spill_resume_stack_lock;
static volatile uint64_t poly_auto_spill_resume_mode;
uint8_t poly_trap_vector_stacks[POLY_TRAP_VECTOR_STACK_COUNT]
  [POLY_TRAP_VECTOR_STACK_SIZE]
  __attribute__((used, aligned(16)));
static volatile uint32_t
  poly_trap_vector_stack_locks[POLY_TRAP_VECTOR_STACK_COUNT];
struct poly_auto_spill_resume_info {
  uint64_t buffer;
  uint64_t mode;
};
static __thread volatile uint64_t *poly_thread_atomic_counter;
static __thread uint64_t poly_thread_atomic_iterations;
static __thread uint64_t poly_thread_atomic_index;
static __thread uint64_t poly_thread_atomic_count;

struct poly_request {
  char path[160];
  char symbol[80];
  uint64_t expected;
  int check_expected;
};

struct poly_thread_run_context {
  struct poly_request request;
  volatile int *start_flag;
  int use_trap_vector;
  int atomic_mode;
  uintptr_t index;
  size_t thread_count;
  uint64_t atomic_iterations;
  volatile uint64_t *atomic_counter;
  volatile long native_tid;
  uint64_t result;
  uint64_t spill_buffer;
  int status;
};

struct poly_atomic_migrator_context {
  struct poly_thread_run_context *contexts;
  size_t thread_count;
  volatile int *done_flag;
  unsigned long allowed_mask;
  unsigned int cpu_count;
  uint64_t migrations;
  int status;
};

static uint32_t process_native_signature_slot = 3;
static uint32_t process_fp64_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_FP64;
static uint32_t process_fp32_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_NATIVE_REGS_FP32;
static uint32_t process_sret_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_SRET_X86_SYSV_REGS;
static uint32_t process_native_sret_signature_slot =
  POLY_ABI_SIGNATURE_SLOT_NATIVE_SRET_REGS;
static uint32_t process_vec128_signature_slot = 5;
static uint32_t process_compact_u32_f32_signature_slot = 6;
static uint32_t process_compact_f32_u32_signature_slot = 7;
static __thread uint8_t poly_state_key_anchor;
static __thread volatile uint64_t poly_monitor_packet[16]
  __attribute__((aligned(64)));
static volatile uint64_t poly_monitor_packet_count;
static volatile uint64_t poly_monitor_packet_syscall_aarch64_count;
static volatile uint64_t poly_monitor_packet_syscall_riscv_count;
static volatile uint64_t poly_monitor_packet_break_aarch64_count;
static volatile uint64_t poly_monitor_packet_break_riscv_count;
static volatile uint64_t poly_monitor_packet_import_count;
static volatile uint64_t poly_monitor_packet_illegal_count;
static volatile uint64_t poly_monitor_packet_other_count;
static volatile uint64_t poly_process_terminal_exit_code;
static uint8_t *process_brk_mapping;
static size_t process_brk_mapping_size;
static uint64_t process_brk_current;
static int polyexec_trace_syscalls = -1;
static int polyexec_trace_postgres_syscalls = -1;
static int polyexec_trace_trap_returns = -1;
static int polyexec_trace_protected_signal_waits = -1;
static int polyexec_disable_io_uring = -1;
static int polyexec_disable_rseq = -1;
static int polyexec_virtual_sigchld = -1;
static int polyexec_syscall_summary = -1;
static int polyexec_prefault_guest_mmaps = -1;
static unsigned int polyexec_watchdog_seconds;
static int polyexec_watchdog_signal;
static timer_t polyexec_watchdog_timer;
static int polyexec_stdout_passthrough = -1;
static int polyexec_saved_stdout = -1;
static int poly_trap_vector_active;
static __thread uint64_t poly_guest_sigmask_shadow;
static __thread int poly_guest_sigmask_shadow_valid;
static uint64_t poly_aarch64_vdso_rt_sigreturn;
static volatile sig_atomic_t poly_pending_virtual_signal_mask;
static __thread uint64_t poly_aarch64_active_signal_frame;
static __thread uint64_t poly_aarch64_guest_sigaltstack_sp;
static __thread uint64_t poly_aarch64_guest_sigaltstack_size;
static __thread int32_t poly_aarch64_guest_sigaltstack_flags = 2;
static __thread uint64_t poly_riscv_guest_sigaltstack_sp;
static __thread uint64_t poly_riscv_guest_sigaltstack_size;
static __thread int32_t poly_riscv_guest_sigaltstack_flags = 2;
static __thread struct poly_xsave_state poly_trap_return_state
  __attribute__((aligned(POLY_STATE_XSAVE_ALIGN_ARCH)));
static uint64_t poly_syscall_summary_aarch64[POLYEXEC_SYSCALL_SUMMARY_MAX];
static uint64_t poly_syscall_summary_x86[POLYEXEC_SYSCALL_SUMMARY_MAX];
static uint64_t poly_syscall_summary_error_aarch64[POLYEXEC_SYSCALL_SUMMARY_MAX];
static uint64_t poly_syscall_summary_total;
static uint64_t poly_syscall_summary_errors;
static int poly_tiocgwinsz_stdin_enotty_cached;
static uint64_t poly_tiocgwinsz_stdin_cache_hits;
static uint64_t poly_tiocgwinsz_stdin_cache_misses;

static int poly_prefault_guest_mmaps_enabled(void);

struct poly_runtime_trap_packet {
  uint64_t reason;
  uint64_t mode;
  uint64_t number;
  uint64_t selector;
  uint64_t pc;
  uint64_t next_pc;
  uint64_t flags;
  uint64_t reserved[2];
  uint64_t args[8];
};

struct poly_clone_child_handoff {
  struct poly_runtime_trap_packet packet;
  struct poly_xsave_state state
    __attribute__((aligned(POLY_STATE_XSAVE_ALIGN_ARCH)));
  volatile uint64_t monitor_packet[16] __attribute__((aligned(64)));
  uint8_t state_key_anchor;
  void *native_tls_mapping;
  size_t native_tls_mapping_size;
  uint64_t native_tls_base;
  void *mapping;
  size_t mapping_size;
  uint64_t foreign_stack;
  uint64_t foreign_tls;
};

struct poly_linux_ksigaction {
  uint64_t handler;
  uint64_t flags;
  uint64_t restorer;
  uint64_t mask;
};

#define POLY_AARCH64_VSIGFRAME_MAGIC 0x50534c4147534631ULL
#define POLY_AARCH64_GPR_VALID_MASK 0x7fffffffULL
#define POLY_AARCH64_GPR_VALID_MASK_FULL 0xffffffffULL
#define POLY_AARCH64_NZCV_MASK 0x0fULL
#define POLY_AARCH64_FPCR_RMODE_MASK (3ULL << 22)
#define POLY_AARCH64_FPSR_MASK 0x9fULL
#define POLY_RISCV_FCSR_MASK 0xffULL
#define POLY_AARCH64_VSIG_STACK_SIZE 65536U
#define POLY_LINUX_SA_ONSTACK 0x08000000ULL
#define POLY_LINUX_SS_ONSTACK 1
#define POLY_LINUX_SS_DISABLE 2
#define POLY_AARCH64_MINSIGSTKSZ 2048U

struct poly_aarch64_siginfo {
  int32_t si_signo;
  int32_t si_errno;
  int32_t si_code;
  int32_t reserved0;
  uint64_t fault_addr;
  uint8_t reserved[104];
};

struct poly_aarch64_sigcontext {
  uint64_t fault_address;
  uint64_t regs[31];
  uint64_t sp;
  uint64_t pc;
  uint64_t pstate;
  uint64_t reserved_align;
  uint8_t reserved[4096];
};

struct poly_aarch64_ucontext {
  uint64_t uc_flags;
  uint64_t uc_link;
  uint64_t stack_sp;
  int32_t stack_flags;
  uint32_t stack_pad;
  uint64_t stack_size;
  uint64_t sigmask;
  uint8_t pad[(1024 - 64) / 8];
  uint8_t pad2[8];
  struct poly_aarch64_sigcontext mcontext;
};

struct poly_linux_stack_t {
  uint64_t sp;
  int32_t flags;
  uint32_t pad;
  uint64_t size;
};

struct poly_aarch64_virtual_signal_frame {
  uint64_t magic;
  uint64_t size;
  uint64_t signum;
  uint64_t handler_args[4];
  struct poly_aarch64_siginfo siginfo;
  struct poly_aarch64_ucontext ucontext;
  struct poly_trap_packet saved_trap;
  uint64_t saved_trap_args[POLY_TRAP_PACKET_ARG_COUNT];
  struct poly_trap_restore_state saved_trap_restore;
  struct poly_native_return_state saved_native_return;
  uint64_t aarch64_gpr[32];
  struct poly_u128 aarch64_fp[32];
  uint64_t aarch64_nzcv;
  uint64_t aarch64_fpcr;
  uint64_t aarch64_fpsr;
  uint64_t guest_sigmask;
  uint64_t interrupted_pc;
};

static struct poly_linux_ksigaction poly_guest_signal_action_shadow[65];
static unsigned char poly_guest_signal_action_shadow_valid[65];

struct poly_process_exit_finalizer_context {
  struct poly_program *program;
  uint8_t *loaded_image;
  uint8_t *trampoline_code;
  size_t prefix_size;
  uint64_t return_pc;
  uint8_t *scratch;
  int active;
  int run_finalizers;
  int running;
  int completed;
};

static struct poly_process_exit_finalizer_context
  poly_process_exit_finalizers;

static int run_process_exit_finalizers(void);
static void reset_process_brk_arena(void);
static void free_program(struct poly_program *program);

static int run_irelative_resolver(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint8_t *scratch, uint64_t resolver_vaddr,
    uint64_t *resolved);
static int emit_process_cross_isa_call_stub(int caller_arch, int callee_arch,
    uint64_t target, int bridge_kind, uint32_t signature_slot,
    uint64_t *stub_addr);
static void poly_auto_spill_resume_trampoline(void);
static void poly_trap_vector_handler(void);
static uint64_t get_x86_fs_base(void);
static uint64_t poly_trap_vector_return_result(uint64_t result,
    const struct poly_runtime_trap_packet *packet,
    const struct poly_xsave_state *trap_state, int has_trap_state);
static void poly_clear_native_return_state(struct poly_xsave_state *state);
static void poly_clear_return_transition_state(struct poly_xsave_state *state);
static int poly_guest_read_u64(uint64_t address, uint64_t *out);
static int poly_try_deliver_aarch64_signal(struct poly_xsave_state *state,
    uint64_t forced_signum, uint64_t fault_address, uint64_t fault_code);
static int poly_trace_protected_signal_waits_enabled(void);
static int poly_guest_range_is_mapped(uint64_t address, size_t length,
    int writable);
static void poly_prefault_range(uint64_t address, uint64_t length,
    int writable);

static inline void poly_mode_x86(void) {
  asm volatile(
    "movq %%rsp, %0\n"
    "xorl %%r15d, %%r15d\n"
    POLY_X86_CTRL_PENTER_MODE_ASM
    "movq %0, %%rsp\n"
    : "+m"(poly_mode_x86_saved_rsp)
    :
    :
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
    "r12", "r13", "r14", "r15", "memory");
}

static inline uint64_t poly_current_x86_rsp(void) {
  uint64_t rsp;
  asm volatile("movq %%rsp, %0" : "=r"(rsp) :: "memory");
  return rsp;
}

static void poly_sanitize_aarch64_trap_restore_payload(
    struct poly_trap_restore_state *restore) {
  restore->aarch64_nzcv &= POLY_AARCH64_NZCV_MASK;
  restore->aarch64_fpcr &= POLY_AARCH64_FPCR_RMODE_MASK;
  restore->aarch64_fpsr &= POLY_AARCH64_FPSR_MASK;
  restore->aarch64_reserved = 0;
  restore->riscv_gpr_valid_mask = 0;
  memset(restore->riscv_gpr, 0, sizeof(restore->riscv_gpr));
  memset(restore->riscv_fp, 0, sizeof(restore->riscv_fp));
  restore->riscv_fcsr = 0;
  memset(restore->riscv_reserved, 0, sizeof(restore->riscv_reserved));
}

static int poly_aarch64_trap_restore_gpr_mask_usable(uint64_t mask) {
  return mask == POLY_AARCH64_GPR_VALID_MASK ||
    mask == POLY_AARCH64_GPR_VALID_MASK_FULL;
}

static void poly_sanitize_riscv_trap_restore_payload(
    struct poly_trap_restore_state *restore) {
  restore->aarch64_gpr_valid_mask = 0;
  memset(restore->aarch64_gpr, 0, sizeof(restore->aarch64_gpr));
  memset(restore->aarch64_fp, 0, sizeof(restore->aarch64_fp));
  restore->aarch64_nzcv = 0;
  restore->aarch64_fpcr = 0;
  restore->aarch64_fpsr = 0;
  restore->aarch64_reserved = 0;
  restore->riscv_fcsr &= POLY_RISCV_FCSR_MASK;
  memset(restore->riscv_reserved, 0, sizeof(restore->riscv_reserved));
}

static uint64_t poly_state_key_set(uint64_t value) {
  asm volatile(POLY_OP_STATE_KEY_SET : "+a"(value) :: "memory");
  return value;
}

static uint64_t poly_state_key_get(void) {
  uint64_t value;
  asm volatile(POLY_OP_STATE_KEY_GET : "=a"(value) :: "memory");
  return value;
}

static inline void poly_state_export(struct poly_xsave_state *state) {
  asm volatile(POLY_OP_STATE_EXPORT :: "a"(state) : "r15", "memory");
}

static inline void poly_state_import(const struct poly_xsave_state *state) {
  asm volatile(POLY_OP_STATE_IMPORT :: "a"(state) : "r15", "memory");
}

static int install_poly_thread_state_key(void) {
  const char *use_explicit = getenv("POLYEXEC_USE_EXPLICIT_STATE_KEY");
  polyexec_use_explicit_state_key =
    use_explicit == NULL || strcmp(use_explicit, "0") != 0;
  if (!polyexec_use_explicit_state_key) {
    if (poly_state_key_set(0) != 0 || poly_state_key_get() != 0) {
      fprintf(stderr,
        "POLYEXEC_FAIL: Poly state-key clear failed got=0x%llx\n",
        (unsigned long long) poly_state_key_get());
      return -1;
    }
    puts("POLYEXEC_STATE_KEY: explicit=0");
    return 0;
  }

  const uint64_t key = (uint64_t) (uintptr_t) &poly_state_key_anchor;
  if (key == 0 || poly_state_key_set(key) != 0 ||
      poly_state_key_get() != key) {
    fprintf(stderr,
      "POLYEXEC_FAIL: explicit Poly state-key setup failed key=0x%llx got=0x%llx\n",
      (unsigned long long) key, (unsigned long long) poly_state_key_get());
    return -1;
  }
  printf("POLYEXEC_STATE_KEY: explicit=1 key=0x%llx\n",
    (unsigned long long) key);
  return 0;
}

static uint64_t poly_abi_signature_set(uint64_t slot, uint64_t kind) {
  uint64_t rax = slot;
  uint64_t rdx = poly_abi_signature_control_value(kind);
  asm volatile(POLY_OP_ABI_SIGNATURE_SET
    : "+a"(rax), "+d"(rdx)
    :
    : "memory");
  return rax;
}

static uint64_t poly_abi_signature_set_with_flags(uint64_t slot, uint64_t kind,
    uint32_t flags) {
  uint64_t rax = slot;
  uint64_t rdx = poly_abi_signature_control_value_with_flags(kind, flags);
  asm volatile(POLY_OP_ABI_SIGNATURE_SET
    : "+a"(rax), "+d"(rdx)
    :
    : "memory");
  return rax;
}

static uint64_t poly_event_ptr_set(uint64_t frame, uint64_t bytes) {
  asm volatile(POLY_OP_EVENT_PTR_SET
    : "+a"(frame), "+d"(bytes)
    :
    : "memory");
  return frame;
}

static uint64_t poly_spill_desc_set(uint64_t descriptor, uint64_t bytes) {
  asm volatile(POLY_OP_SPILL_DESC_SET
    : "+a"(descriptor), "+d"(bytes)
    :
    : "memory");
  return descriptor;
}

static struct poly_xsave_state *poly_auto_spill_active_state(void) {
  return poly_auto_spill_state_active != NULL ?
    poly_auto_spill_state_active : &poly_auto_spill_state;
}

static void populate_poly_v2_spill_descriptor(uint64_t buffer,
    uint64_t resume) {
  uint64_t generation = poly_auto_spill_descriptor.generation + 1;
  if (generation == 0)
    generation = 1;
  memset(&poly_auto_spill_descriptor, 0, sizeof(poly_auto_spill_descriptor));
  memset(&poly_auto_spill_event_frame, 0, sizeof(poly_auto_spill_event_frame));
  poly_auto_spill_descriptor.magic = POLY_V2_SPILL_DESC_MAGIC;
  poly_auto_spill_descriptor.bytes = POLY_V2_SPILL_DESC_BYTES;
  poly_auto_spill_descriptor.version = POLY_V2_SPILL_DESC_VERSION;
  poly_auto_spill_descriptor.header_bytes = POLY_V2_SPILL_DESC_HEADER_BYTES;
  poly_auto_spill_descriptor.flags = POLY_V2_SPILL_DESC_FLAG_ENABLE |
    POLY_V2_SPILL_DESC_FLAG_EVENT_FRAME |
    POLY_V2_SPILL_DESC_FLAG_RESUME_STACK |
    POLY_V2_SPILL_DESC_FLAG_STATE_IMAGE;
  poly_auto_spill_descriptor.owner_cookie =
    (uint64_t) (uintptr_t) &poly_auto_spill_descriptor;
  poly_auto_spill_descriptor.generation = generation;
  poly_auto_spill_descriptor.valid_mask =
    POLY_V2_SPILL_DESC_VALID_STATE_ADDR |
    POLY_V2_SPILL_DESC_VALID_EVENT_ADDR |
    POLY_V2_SPILL_DESC_VALID_RESUME_RIP |
    POLY_V2_SPILL_DESC_VALID_RESUME_STACK;
  poly_auto_spill_descriptor.state_addr = buffer;
  poly_auto_spill_descriptor.state_bytes = POLY_STATE_XSAVE_BYTES_ARCH;
  poly_auto_spill_descriptor.state_align = POLY_STATE_XSAVE_ALIGN_ARCH;
  poly_auto_spill_descriptor.state_layout_version =
    POLY_STATE_XSAVE_LAYOUT_VERSION;
  poly_auto_spill_descriptor.event_addr =
    (uint64_t) (uintptr_t) &poly_auto_spill_event_frame;
  poly_auto_spill_descriptor.event_bytes = POLY_V2_EVENT_BYTES;
  poly_auto_spill_descriptor.resume_rip = resume;
  poly_auto_spill_descriptor.resume_stack_base =
    (uint64_t) (uintptr_t) poly_auto_spill_resume_stack;
  poly_auto_spill_descriptor.resume_stack_bytes =
    POLY_AUTO_SPILL_RESUME_STACK_SIZE;
  poly_auto_spill_descriptor.resume_stack_top =
    (uint64_t) (uintptr_t) (poly_auto_spill_resume_stack +
      POLY_AUTO_SPILL_RESUME_STACK_SIZE);
  poly_auto_spill_descriptor.monitor_packet_addr =
    (uint64_t) (uintptr_t) poly_monitor_packet;
  poly_auto_spill_descriptor.monitor_packet_bytes =
    sizeof(poly_monitor_packet);
  poly_auto_spill_descriptor.frontend_mask =
    (1ULL << POLY_MODE_RAW_AARCH64) | (1ULL << POLY_MODE_RAW_RISCV);
}

static int refresh_poly_auto_spill(void) {
  if (!poly_auto_spill_installed)
    return 0;
  const uint64_t buffer =
    (uint64_t) (uintptr_t) poly_auto_spill_active_state();
  const uint64_t resume = (uint64_t) (uintptr_t)
    poly_auto_spill_resume_trampoline;
  populate_poly_v2_spill_descriptor(buffer, resume);
  if (poly_event_ptr_set((uint64_t) (uintptr_t) &poly_auto_spill_event_frame,
        POLY_V2_EVENT_BYTES) != 0)
    return -1;
  if (poly_spill_desc_set(
        (uint64_t) (uintptr_t) &poly_auto_spill_descriptor,
        POLY_V2_SPILL_DESC_BYTES) != 0)
    return -1;
  return 0;
}

static int prefault_poly_auto_spill(void) {
  if (!poly_auto_spill_installed)
    return 0;

  struct poly_xsave_state *state = poly_auto_spill_active_state();
  volatile uint8_t *bytes = (volatile uint8_t *) state;
  const size_t size = sizeof(*state);
  for (size_t offset = 0; offset < size; offset += 4096) {
    uint8_t value = bytes[offset];
    bytes[offset] = value;
  }
  uint8_t value = bytes[size - 1];
  bytes[size - 1] = value;
  return refresh_poly_auto_spill();
}

static uint64_t poly_auto_spill_count_status(void) {
  uint64_t rax;
  asm volatile(POLY_OP_AUTO_SPILL_COUNT_STATUS : "=a"(rax) ::
    "memory");
  return rax;
}

static uint64_t poly_auto_spill_bytes_status(void) {
  uint64_t rax;
  asm volatile(POLY_OP_AUTO_SPILL_BYTES_STATUS : "=a"(rax) ::
    "memory");
  return rax;
}

static uint64_t poly_auto_spill_cycles_status(void) {
  uint64_t rax;
  asm volatile(POLY_OP_AUTO_SPILL_CYCLES_STATUS : "=a"(rax) ::
    "memory");
  return rax;
}

static int polyexec_check_arch_state_contract(void) {
  struct poly_cpuid_contract_failure failure;
  if (!poly_cpuid_verify_arch_state_contract(&failure)) {
    fprintf(stderr,
      "POLYEXEC_FAIL: %s mismatch leaf=0x%x subleaf=%u got=(0x%x,0x%x,0x%x,0x%x) expected=(0x%x,0x%x,0x%x,0x%x)\n",
      failure.name, failure.leaf, failure.subleaf,
      failure.actual.eax, failure.actual.ebx,
      failure.actual.ecx, failure.actual.edx,
      failure.expected.eax, failure.expected.ebx,
      failure.expected.ecx, failure.expected.edx);
    return -1;
  }
  return 0;
}

static int read_poly_base_contract(int require_trap_vector) {
  const struct poly_cpuid_regs base = poly_read_cpuid(POLY_CPUID_BASE, 0);
  if (base.eax < POLY_CPUID_MAX || !poly_cpuid_vendor_matches(&base)) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly CPUID missing base=(0x%x,0x%x,0x%x,0x%x)\n",
      base.eax, base.ebx, base.ecx, base.edx);
    return -1;
  }

  const uint32_t required_modes =
    poly_cpuid_expected_mode_mask();
  uint32_t required_features = poly_cpuid_expected_feature_mask();
  if (!require_trap_vector)
    required_features &= ~POLY_CPUID_FEATURE_TRAP_VECTOR;
  const uint32_t forbidden_features = poly_cpuid_forbidden_feature_mask();

  const struct poly_cpuid_regs features =
    poly_read_cpuid(POLY_CPUID_BASE + 1, 0);
  if (features.eax != POLY_CPUID_ABI_VERSION ||
      features.ebx != required_modes ||
      (features.ecx & required_features) != required_features ||
      features.edx != POLY_STATE_XSAVE_COMPONENT_ARCH ||
      (features.ecx & forbidden_features) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly CPUID feature mismatch features=(%u,0x%x,0x%x,0x%x) expected_modes=0x%x expected_xsave=%u\n",
      features.eax, features.ebx, features.ecx, features.edx,
      required_modes, POLY_STATE_XSAVE_COMPONENT_ARCH);
    return -1;
  }

  if (polyexec_check_arch_state_contract() != 0)
    return -1;

  const struct poly_cpuid_regs abi_bridge =
    poly_read_cpuid(POLY_CPUID_BASE + 9, 0);
  const struct poly_cpuid_regs expected_abi_bridge =
    poly_cpuid_expected_abi_bridge_leaf();
  const uint32_t forbidden_abi_bridge =
    poly_cpuid_forbidden_abi_bridge_mask();
  if (abi_bridge.eax != expected_abi_bridge.eax ||
      abi_bridge.ebx != expected_abi_bridge.ebx ||
      abi_bridge.ecx != expected_abi_bridge.ecx ||
      abi_bridge.edx != expected_abi_bridge.edx ||
      (abi_bridge.ebx & forbidden_abi_bridge) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: CPU ABI bridge mismatch abi=(%u,0x%x,0x%x,0x%x) expected=(%u,0x%x,0x%x,0x%x)\n",
      abi_bridge.eax, abi_bridge.ebx, abi_bridge.ecx, abi_bridge.edx,
      expected_abi_bridge.eax, expected_abi_bridge.ebx,
      expected_abi_bridge.ecx, expected_abi_bridge.edx);
    return -1;
  }

  const struct poly_cpuid_regs x86_controls =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 5);
  const struct poly_cpuid_regs expected_x86_controls =
    poly_cpuid_expected_escape_leaf5();
  if (!poly_cpuid_regs_match(&x86_controls, &expected_x86_controls)) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly x86 control manifest mismatch x86=(0x%x,0x%x,0x%x,0x%x)\n",
      x86_controls.eax, x86_controls.ebx, x86_controls.ecx,
      x86_controls.edx);
    return -1;
  }
  const struct poly_cpuid_regs x86_geometry =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 32);
  const struct poly_cpuid_regs expected_x86_geometry =
    poly_cpuid_expected_escape_leaf32();
  if (!poly_cpuid_regs_match(&x86_geometry, &expected_x86_geometry)) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly x86 opcode geometry mismatch x86=(0x%x,0x%x,0x%x,0x%x)\n",
      x86_geometry.eax, x86_geometry.ebx, x86_geometry.ecx,
      x86_geometry.edx);
    return -1;
  }
  const struct poly_cpuid_regs auto_spill_status =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 34);
  const struct poly_cpuid_regs expected_auto_spill_status =
    poly_cpuid_expected_escape_leaf34();
  if (!poly_cpuid_regs_match(&auto_spill_status,
        &expected_auto_spill_status)) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly auto-spill status manifest mismatch x86=(0x%x,0x%x,0x%x,0x%x)\n",
      auto_spill_status.eax, auto_spill_status.ebx,
      auto_spill_status.ecx, auto_spill_status.edx);
    return -1;
  }

  const struct poly_cpuid_regs signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 7);
  const struct poly_cpuid_regs expected_signature =
    poly_cpuid_expected_escape_leaf7();
  const uint32_t native_slot = (signature.ecx >> 24) & 0xffU;
  const uint32_t native_kind = (signature.edx >> 24) & 0xffU;
  const struct poly_cpuid_regs signature_ext =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 17);
  const struct poly_cpuid_regs expected_signature_ext =
    poly_cpuid_expected_escape_leaf17();
  const struct poly_cpuid_regs signature_compact =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 20);
  const struct poly_cpuid_regs expected_signature_compact =
    poly_cpuid_expected_escape_leaf20();
  const struct poly_cpuid_regs signature_fp64 =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 22);
  const struct poly_cpuid_regs expected_signature_fp64 =
    poly_cpuid_expected_escape_leaf22();
  const struct poly_cpuid_regs signature_fp32 =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 23);
  const struct poly_cpuid_regs expected_signature_fp32 =
    poly_cpuid_expected_escape_leaf23();
  const struct poly_cpuid_regs signature_sret =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 24);
  const struct poly_cpuid_regs expected_signature_sret =
    poly_cpuid_expected_escape_leaf24();
  const struct poly_cpuid_regs signature_hfa32 =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 26);
  const struct poly_cpuid_regs expected_signature_hfa32 =
    poly_cpuid_expected_escape_leaf26();
  const struct poly_cpuid_regs signature_hfa32_arg =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 27);
  const struct poly_cpuid_regs expected_signature_hfa32_arg =
    poly_cpuid_expected_escape_leaf27();
  const struct poly_cpuid_regs signature_native_sret =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 28);
  const struct poly_cpuid_regs expected_signature_native_sret =
    poly_cpuid_expected_escape_leaf28();
  const struct poly_cpuid_regs signature_hfa64_ret =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 29);
  const struct poly_cpuid_regs expected_signature_hfa64_ret =
    poly_cpuid_expected_escape_leaf29();
  const uint32_t vec128_slot = signature_ext.ecx;
  const uint32_t vec128_kind = signature_ext.edx;
  const uint32_t compact_u32_f32_slot = signature_compact.eax;
  const uint32_t compact_u32_f32_kind = signature_compact.ebx;
  const uint32_t compact_f32_u32_slot = signature_compact.ecx;
  const uint32_t compact_f32_u32_kind = signature_compact.edx;
  const uint32_t fp64_slot = signature_fp64.edx;
  const uint32_t fp32_slot = signature_fp32.edx;
  const uint32_t sret_slot = signature_sret.eax;
  const uint32_t native_sret_slot = signature_native_sret.eax;
  if (signature.eax != expected_signature.eax ||
      signature.ebx != expected_signature.ebx ||
      signature.ecx != expected_signature.ecx ||
      signature.edx != expected_signature.edx ||
      signature_ext.eax != expected_signature_ext.eax ||
      signature_ext.ebx != expected_signature_ext.ebx ||
      signature_ext.ecx != expected_signature_ext.ecx ||
      signature_ext.edx != expected_signature_ext.edx ||
      signature_compact.eax != expected_signature_compact.eax ||
      signature_compact.ebx != expected_signature_compact.ebx ||
      signature_compact.ecx != expected_signature_compact.ecx ||
      signature_compact.edx != expected_signature_compact.edx ||
      signature_fp64.eax != expected_signature_fp64.eax ||
      signature_fp64.ebx != expected_signature_fp64.ebx ||
      signature_fp64.ecx != expected_signature_fp64.ecx ||
      signature_fp64.edx != expected_signature_fp64.edx ||
      signature_fp32.eax != expected_signature_fp32.eax ||
      signature_fp32.ebx != expected_signature_fp32.ebx ||
      signature_fp32.ecx != expected_signature_fp32.ecx ||
      signature_fp32.edx != expected_signature_fp32.edx ||
      signature_sret.eax != expected_signature_sret.eax ||
      signature_sret.ebx != expected_signature_sret.ebx ||
      signature_sret.ecx != expected_signature_sret.ecx ||
      signature_sret.edx != expected_signature_sret.edx ||
      signature_hfa32.eax != expected_signature_hfa32.eax ||
      signature_hfa32.ebx != expected_signature_hfa32.ebx ||
      signature_hfa32.ecx != expected_signature_hfa32.ecx ||
      signature_hfa32.edx != expected_signature_hfa32.edx ||
      signature_hfa32_arg.eax != expected_signature_hfa32_arg.eax ||
      signature_hfa32_arg.ebx != expected_signature_hfa32_arg.ebx ||
      signature_hfa32_arg.ecx != expected_signature_hfa32_arg.ecx ||
      signature_hfa32_arg.edx != expected_signature_hfa32_arg.edx ||
      signature_native_sret.eax != expected_signature_native_sret.eax ||
      signature_native_sret.ebx != expected_signature_native_sret.ebx ||
      signature_native_sret.ecx != expected_signature_native_sret.ecx ||
      signature_native_sret.edx != expected_signature_native_sret.edx ||
      signature_hfa64_ret.eax != expected_signature_hfa64_ret.eax ||
      signature_hfa64_ret.ebx != expected_signature_hfa64_ret.ebx ||
      signature_hfa64_ret.ecx != expected_signature_hfa64_ret.ecx ||
      signature_hfa64_ret.edx != expected_signature_hfa64_ret.edx ||
      native_slot >= signature.ebx ||
      native_kind != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS ||
      vec128_slot >= signature.ebx ||
      vec128_kind != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32 ||
      compact_u32_f32_slot >= signature.ebx ||
      compact_u32_f32_kind !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_U32_F32 ||
      compact_f32_u32_slot >= signature.ebx ||
      compact_f32_u32_kind !=
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_F32_U32 ||
      fp64_slot >= signature.ebx ||
      fp32_slot >= signature.ebx ||
      sret_slot >= signature.ebx ||
      native_sret_slot >= signature.ebx) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native signature manifest mismatch sig=(0x%x,%u,0x%x,0x%x) ext=(%u,%u,0x%x,0x%x) compact=(%u,%u,%u,%u) fp64=(%u,%u,%u,%u) fp32=(%u,%u,%u,%u) sret=(%u,%u,%u,%u) hfa32=(%u,%u,%u,%u) hfa32arg=(%u,%u,%u,%u) nsret=(%u,%u,%u,%u) hfa64ret=(%u,%u,%u,%u)\n",
      signature.eax, signature.ebx, signature.ecx, signature.edx,
      signature_ext.eax, signature_ext.ebx, signature_ext.ecx,
      signature_ext.edx, signature_compact.eax, signature_compact.ebx,
      signature_compact.ecx, signature_compact.edx, signature_fp64.eax,
      signature_fp64.ebx, signature_fp64.ecx, signature_fp64.edx,
      signature_fp32.eax, signature_fp32.ebx, signature_fp32.ecx,
      signature_fp32.edx, signature_sret.eax, signature_sret.ebx,
      signature_sret.ecx, signature_sret.edx, signature_hfa32.eax,
      signature_hfa32.ebx, signature_hfa32.ecx, signature_hfa32.edx,
      signature_hfa32_arg.eax, signature_hfa32_arg.ebx,
      signature_hfa32_arg.ecx, signature_hfa32_arg.edx,
      signature_native_sret.eax, signature_native_sret.ebx,
      signature_native_sret.ecx, signature_native_sret.edx,
      signature_hfa64_ret.eax, signature_hfa64_ret.ebx,
      signature_hfa64_ret.ecx, signature_hfa64_ret.edx);
    return -1;
  }
  if (poly_abi_signature_set(native_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native signature slot setup failed slot=%u\n",
      native_slot);
    return -1;
  }
  process_native_signature_slot = native_slot;
  process_fp64_signature_slot = fp64_slot;
  process_fp32_signature_slot = fp32_slot;
  process_sret_signature_slot = sret_slot;
  process_native_sret_signature_slot = native_sret_slot;
  if (poly_abi_signature_set(process_fp64_signature_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native fp64 signature slot setup failed slot=%u\n",
      process_fp64_signature_slot);
    return -1;
  }
  if (poly_abi_signature_set(process_fp32_signature_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP32) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native fp32 signature slot setup failed slot=%u\n",
      process_fp32_signature_slot);
    return -1;
  }
  if (poly_abi_signature_set(process_sret_signature_slot,
        POLY_ABI_SIGNATURE_KIND_SRET_X86_SYSV_REGS) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly sret signature slot setup failed slot=%u\n",
      process_sret_signature_slot);
    return -1;
  }
  if (poly_abi_signature_set(process_native_sret_signature_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native sret signature slot setup failed slot=%u\n",
      process_native_sret_signature_slot);
    return -1;
  }
  if (poly_abi_signature_set(vec128_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native vec128 signature slot setup failed slot=%u\n",
      vec128_slot);
    return -1;
  }
  process_vec128_signature_slot = vec128_slot;
  if (poly_abi_signature_set(compact_u32_f32_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_U32_F32) != 0 ||
      poly_abi_signature_set(compact_f32_u32_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_F32_U32) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native compact signature slot setup failed u32_f32=%u f32_u32=%u\n",
      compact_u32_f32_slot, compact_f32_u32_slot);
    return -1;
  }
  process_compact_u32_f32_signature_slot = compact_u32_f32_slot;
  process_compact_f32_u32_signature_slot = compact_f32_u32_slot;

  return 0;
}

static inline void poly_trap_vector_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline void poly_trap_vector_mode_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
}

static inline void poly_monitor_packet_set_value(uint64_t value) {
  asm volatile(POLY_OP_MONITOR_PACKET_SET :: "a"(value) : "memory");
}

static inline uint64_t poly_monitor_packet_get_value(void) {
  uint64_t value = 0;
  asm volatile(POLY_OP_MONITOR_PACKET_GET : "=a"(value) :: "memory");
  return value;
}

static int poly_is_raw_foreign_mode(uint64_t mode) {
  return mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV;
}

static void poly_write_all_stderr(const char *text, size_t len) {
  while (len != 0) {
    ssize_t written = write(STDERR_FILENO, text, len);
    if (written <= 0)
      return;
    text += (size_t) written;
    len -= (size_t) written;
  }
}

static void poly_write_literal_stderr(const char *text) {
  size_t len = 0;
  while (text[len] != '\0')
    len++;
  poly_write_all_stderr(text, len);
}

static void poly_write_hex64_stderr(uint64_t value) {
  static const char hex[] = "0123456789abcdef";
  char buf[16];
  for (unsigned n = 0; n < sizeof(buf); n++) {
    const unsigned shift = (unsigned) ((sizeof(buf) - 1 - n) * 4);
    buf[n] = hex[(value >> shift) & 0xfU];
  }
  poly_write_all_stderr(buf, sizeof(buf));
}

static void poly_write_u32_stderr(uint32_t value) {
  char buf[10];
  size_t pos = sizeof(buf);
  do {
    buf[--pos] = (char) ('0' + (value % 10U));
    value /= 10U;
  } while (value != 0);
  poly_write_all_stderr(buf + pos, sizeof(buf) - pos);
}

static void poly_dump_aarch64_gprs_stderr(const char *prefix,
    const uint64_t gpr[32], uint64_t tls_base) {
  poly_write_literal_stderr(prefix);
  for (uint32_t n = 0; n < 31; n++) {
    if (n != 0 && (n % 4U) == 0U) {
      poly_write_literal_stderr("\n");
      poly_write_literal_stderr(prefix);
    } else if (n != 0) {
      poly_write_literal_stderr(" ");
    }
    poly_write_literal_stderr("x");
    poly_write_u32_stderr(n);
    poly_write_literal_stderr("=0x");
    poly_write_hex64_stderr(gpr[n]);
  }
  poly_write_literal_stderr("\n");
  poly_write_literal_stderr(prefix);
  poly_write_literal_stderr("sp=0x");
  poly_write_hex64_stderr(gpr[31]);
  poly_write_literal_stderr(" tls=0x");
  poly_write_hex64_stderr(tls_base);
  poly_write_literal_stderr("\n");
}

static void poly_dump_aarch64_stack_words_stderr(const char *prefix,
    uint64_t sp) {
  if (!poly_guest_range_is_mapped(sp, 64U * sizeof(uint64_t), 0))
    return;
  const volatile uint64_t *words =
    (const volatile uint64_t *) (uintptr_t) sp;
  for (uint32_t n = 0; n < 64; n++) {
    if ((n % 4U) == 0U) {
      if (n != 0)
        poly_write_literal_stderr("\n");
      poly_write_literal_stderr(prefix);
      poly_write_literal_stderr("sp+0x");
      poly_write_hex64_stderr((uint64_t) n * 8U);
      poly_write_literal_stderr(": ");
    } else {
      poly_write_literal_stderr(" ");
    }
    poly_write_hex64_stderr(words[n]);
  }
  poly_write_literal_stderr("\n");
}

static void poly_dump_aarch64_memory_words_stderr(const char *prefix,
    const char *label, uint64_t address) {
  if (address < 4096)
    return;
  if (!poly_guest_range_is_mapped(address, 8U * sizeof(uint64_t), 0))
    return;
  const volatile uint64_t *words =
    (const volatile uint64_t *) (uintptr_t) address;
  poly_write_literal_stderr(prefix);
  poly_write_literal_stderr(label);
  poly_write_literal_stderr("@0x");
  poly_write_hex64_stderr(address);
  poly_write_literal_stderr(":");
  for (uint32_t n = 0; n < 8; n++) {
    poly_write_literal_stderr(" ");
    poly_write_hex64_stderr(words[n]);
  }
  poly_write_literal_stderr("\n");
}

static void poly_dump_aarch64_fault_operands_stderr(const char *prefix,
    const uint64_t gpr[32]) {
  poly_dump_aarch64_memory_words_stderr(prefix, "x0", gpr[0]);
  poly_dump_aarch64_memory_words_stderr(prefix, "x12", gpr[12]);
  poly_dump_aarch64_memory_words_stderr(prefix, "x13", gpr[13]);
  poly_dump_aarch64_memory_words_stderr(prefix, "x14", gpr[14]);
  poly_dump_aarch64_memory_words_stderr(prefix, "x20", gpr[20]);
  poly_dump_aarch64_memory_words_stderr(prefix, "x21", gpr[21]);
  poly_dump_aarch64_memory_words_stderr(prefix, "x22", gpr[22]);
}

static int poly_dump_maps_on_fault_enabled(void) {
  if (polyexec_dump_maps_on_fault < 0) {
    const char *value = getenv("POLYEXEC_DUMP_MAPS_ON_FAULT");
    polyexec_dump_maps_on_fault = value != NULL && value[0] != '\0' &&
      strcmp(value, "0") != 0;
  }
  return polyexec_dump_maps_on_fault;
}

static int poly_dump_aarch64_node_chain_on_fault_enabled(void) {
  if (polyexec_dump_aarch64_node_chain_on_fault < 0) {
    const char *value = getenv("POLYEXEC_DUMP_AARCH64_NODE_CHAIN_ON_FAULT");
    polyexec_dump_aarch64_node_chain_on_fault =
      value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
  }
  return polyexec_dump_aarch64_node_chain_on_fault;
}

static void poly_dump_aarch64_node_chain_stderr(const char *prefix,
    const uint64_t gpr[32]) {
  if (!poly_dump_aarch64_node_chain_on_fault_enabled())
    return;
  uint64_t parent = gpr[21];
  if (parent < 4096)
    return;
  if (!poly_guest_range_is_mapped(parent, 9U * sizeof(uint64_t), 0))
    return;
  const volatile uint64_t *parent_words =
    (const volatile uint64_t *) (uintptr_t) parent;
  uint64_t node = parent_words[8];
  poly_write_literal_stderr(prefix);
  poly_write_literal_stderr("parent=0x");
  poly_write_hex64_stderr(parent);
  poly_write_literal_stderr(" head=0x");
  poly_write_hex64_stderr(node);
  poly_write_literal_stderr("\n");
  for (uint32_t n = 0; n < 16 && node >= 4096; n++) {
    if (!poly_guest_range_is_mapped(node, 3U * sizeof(uint64_t), 0))
      break;
    const volatile uint64_t *words =
      (const volatile uint64_t *) (uintptr_t) node;
    uint64_t header = words[0];
    uint64_t payload = words[1];
    uint64_t next = words[2];
    poly_write_literal_stderr(prefix);
    poly_write_literal_stderr("node[");
    poly_write_u32_stderr(n);
    poly_write_literal_stderr("]=0x");
    poly_write_hex64_stderr(node);
    poly_write_literal_stderr(" w0=0x");
    poly_write_hex64_stderr(header);
    poly_write_literal_stderr(" w1=0x");
    poly_write_hex64_stderr(payload);
    poly_write_literal_stderr(" next=0x");
    poly_write_hex64_stderr(next);
    poly_write_literal_stderr("\n");
    node = next;
  }
}

static void poly_dump_proc_maps_stderr(void) {
  int fd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return;
  poly_write_literal_stderr("POLYEXEC_MAPS_BEGIN\n");
  char buf[1024];
  for (;;) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n <= 0)
      break;
    poly_write_all_stderr(buf, (size_t) n);
  }
  poly_write_literal_stderr("POLYEXEC_MAPS_END\n");
  close(fd);
}

static void report_poly_spill_page_fault(void) {
  struct poly_xsave_state *spill_state = poly_auto_spill_active_state();
  const uint64_t cr2 = spill_state->trap_args[3];
  poly_write_literal_stderr("Poly Page Fault at Address 0x");
  poly_write_hex64_stderr(cr2);
  poly_write_literal_stderr(" PC 0x");
  poly_write_hex64_stderr(spill_state->header.foreign_pc);
  poly_write_literal_stderr("\n");
  if (spill_state->header.current_mode == POLY_MODE_RAW_AARCH64) {
    poly_dump_aarch64_gprs_stderr("POLYEXEC_SPILL_AARCH64_REGS: ",
      spill_state->aarch64_gpr, spill_state->frontend_tls.aarch64_tls_base);
    poly_dump_aarch64_fault_operands_stderr(
      "POLYEXEC_SPILL_AARCH64_MEM: ", spill_state->aarch64_gpr);
    poly_dump_aarch64_node_chain_stderr(
      "POLYEXEC_SPILL_AARCH64_NODE_CHAIN: ", spill_state->aarch64_gpr);
    poly_dump_aarch64_stack_words_stderr("POLYEXEC_SPILL_AARCH64_STACK: ",
      spill_state->aarch64_gpr[31]);
  }
  if (poly_dump_maps_on_fault_enabled())
    poly_dump_proc_maps_stderr();
}

static void poly_dump_native_signal_context_stderr(void *ucontext) {
#if defined(__x86_64__) && defined(REG_RIP)
  if (ucontext == NULL)
    return;
  const ucontext_t *ctx = (const ucontext_t *) ucontext;
  const greg_t *regs = ctx->uc_mcontext.gregs;
  poly_write_literal_stderr("POLYEXEC_NATIVE_CONTEXT: rip=0x");
  poly_write_hex64_stderr((uint64_t) regs[REG_RIP]);
  poly_write_literal_stderr(" rsp=0x");
  poly_write_hex64_stderr((uint64_t) regs[REG_RSP]);
  poly_write_literal_stderr(" rbp=0x");
  poly_write_hex64_stderr((uint64_t) regs[REG_RBP]);
  poly_write_literal_stderr(" rax=0x");
  poly_write_hex64_stderr((uint64_t) regs[REG_RAX]);
  poly_write_literal_stderr(" rdi=0x");
  poly_write_hex64_stderr((uint64_t) regs[REG_RDI]);
  poly_write_literal_stderr(" rsi=0x");
  poly_write_hex64_stderr((uint64_t) regs[REG_RSI]);
  poly_write_literal_stderr(" rdx=0x");
  poly_write_hex64_stderr((uint64_t) regs[REG_RDX]);
  poly_write_literal_stderr("\n");
#else
  (void) ucontext;
#endif
}

static void poly_dump_exported_signal_state_stderr(void) {
  struct poly_xsave_state state __attribute__((aligned(POLY_STATE_XSAVE_ALIGN_ARCH)));
  memset(&state, 0, sizeof(state));
  asm volatile(POLY_OP_STATE_EXPORT :: "a"(&state) : "r15", "memory");
  if (state.header.magic != POLY_STATE_XSAVE_MAGIC)
    return;

  poly_write_literal_stderr("POLYEXEC_EXPORTED_STATE: mode=0x");
  poly_write_hex64_stderr(state.header.current_mode);
  poly_write_literal_stderr(" foreign_pc=0x");
  poly_write_hex64_stderr(state.header.foreign_pc);
  poly_write_literal_stderr(" foreign_tls=0x");
  poly_write_hex64_stderr(state.header.foreign_tls_base);
  poly_write_literal_stderr(" spill_reason=0x");
  poly_write_hex64_stderr(state.header.spill_reason);
  poly_write_literal_stderr("\n");
  if (state.header.current_mode == POLY_MODE_RAW_AARCH64) {
    poly_dump_aarch64_gprs_stderr("POLYEXEC_AARCH64_REGS: ",
      state.aarch64_gpr, state.frontend_tls.aarch64_tls_base);
    poly_dump_aarch64_node_chain_stderr("POLYEXEC_AARCH64_NODE_CHAIN: ",
      state.aarch64_gpr);
  } else if (state.header.current_mode == POLY_MODE_RAW_RISCV) {
    poly_write_literal_stderr("POLYEXEC_RISCV_REGS: x10=0x");
    poly_write_hex64_stderr(state.riscv_gpr[10]);
    poly_write_literal_stderr(" x11=0x");
    poly_write_hex64_stderr(state.riscv_gpr[11]);
    poly_write_literal_stderr(" x12=0x");
    poly_write_hex64_stderr(state.riscv_gpr[12]);
    poly_write_literal_stderr(" x13=0x");
    poly_write_hex64_stderr(state.riscv_gpr[13]);
    poly_write_literal_stderr(" sp=0x");
    poly_write_hex64_stderr(state.riscv_gpr[2]);
    poly_write_literal_stderr(" tls=0x");
    poly_write_hex64_stderr(state.frontend_tls.riscv_tls_base);
    poly_write_literal_stderr("\n");
  }
}

static int poly_redirect_signal_to_auto_spill_resume(void *ucontext) {
#if defined(__x86_64__) && defined(REG_RIP)
  if (ucontext == NULL)
    return 0;
  ucontext_t *ctx = (ucontext_t *) ucontext;
  ctx->uc_mcontext.gregs[REG_RIP] =
    (greg_t) (uintptr_t) poly_auto_spill_resume_trampoline;
  ctx->uc_mcontext.gregs[REG_RSP] =
    (greg_t) (uintptr_t) (poly_auto_spill_resume_stack +
      POLY_AUTO_SPILL_RESUME_STACK_SIZE);
  return 1;
#else
  (void) ucontext;
  return 0;
#endif
}

static void poly_auto_spill_signal(int signo, siginfo_t *info,
    void *ucontext) {
  poly_auto_spill_signal_signo = signo;
  poly_auto_spill_signal_code = info != NULL ? info->si_code : 0;
  poly_auto_spill_signal_addr =
    info != NULL ? (uintptr_t) info->si_addr : (uintptr_t) 0;
  poly_write_literal_stderr("POLYEXEC_SIGNAL: signo=0x");
  poly_write_hex64_stderr((uint64_t) signo);
  poly_write_literal_stderr(" code=0x");
  poly_write_hex64_stderr(info != NULL ? (uint64_t) info->si_code : 0);
  poly_write_literal_stderr(" addr=0x");
  poly_write_hex64_stderr(info != NULL ? (uint64_t) (uintptr_t) info->si_addr : 0);
  poly_write_literal_stderr("\n");
  poly_dump_native_signal_context_stderr(ucontext);
  poly_dump_exported_signal_state_stderr();
  struct poly_xsave_state *spill_state = poly_auto_spill_active_state();
  if (spill_state->header.magic == POLY_STATE_XSAVE_MAGIC &&
      spill_state->header.current_mode == POLY_MODE_RAW_AARCH64 &&
      (spill_state->header.spill_reason == POLY_SPILL_REASON_INTERRUPT ||
       spill_state->header.spill_reason == POLY_SPILL_REASON_PAGE_FAULT ||
       spill_state->header.spill_reason == POLY_SPILL_REASON_FAULT)) {
    poly_auto_spill_installed = 1;
    if (poly_redirect_signal_to_auto_spill_resume(ucontext)) {
      if (poly_trace_protected_signal_waits_enabled()) {
        poly_write_literal_stderr(
          "POLYEXEC_AUTO_SPILL_SIGNAL_REDIRECT: signum=0x");
        poly_write_hex64_stderr((uint64_t) signo);
        poly_write_literal_stderr(" mode=0x");
        poly_write_hex64_stderr(spill_state->header.current_mode);
        poly_write_literal_stderr(" reason=0x");
        poly_write_hex64_stderr(spill_state->header.spill_reason);
        poly_write_literal_stderr(" foreign_pc=0x");
        poly_write_hex64_stderr(spill_state->header.foreign_pc);
        poly_write_literal_stderr(" fault=0x");
        poly_write_hex64_stderr(spill_state->trap_args[3]);
        poly_write_literal_stderr("\n");
      }
      return;
    }
    report_poly_spill_page_fault();
    _exit(128 + signo);
  }
  if (spill_state->header.magic == POLY_STATE_XSAVE_MAGIC) {
    poly_write_literal_stderr(" spill_mode=0x");
    poly_write_hex64_stderr(spill_state->header.current_mode);
    poly_write_literal_stderr(" spill_reason=0x");
    poly_write_hex64_stderr(spill_state->header.spill_reason);
    poly_write_literal_stderr(" foreign_pc=0x");
    poly_write_hex64_stderr(spill_state->header.foreign_pc);
    poly_write_literal_stderr(" trap_pc=0x");
    poly_write_hex64_stderr(spill_state->trap.trap_pc);
    poly_write_literal_stderr(" trap_resume=0x");
    poly_write_hex64_stderr(spill_state->trap.resume_pc);
    poly_write_literal_stderr(" trap_arg0=0x");
    poly_write_hex64_stderr(spill_state->trap_args[0]);
    poly_write_literal_stderr(" trap_arg1=0x");
    poly_write_hex64_stderr(spill_state->trap_args[1]);
    poly_write_literal_stderr(" trap_arg2=0x");
    poly_write_hex64_stderr(spill_state->trap_args[2]);
    poly_write_literal_stderr(" trap_arg3=0x");
    poly_write_hex64_stderr(spill_state->trap_args[3]);
    if (spill_state->header.current_mode == POLY_MODE_RAW_AARCH64) {
      poly_write_literal_stderr("\n");
      poly_dump_aarch64_gprs_stderr("POLYEXEC_SPILL_AARCH64_REGS: ",
        spill_state->aarch64_gpr, spill_state->frontend_tls.aarch64_tls_base);
      poly_dump_aarch64_stack_words_stderr("POLYEXEC_SPILL_AARCH64_STACK: ",
        spill_state->aarch64_gpr[31]);
    }
  }
  poly_write_literal_stderr("POLYEXEC_SIGNAL_STATE_END\n");
  if (poly_dump_maps_on_fault_enabled())
    poly_dump_proc_maps_stderr();
  _exit(128 + signo);
}

static void poly_watchdog_signal(int signo, siginfo_t *info,
    void *ucontext) {
  (void) signo;
  (void) info;
  (void) ucontext;
  poly_write_literal_stderr("POLYEXEC_WATCHDOG: seconds=0x");
  poly_write_hex64_stderr((uint64_t) polyexec_watchdog_seconds);
  poly_write_literal_stderr("\n");
  poly_dump_exported_signal_state_stderr();
  poly_write_literal_stderr("POLYEXEC_WATCHDOG_EXIT\n");
  _exit(124);
}

static int install_poly_watchdog(void) {
  if (polyexec_watchdog_seconds == 0)
    return 0;

  polyexec_watchdog_signal = SIGRTMIN;
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = poly_watchdog_signal;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO;
  if (sigaction(polyexec_watchdog_signal, &action, NULL) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: watchdog signal install failed signum=%d: %s\n",
      polyexec_watchdog_signal, strerror(errno));
    return -1;
  }

  sigset_t watchdog_set;
  sigemptyset(&watchdog_set);
  sigaddset(&watchdog_set, polyexec_watchdog_signal);
  if (sigprocmask(SIG_UNBLOCK, &watchdog_set, NULL) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: watchdog signal unblock failed signum=%d: %s\n",
      polyexec_watchdog_signal, strerror(errno));
    return -1;
  }

  struct sigevent event;
  memset(&event, 0, sizeof(event));
  event.sigev_notify = SIGEV_SIGNAL;
  event.sigev_signo = polyexec_watchdog_signal;
  if (timer_create(CLOCK_MONOTONIC, &event, &polyexec_watchdog_timer) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: watchdog timer_create failed: %s\n",
      strerror(errno));
    return -1;
  }

  struct itimerspec spec;
  memset(&spec, 0, sizeof(spec));
  spec.it_value.tv_sec = (time_t) polyexec_watchdog_seconds;
  if (timer_settime(polyexec_watchdog_timer, 0, &spec, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: watchdog timer_settime failed: %s\n",
      strerror(errno));
    return -1;
  }

  printf("POLYEXEC_WATCHDOG_ARMED: seconds=%u signum=%d\n",
    polyexec_watchdog_seconds, polyexec_watchdog_signal);
  return 0;
}

static int poly_install_runtime_signal_alt_stack(void) {
  if (poly_runtime_signal_alt_stack_installed)
    return 0;
  stack_t stack;
  memset(&stack, 0, sizeof(stack));
  stack.ss_sp = poly_runtime_signal_alt_stack;
  stack.ss_size = sizeof(poly_runtime_signal_alt_stack);
  if (sigaltstack(&stack, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: runtime signal altstack install failed: %s\n",
      strerror(errno));
    return -1;
  }
  poly_runtime_signal_alt_stack_installed = 1;
  return 0;
}

static int install_poly_signal_diagnostics(void) {
  if (poly_install_runtime_signal_alt_stack() < 0)
    return -1;
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = poly_auto_spill_signal;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO | SA_ONSTACK;
  if (sigaction(SIGSEGV, &action, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: SIGSEGV diagnostic handler install failed: %s\n",
      strerror(errno));
    return -1;
  }
  if (sigaction(SIGBUS, &action, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: SIGBUS diagnostic handler install failed: %s\n",
      strerror(errno));
    return -1;
  }
  if (sigaction(SIGILL, &action, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: SIGILL diagnostic handler install failed: %s\n",
      strerror(errno));
    return -1;
  }
  sigset_t runtime_signals;
  sigemptyset(&runtime_signals);
  sigaddset(&runtime_signals, SIGSEGV);
  sigaddset(&runtime_signals, SIGBUS);
  sigaddset(&runtime_signals, SIGILL);
  if (sigprocmask(SIG_UNBLOCK, &runtime_signals, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: diagnostic signal unblock failed: %s\n",
      strerror(errno));
    return -1;
  }
  return 0;
}

static int poly_install_auto_spill_signal_actions(void) {
  if (poly_install_runtime_signal_alt_stack() < 0)
    return -1;
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = poly_auto_spill_signal;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO | SA_ONSTACK;
  if (sigaction(SIGSEGV, &action, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: SIGSEGV handler install failed: %s\n",
      strerror(errno));
    return -1;
  }
  if (sigaction(SIGBUS, &action, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: SIGBUS handler install failed: %s\n",
      strerror(errno));
    return -1;
  }
  if (sigaction(SIGILL, &action, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: SIGILL handler install failed: %s\n",
      strerror(errno));
    return -1;
  }
  sigset_t runtime_signals;
  sigemptyset(&runtime_signals);
  sigaddset(&runtime_signals, SIGSEGV);
  sigaddset(&runtime_signals, SIGBUS);
  sigaddset(&runtime_signals, SIGILL);
  if (sigprocmask(SIG_UNBLOCK, &runtime_signals, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: runtime signal unblock failed: %s\n",
      strerror(errno));
    return -1;
  }
  return 0;
}

__attribute__((noinline, used))
static void poly_auto_spill_resume_dispatch(
    struct poly_auto_spill_resume_info *resume_info) {
  struct poly_xsave_state *spill_state = poly_auto_spill_active_state();
  if (resume_info == NULL || !poly_auto_spill_installed ||
      spill_state->header.magic != POLY_STATE_XSAVE_MAGIC) {
    poly_write_literal_stderr(
      "POLYEXEC_FAIL: auto-spill resume without valid spill image\n");
    _exit(125);
  }

  const uint32_t reason = spill_state->header.spill_reason;
  if (reason == POLY_SPILL_REASON_INTERRUPT) {
    memset(&spill_state->trap, 0, sizeof(spill_state->trap));
    memset(spill_state->trap_args, 0, sizeof(spill_state->trap_args));
    memset(&spill_state->trap_restore, 0, sizeof(spill_state->trap_restore));
    if (refresh_poly_auto_spill() < 0) {
      poly_write_literal_stderr(
        "POLYEXEC_FAIL: auto-spill refresh failed during resume\n");
      _exit(125);
    }
    resume_info->buffer = (uint64_t) (uintptr_t) spill_state;
    resume_info->mode = spill_state->header.current_mode;
    return;
  }

  if (reason == POLY_SPILL_REASON_PAGE_FAULT) {
    const uint64_t fault_address = spill_state->trap_args[3];
    const int fault_is_write = (spill_state->trap_args[2] & 0x2ULL) != 0;
    if (spill_state->header.current_mode == POLY_MODE_RAW_AARCH64 &&
        fault_address != 0 &&
        poly_guest_range_is_mapped(fault_address, 1, fault_is_write)) {
      poly_prefault_range(fault_address, 1, fault_is_write);
      spill_state->header.spill_reason = 0;
      memset(&spill_state->trap, 0, sizeof(spill_state->trap));
      memset(spill_state->trap_args, 0, sizeof(spill_state->trap_args));
      memset(&spill_state->trap_restore, 0, sizeof(spill_state->trap_restore));
      if (refresh_poly_auto_spill() < 0) {
        poly_write_literal_stderr(
          "POLYEXEC_FAIL: auto-spill refresh failed during page-fault retry\n");
        _exit(125);
      }
      if (poly_trace_protected_signal_waits_enabled()) {
        poly_write_literal_stderr("POLYEXEC_PAGE_FAULT_RETRY: addr=0x");
        poly_write_hex64_stderr(fault_address);
        poly_write_literal_stderr(" write=0x");
        poly_write_hex64_stderr((uint64_t) fault_is_write);
        poly_write_literal_stderr(" pc=0x");
        poly_write_hex64_stderr(spill_state->header.foreign_pc);
        poly_write_literal_stderr("\n");
      }
      resume_info->buffer = (uint64_t) (uintptr_t) spill_state;
      resume_info->mode = spill_state->header.current_mode;
      return;
    }
    if (spill_state->header.current_mode == POLY_MODE_RAW_AARCH64 &&
        poly_try_deliver_aarch64_signal(spill_state, SIGSEGV,
          fault_address,
          (spill_state->trap_args[2] & 0x1ULL) != 0 ?
            SEGV_ACCERR : SEGV_MAPERR)) {
      spill_state->header.spill_reason = 0;
      if (refresh_poly_auto_spill() < 0) {
        poly_write_literal_stderr(
          "POLYEXEC_FAIL: auto-spill refresh failed during signal delivery\n");
        _exit(125);
      }
      resume_info->buffer = (uint64_t) (uintptr_t) spill_state;
      resume_info->mode = spill_state->header.current_mode;
      return;
    }
    report_poly_spill_page_fault();
    _exit(139);
  }

  if (reason == POLY_SPILL_REASON_FAULT) {
    uint64_t forced_signum = SIGSEGV;
    uint64_t fault_address = 0;
    uint64_t fault_code = SI_KERNEL;
    if (poly_auto_spill_signal_signo == SIGILL) {
      forced_signum = SIGILL;
      fault_address = (uint64_t) poly_auto_spill_signal_addr;
      fault_code = poly_auto_spill_signal_code != 0 ?
        (uint64_t) poly_auto_spill_signal_code : (uint64_t) ILL_ILLOPC;
    } else if (poly_auto_spill_signal_signo == SIGBUS) {
      forced_signum = SIGBUS;
      fault_address = (uint64_t) poly_auto_spill_signal_addr;
      fault_code = poly_auto_spill_signal_code != 0 ?
        (uint64_t) poly_auto_spill_signal_code : (uint64_t) BUS_ADRERR;
    }
    if (spill_state->header.current_mode == POLY_MODE_RAW_AARCH64 &&
        poly_try_deliver_aarch64_signal(spill_state, forced_signum,
          fault_address, fault_code)) {
      spill_state->header.spill_reason = 0;
      if (refresh_poly_auto_spill() < 0) {
        poly_write_literal_stderr(
          "POLYEXEC_FAIL: auto-spill refresh failed during fault delivery\n");
        _exit(125);
      }
      resume_info->buffer = (uint64_t) (uintptr_t) spill_state;
      resume_info->mode = spill_state->header.current_mode;
      return;
    }
    report_poly_spill_page_fault();
    _exit(139);
  }

  poly_write_literal_stderr("POLYEXEC_FAIL: unsupported Poly auto-spill reason\n");
  _exit(125);
}

__attribute__((naked, noinline, used))
static void poly_auto_spill_resume_trampoline(void) {
  __asm__(
    "1:\n"
    "movl $1, %eax\n"
    "xchgl %eax, poly_auto_spill_resume_stack_lock(%rip)\n"
    "testl %eax, %eax\n"
    "jnz 1b\n"
    "leaq poly_auto_spill_resume_stack+65536(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq %r11\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "movq %rsp, %rdi\n"
    "call poly_auto_spill_resume_dispatch\n"
    "movq (%rsp), %rax\n"
    "movq %rax, 88(%rbp)\n"
    "movq 8(%rsp), %rax\n"
    "movq %rax, 8(%rbp)\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq %r15, poly_auto_spill_resume_mode(%rip)\n"
    "movq %rdx, %rax\n"
    POLY_OP_PRESTORE
    "movl poly_auto_spill_resume_mode(%rip), %edi\n"
    "movl %edi, %r15d\n"
    "movl $0, poly_auto_spill_resume_stack_lock(%rip)\n"
    POLY_X86_CTRL_PENTER_MODE_ASM
    "ud2\n");
}

static int install_poly_auto_spill(void) {
  memset(&poly_auto_spill_state, 0, sizeof(poly_auto_spill_state));
  poly_auto_spill_state_active = &poly_auto_spill_state;

  if (poly_install_auto_spill_signal_actions() < 0)
    return -1;

  const uint64_t buffer =
    (uint64_t) (uintptr_t) poly_auto_spill_active_state();
  const uint64_t resume = (uint64_t) (uintptr_t)
    poly_auto_spill_resume_trampoline;
  poly_auto_spill_installed = 1;
  if (refresh_poly_auto_spill() < 0) {
    poly_auto_spill_installed = 0;
    fprintf(stderr,
      "POLYEXEC_FAIL: auto-spill setup failed buffer=0x%llx resume=0x%llx\n",
      (unsigned long long) buffer, (unsigned long long) resume);
    return -1;
  }
  printf("POLYEXEC_AUTO_SPILL: buffer=0x%llx resume=0x%llx\n",
    (unsigned long long) buffer, (unsigned long long) resume);
  printf("POLYEXEC_V2_AUTO_SPILL: descriptor=0x%llx event=0x%llx generation=%llu\n",
    (unsigned long long) (uintptr_t) &poly_auto_spill_descriptor,
    (unsigned long long) (uintptr_t) &poly_auto_spill_event_frame,
    (unsigned long long) poly_auto_spill_descriptor.generation);
  return 0;
}

static void clear_poly_auto_spill(void) {
  if (!poly_auto_spill_installed)
    return;
  (void) poly_spill_desc_set(0, 0);
  (void) poly_event_ptr_set(0, 0);
  poly_auto_spill_installed = 0;
}

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

struct poly_linux_generic_epoll_event {
  uint32_t events;
  uint32_t pad;
  uint64_t data;
};

struct poly_riscv_hwprobe_pair {
  int64_t key;
  uint64_t value;
};

struct poly_x86_epoll_event {
  uint32_t events;
  uint64_t data;
} __attribute__((packed));

struct poly_utsname {
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
  char domainname[65];
};

static uint64_t align_down_u64(uint64_t value, uint64_t alignment) {
  return value & ~(alignment - 1);
}

static uint64_t align_up_u64(uint64_t value, uint64_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

static int align_up_size(size_t value, size_t alignment, size_t *result) {
  if (alignment <= 1) {
    *result = value;
    return 0;
  }
  if ((alignment & (alignment - 1)) != 0 ||
      value > SIZE_MAX - (alignment - 1))
    return -1;
  *result = (value + alignment - 1) & ~(alignment - 1);
  return 0;
}

static int reserve_process_tls_range(size_t *total_size, uint64_t memsz,
    uint64_t alignment, size_t *offset) {
  if (memsz == 0) {
    *offset = 0;
    return 0;
  }
  if (memsz > MAX_PROCESS_TLS_BYTES || alignment > MAX_PROCESS_TLS_BYTES)
    return -1;

  size_t aligned = 0;
  const size_t tls_alignment = alignment ? (size_t) alignment : 1;
  if (align_up_size(*total_size, tls_alignment, &aligned) < 0 ||
      aligned > MAX_PROCESS_TLS_BYTES ||
      memsz > MAX_PROCESS_TLS_BYTES - aligned)
    return -1;
  *offset = aligned;
  *total_size = aligned + (size_t) memsz;
  return 0;
}

static int record_load_segment(struct poly_program *program,
    const Elf64_Phdr *phdr) {
  if (program->load_segment_count >= MAX_LOAD_SEGMENTS) {
    fprintf(stderr, "POLYEXEC_FAIL: too many PT_LOAD segments: %s\n",
      program->path);
    return -1;
  }
  program->load_segments[program->load_segment_count].vaddr = phdr->p_vaddr;
  program->load_segments[program->load_segment_count].memsz = phdr->p_memsz;
  program->load_segments[program->load_segment_count].flags = phdr->p_flags;
  program->load_segment_count++;
  return 0;
}

static uint64_t read_u64_le(const uint8_t *bytes) {
  uint64_t value = 0;
  for (unsigned n = 0; n < 8; n++)
    value |= (uint64_t) bytes[n] << (n * 8);
  return value;
}

static uint32_t read_u32_le(const uint8_t *bytes) {
  uint32_t value = 0;
  for (unsigned n = 0; n < 4; n++)
    value |= (uint32_t) bytes[n] << (n * 8);
  return value;
}

static uint16_t read_u16_le(const uint8_t *bytes) {
  uint16_t value = 0;
  for (unsigned n = 0; n < 2; n++)
    value |= (uint16_t) bytes[n] << (n * 8);
  return value;
}

static int process_bridge_kind_from_name(const char *name) {
  if (strcmp(name, "default") == 0)
    return POLY_PROCESS_BRIDGE_DEFAULT;
  if (strcmp(name, "vec128_u32") == 0)
    return POLY_PROCESS_BRIDGE_VEC128_U32;
  if (strcmp(name, "compact_u32_f32") == 0)
    return POLY_PROCESS_BRIDGE_COMPACT_U32_F32;
  if (strcmp(name, "compact_f32_u32") == 0)
    return POLY_PROCESS_BRIDGE_COMPACT_F32_U32;
  if (strcmp(name, "u64_stack9") == 0)
    return POLY_PROCESS_BRIDGE_U64_STACK9;
  if (strcmp(name, "fp64") == 0)
    return POLY_PROCESS_BRIDGE_FP64;
  if (strcmp(name, "fp32") == 0)
    return POLY_PROCESS_BRIDGE_FP32;
  if (strcmp(name, "aarch64_hfa3_f32_ret") == 0)
    return POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET;
  if (strcmp(name, "aarch64_hfa4_f32_ret") == 0)
    return POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET;
  if (strcmp(name, "aarch64_hfa3_f64_ret") == 0)
    return POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET;
  if (strcmp(name, "aarch64_hfa4_f64_ret") == 0)
    return POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET;
  if (strcmp(name, "aarch64_hfa3_f64_arg") == 0)
    return POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_ARG;
  if (strcmp(name, "aarch64_hfa4_f64_arg") == 0)
    return POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG;
  if (strcmp(name, "aarch64_hfa3_f32_arg") == 0)
    return POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG;
  if (strcmp(name, "aarch64_hfa4_f32_arg") == 0)
    return POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG;
  if (strcmp(name, "fpair32_arg") == 0)
    return POLY_PROCESS_BRIDGE_FPAIR32_ARG;
  if (strcmp(name, "fpair64_arg") == 0)
    return POLY_PROCESS_BRIDGE_FPAIR64_ARG;
  if (strcmp(name, "fpair32_ret") == 0)
    return POLY_PROCESS_BRIDGE_FPAIR32_RET;
  if (strcmp(name, "fpair64_ret") == 0)
    return POLY_PROCESS_BRIDGE_FPAIR64_RET;
  if (strcmp(name, "sret_x86_sysv") == 0)
    return POLY_PROCESS_BRIDGE_SRET_X86_SYSV;
  if (strcmp(name, "native_sret") == 0)
    return POLY_PROCESS_BRIDGE_NATIVE_SRET;
  return -1;
}

static int append_process_bridge_spec(struct poly_program *program,
    const char *symbol, size_t symbol_len, const char *bridge,
    size_t bridge_len, const char *source) {
  if (symbol_len == 0 || symbol_len >= sizeof(program->bridge_specs[0].symbol) ||
      bridge_len == 0 || bridge_len >= 40 ||
      program->bridge_spec_count >= MAX_PROCESS_BRIDGE_SPECS) {
    fprintf(stderr, "POLYEXEC_FAIL: bad Poly ABI metadata line: %s\n",
      source);
    return -1;
  }

  char bridge_name[40];
  memcpy(bridge_name, bridge, bridge_len);
  bridge_name[bridge_len] = '\0';
  const int bridge_kind = process_bridge_kind_from_name(bridge_name);
  if (bridge_kind < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unknown Poly ABI bridge kind: %s\n",
      bridge_name);
    return -1;
  }

  struct poly_process_bridge_spec *spec =
    &program->bridge_specs[program->bridge_spec_count++];
  memcpy(spec->symbol, symbol, symbol_len);
  spec->symbol[symbol_len] = '\0';
  spec->bridge_kind = (uint8_t) bridge_kind;
  return 0;
}

static int parse_process_bridge_specs_text(struct poly_program *program,
    const char *desc, size_t desc_size, const char *source) {
  const char *line = desc;
  const char *end = desc + desc_size;
  while (line < end) {
    const char *line_end = memchr(line, '\n', (size_t) (end - line));
    if (!line_end)
      line_end = end;
    while (line < line_end && (*line == ' ' || *line == '\t'))
      line++;
    const char *space = line;
    while (space < line_end && *space != ' ' && *space != '\t')
      space++;
    const char *bridge = space;
    while (bridge < line_end && (*bridge == ' ' || *bridge == '\t'))
      bridge++;
    const char *bridge_end = bridge;
    while (bridge_end < line_end && *bridge_end != ' ' &&
           *bridge_end != '\t')
      bridge_end++;
    if (line < space && bridge < bridge_end &&
        append_process_bridge_spec(program, line, (size_t) (space - line),
          bridge, (size_t) (bridge_end - bridge), source) < 0)
      return -1;
    line = line_end < end ? line_end + 1 : end;
  }
  return 0;
}

static int parse_process_bridge_specs_notes(struct poly_program *program,
    const unsigned char *data, size_t size, size_t offset, size_t note_size) {
  if (offset > size || note_size > size - offset)
    return -1;
  const size_t end = offset + note_size;
  while (offset < end) {
    if (end - offset < 12)
      return -1;
    const uint32_t namesz = read_u32_le(data + offset);
    const uint32_t descsz = read_u32_le(data + offset + 4);
    offset += 12;
    if (namesz > end - offset)
      return -1;
    const char *note_name = (const char *) (data + offset);
    offset += (namesz + 3U) & ~3U;
    if (offset > end || descsz > end - offset)
      return -1;
    const char *desc = (const char *) (data + offset);
    offset += (descsz + 3U) & ~3U;
    if (offset > end)
      return -1;
    if (namesz == 8 && memcmp(note_name, "POLYABI", 8) == 0 &&
        parse_process_bridge_specs_text(program, desc, descsz,
          program->path) < 0)
      return -1;
  }
  return 0;
}

static int process_bridge_kind_for_symbol(const struct poly_program *program,
    const char *symbol_name) {
  if (!symbol_name)
    return POLY_PROCESS_BRIDGE_DEFAULT;
  for (size_t n = 0; n < program->bridge_spec_count; n++) {
    if (strcmp(program->bridge_specs[n].symbol, symbol_name) == 0)
      return program->bridge_specs[n].bridge_kind;
  }
  return POLY_PROCESS_BRIDGE_DEFAULT;
}

static uint32_t process_signature_slot_for_bridge_kind(int bridge_kind) {
  if (bridge_kind == POLY_PROCESS_BRIDGE_VEC128_U32)
    return process_vec128_signature_slot;
  if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_U32_F32)
    return process_compact_u32_f32_signature_slot;
  if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_F32_U32)
    return process_compact_f32_u32_signature_slot;
  if (bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9)
    return POLY_ABI_SIGNATURE_SLOT_EXCHANGE;
  if (bridge_kind == POLY_PROCESS_BRIDGE_FP64)
    return process_fp64_signature_slot;
  if (bridge_kind == POLY_PROCESS_BRIDGE_FP32)
    return process_fp32_signature_slot;
  if (bridge_kind == POLY_PROCESS_BRIDGE_SRET_X86_SYSV)
    return process_sret_signature_slot;
  if (bridge_kind == POLY_PROCESS_BRIDGE_NATIVE_SRET)
    return process_native_sret_signature_slot;
  if (bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG)
    return bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG ?
      process_fp64_signature_slot :
      POLY_ABI_SIGNATURE_SLOT_X86_SYSV_REGS_FP128_RET;
  if (bridge_kind == POLY_PROCESS_BRIDGE_FPAIR32_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_FPAIR64_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_FPAIR32_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_FPAIR64_RET)
    return POLY_ABI_SIGNATURE_SLOT_X86_SYSV_REGS_FP128_RET;
  return process_native_signature_slot;
}

static void write_u64_le(uint8_t *bytes, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    bytes[n] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static uint32_t relative_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_RELATIVE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_RELATIVE;
  if (arch == POLY_ARCH_X86)
    return R_X86_64_RELATIVE;
  return 0;
}

static int symbolic_64_reloc_type_for_arch(int arch, uint32_t type) {
  if (arch == POLY_ARCH_AARCH64)
    return type == R_AARCH64_ABS64 || type == R_AARCH64_GLOB_DAT ||
      type == R_AARCH64_JUMP_SLOT;
  if (arch == POLY_ARCH_RISCV)
    return type == R_RISCV_64 || type == R_RISCV_JUMP_SLOT;
  if (arch == POLY_ARCH_X86)
    return type == R_X86_64_64 || type == R_X86_64_GLOB_DAT ||
      type == R_X86_64_JUMP_SLOT;
  return 0;
}

static uint32_t irelative_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_IRELATIVE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_IRELATIVE;
  if (arch == POLY_ARCH_X86)
    return R_X86_64_IRELATIVE;
  return UINT32_MAX;
}

static uint32_t tls_tprel_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_TLS_TPREL64;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_TLS_TPREL64;
  if (arch == POLY_ARCH_X86)
    return R_X86_64_TPOFF64;
  return UINT32_MAX;
}

static uint32_t none_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_NONE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_NONE;
  if (arch == POLY_ARCH_X86)
    return R_X86_64_NONE;
  return UINT32_MAX;
}

static uint32_t copy_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_COPY;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_COPY;
  if (arch == POLY_ARCH_X86)
    return R_X86_64_COPY;
  return UINT32_MAX;
}

static int elf_vaddr_to_image_offset(const struct poly_program *program,
    uint64_t vaddr, uint64_t size, size_t *offset) {
  if (vaddr < program->base_vaddr || vaddr > UINT64_MAX - size)
    return -1;
  uint64_t image_offset = vaddr - program->base_vaddr;
  if (image_offset > program->code_size || size > program->code_size - image_offset)
    return -1;
  *offset = (size_t) image_offset;
  return 0;
}

static int dynamic_symbol_count_from_hash(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t symtab_vaddr, uint64_t syment,
    uint64_t hash_vaddr, uint64_t gnu_hash_vaddr, size_t *symbol_count) {
  *symbol_count = 0;
  if (hash_vaddr) {
    size_t hash_offset = 0;
    if (elf_vaddr_to_image_offset(program, hash_vaddr, 8, &hash_offset) < 0)
      return -1;
    *symbol_count = read_u32_le(loaded_image + hash_offset + 4);
    return *symbol_count != 0 && *symbol_count <= 4096 ? 0 : -1;
  }

  if (!gnu_hash_vaddr)
    return -1;

  size_t symtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, syment,
        &symtab_offset) < 0)
    return -1;

  size_t hash_offset = 0;
  if (elf_vaddr_to_image_offset(program, gnu_hash_vaddr, 16,
        &hash_offset) < 0)
    return -1;

  const uint32_t nbuckets = read_u32_le(loaded_image + hash_offset);
  const uint32_t symoffset = read_u32_le(loaded_image + hash_offset + 4);
  const uint32_t bloom_size = read_u32_le(loaded_image + hash_offset + 8);
  if (nbuckets == 0 || bloom_size == 0)
    return -1;

  const uint64_t buckets_offset = (uint64_t) hash_offset + 16 +
    (uint64_t) bloom_size * sizeof(uint64_t);
  const uint64_t buckets_size = (uint64_t) nbuckets * sizeof(uint32_t);
  if (buckets_offset > program->code_size ||
      buckets_size > program->code_size - buckets_offset)
    return -1;

  const uint64_t chains_offset = buckets_offset + buckets_size;
  if (chains_offset > program->code_size ||
      symtab_offset > program->code_size ||
      (program->code_size - symtab_offset) / syment < symoffset)
    return -1;

  const size_t max_symbols = (program->code_size - symtab_offset) / syment;
  size_t count = symoffset;
  for (uint32_t n = 0; n < nbuckets; n++) {
    uint32_t index = read_u32_le(loaded_image + buckets_offset +
      (uint64_t) n * sizeof(uint32_t));
    if (index == 0)
      continue;
    if (index < symoffset || index >= max_symbols)
      return -1;

    while (1) {
      const uint64_t chain_offset = chains_offset +
        (uint64_t) (index - symoffset) * sizeof(uint32_t);
      if (chain_offset > program->code_size ||
          sizeof(uint32_t) > program->code_size - chain_offset)
        return -1;
      const uint32_t chain = read_u32_le(loaded_image + chain_offset);
      if ((size_t) index + 1 > count)
        count = (size_t) index + 1;
      if (chain & 1)
        break;
      index++;
      if (index >= max_symbols)
        return -1;
    }
  }

  *symbol_count = count;
  return count != 0 && count <= 4096 ? 0 : -1;
}

static int resolve_same_image_reloc_symbol(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t symtab_vaddr, uint64_t syment,
    uint64_t hash_vaddr, uint64_t gnu_hash_vaddr, uint64_t symbol_index,
    uint64_t *symbol_vaddr, uint8_t *symbol_type) {
  size_t symbol_count = 0;
  if (!symtab_vaddr || syment < sizeof(Elf64_Sym) ||
      dynamic_symbol_count_from_hash(program, loaded_image, symtab_vaddr,
        syment, hash_vaddr, gnu_hash_vaddr, &symbol_count) < 0 ||
      symbol_index >= symbol_count)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size,
        &symtab_offset) < 0)
    return -1;

  const Elf64_Sym *sym = (const Elf64_Sym *) (loaded_image + symtab_offset +
    symbol_index * syment);
  if (sym->st_shndx == SHN_UNDEF)
    return -1;
  *symbol_vaddr = sym->st_value;
  if (symbol_type)
    *symbol_type = ELF64_ST_TYPE(sym->st_info);
  return 0;
}

static int dynamic_symbol_name_by_index(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t symtab_vaddr, uint64_t strtab_vaddr,
    uint64_t strsz, uint64_t syment, uint64_t hash_vaddr,
    uint64_t gnu_hash_vaddr, uint64_t symbol_index, const char **symbol_name,
    uint8_t *symbol_info) {
  size_t symbol_count = 0;
  if (!symtab_vaddr || !strtab_vaddr || !strsz ||
      syment < sizeof(Elf64_Sym) ||
      dynamic_symbol_count_from_hash(program, loaded_image, symtab_vaddr,
        syment, hash_vaddr, gnu_hash_vaddr, &symbol_count) < 0 ||
      symbol_index >= symbol_count)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size,
        &symtab_offset) < 0)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;

  const Elf64_Sym *sym = (const Elf64_Sym *) (loaded_image + symtab_offset +
    symbol_index * syment);
  if (sym->st_name >= strsz)
    return -1;
  *symbol_name = (const char *) (loaded_image + strtab_offset + sym->st_name);
  if (symbol_info)
    *symbol_info = sym->st_info;
  return 0;
}

static int dynamic_symbol_by_index(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t symtab_vaddr, uint64_t strtab_vaddr,
    uint64_t strsz, uint64_t syment, uint64_t hash_vaddr,
    uint64_t gnu_hash_vaddr, uint64_t symbol_index, const Elf64_Sym **symbol,
    const char **symbol_name) {
  size_t symbol_count = 0;
  if (!symtab_vaddr || !strtab_vaddr || !strsz ||
      syment < sizeof(Elf64_Sym) ||
      dynamic_symbol_count_from_hash(program, loaded_image, symtab_vaddr,
        syment, hash_vaddr, gnu_hash_vaddr, &symbol_count) < 0 ||
      symbol_index >= symbol_count)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size,
        &symtab_offset) < 0)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;

  const Elf64_Sym *sym = (const Elf64_Sym *) (loaded_image + symtab_offset +
    symbol_index * syment);
  if (sym->st_name >= strsz)
    return -1;
  *symbol = sym;
  *symbol_name = (const char *) (loaded_image + strtab_offset + sym->st_name);
  return 0;
}

static int dynamic_symbol_table_info(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t *symtab_vaddr,
    uint64_t *strtab_vaddr, uint64_t *strsz, uint64_t *syment,
    uint64_t *hash_vaddr, uint64_t *gnu_hash_vaddr) {
  *symtab_vaddr = 0;
  *strtab_vaddr = 0;
  *strsz = 0;
  *syment = sizeof(Elf64_Sym);
  *hash_vaddr = 0;
  *gnu_hash_vaddr = 0;
  if (!program->dynamic_size)
    return -1;

  const Elf64_Dyn *dyn =
    (const Elf64_Dyn *) (loaded_image + program->dynamic_offset);
  const size_t dyn_count = program->dynamic_size / sizeof(Elf64_Dyn);
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_SYMTAB: *symtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRTAB: *strtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRSZ: *strsz = dyn[n].d_un.d_val; break;
      case DT_SYMENT: *syment = dyn[n].d_un.d_val; break;
      case DT_HASH: *hash_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_GNU_HASH: *gnu_hash_vaddr = dyn[n].d_un.d_ptr; break;
      default: break;
    }
  }

  return *symtab_vaddr && *strtab_vaddr && *strsz &&
    *syment >= sizeof(Elf64_Sym) ? 0 : -1;
}

static void dynamic_version_table_info(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t *versym_vaddr,
    uint64_t *verdef_vaddr, uint64_t *verdef_num, uint64_t *verneed_vaddr,
    uint64_t *verneed_num) {
  *versym_vaddr = 0;
  *verdef_vaddr = 0;
  *verdef_num = 0;
  *verneed_vaddr = 0;
  *verneed_num = 0;
  if (!program->dynamic_size)
    return;

  const Elf64_Dyn *dyn =
    (const Elf64_Dyn *) (loaded_image + program->dynamic_offset);
  const size_t dyn_count = program->dynamic_size / sizeof(Elf64_Dyn);
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_VERSYM: *versym_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_VERDEF: *verdef_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_VERDEFNUM: *verdef_num = dyn[n].d_un.d_val; break;
      case DT_VERNEED: *verneed_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_VERNEEDNUM: *verneed_num = dyn[n].d_un.d_val; break;
      default: break;
    }
  }
}

static int read_symbol_version_index(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t versym_vaddr, uint64_t symbol_index,
    uint16_t *version_index) {
  *version_index = 0;
  if (!versym_vaddr)
    return 0;
  size_t versym_offset = 0;
  if (elf_vaddr_to_image_offset(program, versym_vaddr,
        (symbol_index + 1) * sizeof(uint16_t), &versym_offset) < 0)
    return -1;
  *version_index = read_u16_le(loaded_image + versym_offset +
    symbol_index * sizeof(uint16_t));
  return 0;
}

static int version_string_from_verneed(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t strtab_vaddr, uint64_t strsz,
    uint64_t verneed_vaddr, uint64_t verneed_num, uint16_t version_index,
    const char **version_name) {
  *version_name = NULL;
  if (!verneed_vaddr || !verneed_num)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;
  size_t need_offset = 0;
  if (elf_vaddr_to_image_offset(program, verneed_vaddr, sizeof(Elf64_Verneed),
        &need_offset) < 0)
    return -1;

  for (uint64_t need_index = 0; need_index < verneed_num; need_index++) {
    if (need_offset > program->code_size ||
        sizeof(Elf64_Verneed) > program->code_size - need_offset)
      return -1;
    const Elf64_Verneed *need =
      (const Elf64_Verneed *) (loaded_image + need_offset);
    size_t aux_offset = need_offset + need->vn_aux;
    for (uint16_t aux_index = 0; aux_index < need->vn_cnt; aux_index++) {
      if (aux_offset > program->code_size ||
          sizeof(Elf64_Vernaux) > program->code_size - aux_offset)
        return -1;
      const Elf64_Vernaux *aux =
        (const Elf64_Vernaux *) (loaded_image + aux_offset);
      if ((aux->vna_other & 0x7fffU) == version_index) {
        if (aux->vna_name >= strsz)
          return -1;
        *version_name = (const char *) (loaded_image + strtab_offset +
          aux->vna_name);
        return 0;
      }
      if (!aux->vna_next)
        break;
      aux_offset += aux->vna_next;
    }
    if (!need->vn_next)
      break;
    need_offset += need->vn_next;
  }

  return -1;
}

static int version_string_from_verdef(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t strtab_vaddr, uint64_t strsz,
    uint64_t verdef_vaddr, uint64_t verdef_num, uint16_t version_index,
    const char **version_name) {
  *version_name = NULL;
  if (!verdef_vaddr || !verdef_num)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;
  size_t def_offset = 0;
  if (elf_vaddr_to_image_offset(program, verdef_vaddr, sizeof(Elf64_Verdef),
        &def_offset) < 0)
    return -1;

  for (uint64_t def_index = 0; def_index < verdef_num; def_index++) {
    if (def_offset > program->code_size ||
        sizeof(Elf64_Verdef) > program->code_size - def_offset)
      return -1;
    const Elf64_Verdef *def =
      (const Elf64_Verdef *) (loaded_image + def_offset);
    if ((def->vd_ndx & 0x7fffU) == version_index) {
      const size_t aux_offset = def_offset + def->vd_aux;
      if (aux_offset > program->code_size ||
          sizeof(Elf64_Verdaux) > program->code_size - aux_offset)
        return -1;
      const Elf64_Verdaux *aux =
        (const Elf64_Verdaux *) (loaded_image + aux_offset);
      if (aux->vda_name >= strsz)
        return -1;
      *version_name = (const char *) (loaded_image + strtab_offset +
        aux->vda_name);
      return 0;
    }
    if (!def->vd_next)
      break;
    def_offset += def->vd_next;
  }

  return -1;
}

static int relocation_requested_version_name(
    const struct poly_program *program, const uint8_t *loaded_image,
    uint64_t strtab_vaddr, uint64_t strsz, uint64_t symbol_index,
    const char **version_name) {
  *version_name = NULL;
  uint64_t versym_vaddr = 0, verdef_vaddr = 0, verdef_num = 0;
  uint64_t verneed_vaddr = 0, verneed_num = 0;
  dynamic_version_table_info(program, loaded_image, &versym_vaddr,
    &verdef_vaddr, &verdef_num, &verneed_vaddr, &verneed_num);
  if (!versym_vaddr)
    return 0;

  uint16_t version_index = 0;
  if (read_symbol_version_index(program, loaded_image, versym_vaddr,
        symbol_index, &version_index) < 0)
    return -1;
  version_index &= 0x7fffU;
  if (version_index <= 1)
    return 0;

  return version_string_from_verneed(program, loaded_image, strtab_vaddr,
    strsz, verneed_vaddr, verneed_num, version_index, version_name);
}

static int symbol_definition_matches_version(
    const struct poly_program *program, const uint8_t *loaded_image,
    uint64_t strtab_vaddr, uint64_t strsz, size_t symbol_index,
    const char *requested_version) {
  if (!requested_version)
    return 1;

  uint64_t versym_vaddr = 0, verdef_vaddr = 0, verdef_num = 0;
  uint64_t verneed_vaddr = 0, verneed_num = 0;
  dynamic_version_table_info(program, loaded_image, &versym_vaddr,
    &verdef_vaddr, &verdef_num, &verneed_vaddr, &verneed_num);
  if (!versym_vaddr)
    return 0;

  uint16_t version_index = 0;
  if (read_symbol_version_index(program, loaded_image, versym_vaddr,
        symbol_index, &version_index) < 0)
    return 0;
  version_index &= 0x7fffU;
  if (version_index <= 1)
    return 0;

  const char *defined_version = NULL;
  if (version_string_from_verdef(program, loaded_image, strtab_vaddr, strsz,
        verdef_vaddr, verdef_num, version_index, &defined_version) < 0)
    return 0;
  return strcmp(defined_version, requested_version) == 0;
}

static int symbol_is_dependency_export(const Elf64_Sym *sym) {
  const unsigned bind = ELF64_ST_BIND(sym->st_info);
  const unsigned visibility = ELF64_ST_VISIBILITY(sym->st_other);
  return sym->st_shndx != SHN_UNDEF &&
    (bind == STB_GLOBAL || bind == STB_WEAK) &&
    (visibility == STV_DEFAULT || visibility == STV_PROTECTED);
}

static int resolve_loaded_program_symbol_ex(const struct poly_program *program,
    const uint8_t *loaded_image, const char *symbol_name,
    const char *requested_version, uint64_t *symbol_value, uint8_t *symbol_type,
    uint64_t *ifunc_resolver_vaddr) {
  uint64_t symtab_vaddr = 0, strtab_vaddr = 0, strsz = 0;
  uint64_t syment = sizeof(Elf64_Sym), hash_vaddr = 0, gnu_hash_vaddr = 0;
  if (dynamic_symbol_table_info(program, loaded_image, &symtab_vaddr,
        &strtab_vaddr, &strsz, &syment, &hash_vaddr, &gnu_hash_vaddr) < 0)
    return -1;

  size_t symbol_count = 0;
  if (dynamic_symbol_count_from_hash(program, loaded_image, symtab_vaddr,
        syment, hash_vaddr, gnu_hash_vaddr, &symbol_count) < 0)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size,
        &symtab_offset) < 0)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;

  for (size_t index = 0; index < symbol_count; index++) {
    const Elf64_Sym *sym = (const Elf64_Sym *) (loaded_image +
      symtab_offset + (uint64_t) index * syment);
    if (sym->st_name >= strsz || !symbol_is_dependency_export(sym))
      continue;
    const char *name = (const char *) (loaded_image + strtab_offset +
      sym->st_name);
    if (strcmp(name, symbol_name) != 0)
      continue;
    if (!symbol_definition_matches_version(program, loaded_image, strtab_vaddr,
          strsz, index, requested_version))
      continue;
    const uint8_t type = ELF64_ST_TYPE(sym->st_info);
    if (type == STT_GNU_IFUNC && !ifunc_resolver_vaddr)
      return -1;
    if (type == STT_GNU_IFUNC) {
      *symbol_value = 0;
      *ifunc_resolver_vaddr = sym->st_value;
    }
    else {
      *symbol_value = (uint64_t) (uintptr_t) loaded_image -
        program->base_vaddr + sym->st_value;
      if (ifunc_resolver_vaddr)
        *ifunc_resolver_vaddr = 0;
    }
    if (symbol_type)
      *symbol_type = type;
    return 0;
  }
  return -1;
}

static int resolve_loaded_program_tls_symbol_ex(
    const struct poly_program *program, const uint8_t *loaded_image,
    const char *symbol_name, const char *requested_version,
    uint64_t *tls_offset) {
  uint64_t symtab_vaddr = 0, strtab_vaddr = 0, strsz = 0;
  uint64_t syment = sizeof(Elf64_Sym), hash_vaddr = 0, gnu_hash_vaddr = 0;
  if (dynamic_symbol_table_info(program, loaded_image, &symtab_vaddr,
        &strtab_vaddr, &strsz, &syment, &hash_vaddr, &gnu_hash_vaddr) < 0)
    return -1;

  size_t symbol_count = 0;
  if (dynamic_symbol_count_from_hash(program, loaded_image, symtab_vaddr,
        syment, hash_vaddr, gnu_hash_vaddr, &symbol_count) < 0)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size,
        &symtab_offset) < 0)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;

  for (size_t index = 0; index < symbol_count; index++) {
    const Elf64_Sym *sym = (const Elf64_Sym *) (loaded_image +
      symtab_offset + (uint64_t) index * syment);
    if (sym->st_name >= strsz || !symbol_is_dependency_export(sym) ||
        ELF64_ST_TYPE(sym->st_info) != STT_TLS)
      continue;
    const char *name = (const char *) (loaded_image + strtab_offset +
      sym->st_name);
    if (strcmp(name, symbol_name) != 0)
      continue;
    if (!symbol_definition_matches_version(program, loaded_image, strtab_vaddr,
          strsz, index, requested_version))
      continue;
    *tls_offset = (uint64_t) program->tls_offset + sym->st_value;
    return 0;
  }
  return -1;
}

static int resolve_loaded_dependency_symbol_at_depth(
    const struct poly_program *program, int caller_arch,
    const char *symbol_name, const char *requested_version,
    uint64_t *symbol_value, uint8_t *symbol_type,
    uint8_t *trampoline_code, size_t prefix_size, uint64_t return_pc,
    uint8_t *scratch, size_t depth) {
  if (depth >= MAX_PROCESS_DEP_DEPTH)
    return -1;

  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_process_dependency *dep = &program->deps[d];
    if (!dep->program || !dep->loaded_image)
      continue;

    uint64_t ifunc_resolver_vaddr = 0;
    uint8_t resolved_type = 0;
    if (resolve_loaded_program_symbol_ex(dep->program, dep->loaded_image,
          symbol_name, requested_version, symbol_value, &resolved_type,
          &ifunc_resolver_vaddr) == 0) {
      if (resolved_type == STT_GNU_IFUNC &&
          run_irelative_resolver(dep->program, dep->loaded_image,
            trampoline_code, prefix_size, return_pc, scratch,
            ifunc_resolver_vaddr, symbol_value) < 0)
        return -1;
      if (symbol_type)
        *symbol_type = resolved_type;
      const int bridge_kind =
        process_bridge_kind_for_symbol(dep->program, symbol_name);
      const uint32_t signature_slot =
        process_signature_slot_for_bridge_kind(bridge_kind);
      if (caller_arch != dep->program->arch &&
          (resolved_type == STT_FUNC || resolved_type == STT_NOTYPE ||
           resolved_type == STT_GNU_IFUNC) &&
          emit_process_cross_isa_call_stub(caller_arch, dep->program->arch,
            *symbol_value, bridge_kind, signature_slot, symbol_value) < 0) {
        fprintf(stderr,
          "POLYEXEC_FAIL: cross-ISA relocation stub failed symbol=%s caller=%d callee=%d bridge=%d slot=%u\n",
          symbol_name, caller_arch, dep->program->arch, bridge_kind,
          signature_slot);
        return -1;
      }
      return 0;
    }
    if (resolve_loaded_dependency_symbol_at_depth(dep->program, caller_arch,
          symbol_name, requested_version, symbol_value, symbol_type,
          trampoline_code, prefix_size, return_pc, scratch, depth + 1) == 0)
      return 0;
  }
  return -1;
}

static int resolve_loaded_dependency_symbol(const struct poly_program *program,
    const char *symbol_name, const char *requested_version,
    uint64_t *symbol_value, uint8_t *symbol_type,
    uint8_t *trampoline_code, size_t prefix_size, uint64_t return_pc,
    uint8_t *scratch) {
  return resolve_loaded_dependency_symbol_at_depth(program, program->arch,
    symbol_name, requested_version, symbol_value, symbol_type,
    trampoline_code, prefix_size, return_pc, scratch, 0);
}

static int resolve_loaded_dependency_tls_symbol_at_depth(
    const struct poly_program *program, const char *symbol_name,
    const char *requested_version, uint64_t *tls_offset, size_t depth) {
  if (depth >= MAX_PROCESS_DEP_DEPTH)
    return -1;

  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_process_dependency *dep = &program->deps[d];
    if (!dep->program || !dep->loaded_image)
      continue;

    if (resolve_loaded_program_tls_symbol_ex(dep->program, dep->loaded_image,
          symbol_name, requested_version, tls_offset) == 0)
      return 0;
    if (resolve_loaded_dependency_tls_symbol_at_depth(dep->program,
          symbol_name, requested_version, tls_offset, depth + 1) == 0)
      return 0;
  }
  return -1;
}

static int resolve_loaded_dependency_tls_symbol(
    const struct poly_program *program, const char *symbol_name,
    const char *requested_version, uint64_t *tls_offset) {
  return resolve_loaded_dependency_tls_symbol_at_depth(program, symbol_name,
    requested_version, tls_offset, 0);
}

static int resolve_loaded_program_object_symbol_ex(
    const struct poly_program *program, const uint8_t *loaded_image,
    const char *symbol_name, const char *requested_version,
    uint64_t *symbol_vaddr, size_t *symbol_size) {
  uint64_t symtab_vaddr = 0, strtab_vaddr = 0, strsz = 0;
  uint64_t syment = sizeof(Elf64_Sym), hash_vaddr = 0, gnu_hash_vaddr = 0;
  if (dynamic_symbol_table_info(program, loaded_image, &symtab_vaddr,
        &strtab_vaddr, &strsz, &syment, &hash_vaddr, &gnu_hash_vaddr) < 0)
    return -1;

  size_t symbol_count = 0;
  if (dynamic_symbol_count_from_hash(program, loaded_image, symtab_vaddr,
        syment, hash_vaddr, gnu_hash_vaddr, &symbol_count) < 0)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size,
        &symtab_offset) < 0)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;

  for (size_t index = 0; index < symbol_count; index++) {
    const Elf64_Sym *sym = (const Elf64_Sym *) (loaded_image +
      symtab_offset + (uint64_t) index * syment);
    if (sym->st_name >= strsz || !symbol_is_dependency_export(sym))
      continue;
    const uint8_t type = ELF64_ST_TYPE(sym->st_info);
    if (type != STT_OBJECT && type != STT_NOTYPE)
      continue;
    const char *name = (const char *) (loaded_image + strtab_offset +
      sym->st_name);
    if (strcmp(name, symbol_name) != 0)
      continue;
    if (!symbol_definition_matches_version(program, loaded_image, strtab_vaddr,
          strsz, index, requested_version))
      continue;
    *symbol_vaddr = sym->st_value;
    *symbol_size = (size_t) sym->st_size;
    return 0;
  }
  return -1;
}

static int resolve_loaded_dependency_object_symbol_at_depth(
    const struct poly_program *program, const char *symbol_name,
    const char *requested_version, const struct poly_process_dependency **source_dep,
    uint64_t *source_vaddr, size_t *source_size, size_t depth) {
  if (depth >= MAX_PROCESS_DEP_DEPTH)
    return -1;

  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_process_dependency *dep = &program->deps[d];
    if (!dep->program || !dep->loaded_image)
      continue;

    if (resolve_loaded_program_object_symbol_ex(dep->program,
          dep->loaded_image, symbol_name, requested_version, source_vaddr,
          source_size) == 0) {
      *source_dep = dep;
      return 0;
    }
    if (resolve_loaded_dependency_object_symbol_at_depth(dep->program,
          symbol_name, requested_version, source_dep, source_vaddr,
          source_size, depth + 1) == 0)
      return 0;
  }
  return -1;
}

static int resolve_loaded_dependency_object_symbol(
    const struct poly_program *program, const char *symbol_name,
    const char *requested_version, const struct poly_process_dependency **source_dep,
    uint64_t *source_vaddr, size_t *source_size) {
  return resolve_loaded_dependency_object_symbol_at_depth(program, symbol_name,
    requested_version, source_dep, source_vaddr, source_size, 0);
}

static int resolve_root_scope_dependency_symbol(const struct poly_program *program,
    const char *symbol_name, const char *requested_version,
    uint64_t *symbol_value, uint8_t *symbol_type,
    uint8_t *trampoline_code, size_t prefix_size, uint64_t return_pc,
    uint8_t *scratch) {
  if (!program->scope_root_program || program == program->scope_root_program)
    return -1;
  return resolve_loaded_dependency_symbol_at_depth(program->scope_root_program,
    program->arch, symbol_name, requested_version, symbol_value, symbol_type,
    trampoline_code, prefix_size, return_pc, scratch, 0);
}

static int resolve_root_scope_dependency_tls_symbol(
    const struct poly_program *program, const char *symbol_name,
    const char *requested_version, uint64_t *tls_offset) {
  if (!program->scope_root_program || program == program->scope_root_program)
    return -1;
  return resolve_loaded_dependency_tls_symbol_at_depth(
    program->scope_root_program, symbol_name, requested_version, tls_offset, 0);
}

static int resolve_root_scope_dependency_object_symbol(
    const struct poly_program *program, const char *symbol_name,
    const char *requested_version, const struct poly_process_dependency **source_dep,
    uint64_t *source_vaddr, size_t *source_size) {
  if (!program->scope_root_program || program == program->scope_root_program)
    return -1;
  return resolve_loaded_dependency_object_symbol_at_depth(
    program->scope_root_program, symbol_name, requested_version, source_dep,
    source_vaddr, source_size, 0);
}

static int resolve_root_scope_symbol(const struct poly_program *program,
    const char *symbol_name, const char *requested_version,
    uint8_t *trampoline_code, size_t prefix_size, uint64_t return_pc,
    uint8_t *scratch, uint64_t *symbol_value, uint8_t *symbol_type) {
  if (!program->scope_root_program || !program->scope_root_loaded_image ||
      program == program->scope_root_program)
    return -1;
  uint64_t ifunc_resolver_vaddr = 0;
  uint8_t resolved_type = 0;
  if (resolve_loaded_program_symbol_ex(program->scope_root_program,
        program->scope_root_loaded_image, symbol_name, requested_version,
        symbol_value, &resolved_type, &ifunc_resolver_vaddr) < 0)
    return -1;
  if (resolved_type == STT_GNU_IFUNC) {
    if (run_irelative_resolver(program->scope_root_program,
          program->scope_root_loaded_image, trampoline_code, prefix_size,
          return_pc, scratch, ifunc_resolver_vaddr, symbol_value) < 0)
      return -1;
  }
  const int bridge_kind =
    process_bridge_kind_for_symbol(program->scope_root_program, symbol_name);
  const uint32_t signature_slot =
    process_signature_slot_for_bridge_kind(bridge_kind);
  if (program->arch != program->scope_root_program->arch &&
      (resolved_type == STT_FUNC || resolved_type == STT_NOTYPE ||
       resolved_type == STT_GNU_IFUNC) &&
      emit_process_cross_isa_call_stub(program->arch,
        program->scope_root_program->arch, *symbol_value, bridge_kind,
        signature_slot, symbol_value) < 0)
    return -1;
  if (symbol_type)
    *symbol_type = resolved_type;
  return 0;
}

static int resolve_root_scope_tls_symbol(const struct poly_program *program,
    const char *symbol_name, const char *requested_version,
    uint64_t *tls_offset) {
  if (!program->scope_root_program || !program->scope_root_loaded_image ||
      program == program->scope_root_program)
    return -1;
  return resolve_loaded_program_tls_symbol_ex(program->scope_root_program,
    program->scope_root_loaded_image, symbol_name, requested_version,
    tls_offset);
}

static int resolve_dependency_reloc_symbol(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t symtab_vaddr, uint64_t strtab_vaddr,
    uint64_t strsz, uint64_t syment, uint64_t hash_vaddr,
    uint64_t gnu_hash_vaddr, uint64_t symbol_index, uint64_t *symbol_value,
    uint8_t *symbol_type, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint64_t tls_get_addr_helper_pc, uint8_t *scratch) {
  const char *symbol_name = NULL;
  const char *requested_version = NULL;
  uint8_t unresolved_info = 0;
  if (dynamic_symbol_name_by_index(program, loaded_image, symtab_vaddr,
        strtab_vaddr, strsz, syment, hash_vaddr, gnu_hash_vaddr,
        symbol_index, &symbol_name, &unresolved_info) < 0)
    return -1;
  if (relocation_requested_version_name(program, loaded_image, strtab_vaddr,
        strsz, symbol_index, &requested_version) < 0)
    return -1;
  if ((program->arch == POLY_ARCH_X86 ||
       program->arch == POLY_ARCH_AARCH64 ||
       program->arch == POLY_ARCH_RISCV) &&
      tls_get_addr_helper_pc != 0 &&
      strcmp(symbol_name, "__tls_get_addr") == 0) {
    *symbol_value = tls_get_addr_helper_pc;
    if (symbol_type)
      *symbol_type = STT_FUNC;
    return 0;
  }
  if (resolve_root_scope_symbol(program, symbol_name, requested_version,
        trampoline_code,
        prefix_size, return_pc, scratch, symbol_value, symbol_type) == 0)
    return 0;
  if (resolve_root_scope_dependency_symbol(program, symbol_name,
        requested_version, symbol_value, symbol_type, trampoline_code,
        prefix_size, return_pc, scratch) == 0)
    return 0;
  if (resolve_loaded_dependency_symbol(program, symbol_name, requested_version,
        symbol_value, symbol_type, trampoline_code, prefix_size, return_pc,
        scratch) == 0)
    return 0;
  if (ELF64_ST_BIND(unresolved_info) == STB_WEAK) {
    *symbol_value = 0;
    if (symbol_type)
      *symbol_type = ELF64_ST_TYPE(unresolved_info);
    return 0;
  }
  fprintf(stderr,
    "POLYEXEC_FAIL: unresolved relocation symbol=%s arch=%d bind=%u type=%u\n",
    symbol_name ? symbol_name : "(null)", program->arch,
    (unsigned) ELF64_ST_BIND(unresolved_info),
    (unsigned) ELF64_ST_TYPE(unresolved_info));
  return -1;
}

static int apply_process_copy_relocation(const struct poly_program *program,
    uint8_t *loaded_image, uint64_t symtab_vaddr, uint64_t strtab_vaddr,
    uint64_t strsz, uint64_t syment, uint64_t hash_vaddr,
    uint64_t gnu_hash_vaddr, uint64_t symbol_index, uint64_t target_vaddr) {
  const Elf64_Sym *target_sym = NULL;
  const char *symbol_name = NULL;
  if (dynamic_symbol_by_index(program, loaded_image, symtab_vaddr,
        strtab_vaddr, strsz, syment, hash_vaddr, gnu_hash_vaddr,
        symbol_index, &target_sym, &symbol_name) < 0)
    return -1;

  const char *requested_version = NULL;
  if (relocation_requested_version_name(program, loaded_image, strtab_vaddr,
        strsz, symbol_index, &requested_version) < 0)
    return -1;

  const struct poly_process_dependency *source_dep = NULL;
  uint64_t source_vaddr = 0;
  size_t source_size = 0;
  if (resolve_root_scope_dependency_object_symbol(program, symbol_name,
        requested_version, &source_dep, &source_vaddr, &source_size) < 0 &&
      resolve_loaded_dependency_object_symbol(program, symbol_name,
        requested_version, &source_dep, &source_vaddr, &source_size) < 0)
    return -1;

  const size_t copy_size = target_sym->st_size ?
    (size_t) target_sym->st_size : source_size;
  if (copy_size == 0 || (source_size != 0 && copy_size > source_size))
    return -1;

  size_t target_offset = 0;
  if (elf_vaddr_to_image_offset(program, target_vaddr, copy_size,
        &target_offset) < 0)
    return -1;

  size_t source_offset = 0;
  if (elf_vaddr_to_image_offset(source_dep->program, source_vaddr, copy_size,
        &source_offset) < 0)
    return -1;

  memcpy(loaded_image + target_offset, source_dep->loaded_image + source_offset,
    copy_size);
  return 0;
}

static int resolve_process_tls_reloc_symbol(const struct poly_program *program,
    const uint8_t *loaded_image, uint64_t symtab_vaddr, uint64_t strtab_vaddr,
    uint64_t strsz, uint64_t syment, uint64_t hash_vaddr,
    uint64_t gnu_hash_vaddr, uint64_t symbol_index, uint64_t *tls_offset) {
  size_t symbol_count = 0;
  if (!symtab_vaddr || !strtab_vaddr || !strsz ||
      syment < sizeof(Elf64_Sym) ||
      dynamic_symbol_count_from_hash(program, loaded_image, symtab_vaddr,
        syment, hash_vaddr, gnu_hash_vaddr, &symbol_count) < 0 ||
      symbol_index >= symbol_count)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size,
        &symtab_offset) < 0)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;

  const Elf64_Sym *sym = (const Elf64_Sym *) (loaded_image + symtab_offset +
    symbol_index * syment);
  if (sym->st_name >= strsz)
    return -1;
  const char *symbol_name =
    (const char *) (loaded_image + strtab_offset + sym->st_name);
  const char *requested_version = NULL;
  if (relocation_requested_version_name(program, loaded_image, strtab_vaddr,
        strsz, symbol_index, &requested_version) < 0)
    return -1;

  if (sym->st_shndx == SHN_UNDEF) {
    if (resolve_root_scope_tls_symbol(program, symbol_name, requested_version,
          tls_offset) == 0)
      return 0;
    if (resolve_root_scope_dependency_tls_symbol(program, symbol_name,
          requested_version, tls_offset) == 0)
      return 0;
    if (resolve_loaded_dependency_tls_symbol(program, symbol_name,
          requested_version, tls_offset) == 0)
      return 0;
    if (ELF64_ST_BIND(sym->st_info) == STB_WEAK) {
      *tls_offset = 0;
      return 0;
    }
    return -1;
  }

  if (ELF64_ST_TYPE(sym->st_info) != STT_TLS)
    return -1;
  *tls_offset = (uint64_t) program->tls_offset + sym->st_value;
  return 0;
}

static long poly_x86_syscall6(long number, uint64_t arg0, uint64_t arg1,
    uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
  register long rax __asm__("rax") = number;
  register long rdi __asm__("rdi") = (long) arg0;
  register long rsi __asm__("rsi") = (long) arg1;
  register long rdx __asm__("rdx") = (long) arg2;
  register long r10 __asm__("r10") = (long) arg3;
  register long r8 __asm__("r8") = (long) arg4;
  register long r9 __asm__("r9") = (long) arg5;
  asm volatile("syscall"
      : "+r"(rax)
      : "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8), "r"(r9)
      : "rcx", "r11", "memory");
  return rax;
}

static long poly_x86_clone_monitor_child(uint64_t flags,
    uint64_t child_stack, uint64_t parent_tid, uint64_t child_tid,
    uint64_t tls) {
  register long rax __asm__("rax") = SYS_clone;
  register long rdi __asm__("rdi") = (long) flags;
  register long rsi __asm__("rsi") = (long) child_stack;
  register long rdx __asm__("rdx") = (long) parent_tid;
  register long r10 __asm__("r10") = (long) child_tid;
  register long r8 __asm__("r8") = (long) tls;
  asm volatile(
      "syscall\n"
      "testq %%rax, %%rax\n"
      "jnz 1f\n"
      "popq %%rdi\n"
      "call poly_clone_child_entry\n"
      "ud2\n"
      "1:\n"
      : "+r"(rax)
      : "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8)
      : "rcx", "r11", "memory");
  return rax;
}

__attribute__((noreturn))
static void poly_x86_exit_group_now(uint64_t code) {
  register long rax __asm__("rax") = SYS_exit_group;
  register long rdi __asm__("rdi") = (long) code;
  asm volatile("syscall"
      : "+a"(rax)
      : "D"(rdi)
      : "rcx", "r11", "memory");
  __builtin_unreachable();
}

__attribute__((noreturn))
static void poly_x86_exit_thread_now(uint64_t code) {
  register long rax __asm__("rax") = SYS_exit;
  register long rdi __asm__("rdi") = (long) code;
  asm volatile("syscall"
      : "+a"(rax)
      : "D"(rdi)
      : "rcx", "r11", "memory");
  __builtin_unreachable();
}

static void poly_store_linux_generic_stat(uint64_t destination,
    const struct stat *source) {
  struct poly_linux_generic_stat *target =
    (struct poly_linux_generic_stat *) (uintptr_t) destination;
  memset(target, 0, sizeof(*target));
  target->dev = (uint64_t) source->st_dev;
  target->ino = (uint64_t) source->st_ino;
  target->mode = (uint32_t) source->st_mode;
  target->nlink = (uint32_t) source->st_nlink;
  target->uid = (uint32_t) source->st_uid;
  target->gid = (uint32_t) source->st_gid;
  target->rdev = (uint64_t) source->st_rdev;
  target->size = (int64_t) source->st_size;
  target->blksize = (int32_t) source->st_blksize;
  target->blocks = (int64_t) source->st_blocks;
  target->atime_sec = (int64_t) source->st_atim.tv_sec;
  target->atime_nsec = (uint64_t) source->st_atim.tv_nsec;
  target->mtime_sec = (int64_t) source->st_mtim.tv_sec;
  target->mtime_nsec = (uint64_t) source->st_mtim.tv_nsec;
  target->ctime_sec = (int64_t) source->st_ctim.tv_sec;
  target->ctime_nsec = (uint64_t) source->st_ctim.tv_nsec;
}

static void poly_store_linux_generic_statfs(uint64_t destination,
    const struct statfs *source) {
  struct poly_linux_generic_statfs *target =
    (struct poly_linux_generic_statfs *) (uintptr_t) destination;
  memset(target, 0, sizeof(*target));
  target->type = (int64_t) source->f_type;
  target->bsize = (int64_t) source->f_bsize;
  target->blocks = (uint64_t) source->f_blocks;
  target->bfree = (uint64_t) source->f_bfree;
  target->bavail = (uint64_t) source->f_bavail;
  target->files = (uint64_t) source->f_files;
  target->ffree = (uint64_t) source->f_ffree;
  memcpy(target->fsid, &source->f_fsid, sizeof(target->fsid));
  target->namelen = (int64_t) source->f_namelen;
  target->frsize = (int64_t) source->f_frsize;
  target->flags = (int64_t) source->f_flags;
}

static void poly_store_fixed_string(char *target, size_t target_size,
    const char *value) {
  memset(target, 0, target_size);
  if (target_size == 0)
    return;
  size_t value_len = strlen(value);
  if (value_len >= target_size)
    value_len = target_size - 1;
  memcpy(target, value, value_len);
}

static void poly_load_x86_epoll_event(struct poly_x86_epoll_event *target,
    uint64_t source) {
  const struct poly_linux_generic_epoll_event *event =
    (const struct poly_linux_generic_epoll_event *) (uintptr_t) source;
  target->events = event->events;
  target->data = event->data;
}

static void poly_store_linux_generic_epoll_event(uint64_t destination,
    const struct poly_x86_epoll_event *source) {
  struct poly_linux_generic_epoll_event *event =
    (struct poly_linux_generic_epoll_event *) (uintptr_t) destination;
  event->events = source->events;
  event->pad = 0;
  event->data = source->data;
}

static uint64_t poly_dispatch_epoll_wait_events(long x86_number,
    uint64_t epoll_fd, uint64_t events, uint64_t maxevents, uint64_t timeout,
    uint64_t sigmask, uint64_t sigsetsize) {
  if (maxevents == 0)
    return (uint64_t) -EINVAL;
  if (events == 0)
    return (uint64_t) -EFAULT;
  if (maxevents > SIZE_MAX / sizeof(struct poly_x86_epoll_event))
    return (uint64_t) -EINVAL;

  size_t event_count = (size_t) maxevents;
  size_t event_bytes = event_count * sizeof(struct poly_x86_epoll_event);
  struct poly_x86_epoll_event *x86_events = malloc(event_bytes);
  if (x86_events == NULL)
    return (uint64_t) -ENOMEM;

  long status = poly_x86_syscall6(x86_number, epoll_fd,
    (uint64_t) (uintptr_t) x86_events, maxevents, timeout, sigmask,
    sigsetsize);
  if (status > 0) {
    for (long i = 0; i < status; i++) {
      poly_store_linux_generic_epoll_event(events +
        (uint64_t) i * sizeof(struct poly_linux_generic_epoll_event),
        &x86_events[i]);
    }
  }
  free(x86_events);
  return (uint64_t) status;
}

static uint64_t poly_riscv_hwprobe_value(int64_t key, int *known) {
  *known = 1;
  switch (key) {
    case POLY_RISCV_HWPROBE_KEY_MVENDORID:
    case POLY_RISCV_HWPROBE_KEY_MARCHID:
    case POLY_RISCV_HWPROBE_KEY_MIMPID:
      return 0;
    case POLY_RISCV_HWPROBE_KEY_BASE_BEHAVIOR:
      return POLY_RISCV_HWPROBE_BASE_BEHAVIOR_IMA;
    case POLY_RISCV_HWPROBE_KEY_IMA_EXT_0:
      return POLY_RISCV_HWPROBE_IMA_FD |
        POLY_RISCV_HWPROBE_IMA_C |
        POLY_RISCV_HWPROBE_EXT_ZBA |
        POLY_RISCV_HWPROBE_EXT_ZBB |
        POLY_RISCV_HWPROBE_EXT_ZBS |
        POLY_RISCV_HWPROBE_EXT_ZICOND |
        POLY_RISCV_HWPROBE_EXT_ZICNTR;
    case POLY_RISCV_HWPROBE_KEY_CPUPERF_0:
    case POLY_RISCV_HWPROBE_KEY_ZICBOZ_BLOCK_SIZE:
    case POLY_RISCV_HWPROBE_KEY_HIGHEST_VIRT_ADDRESS:
    case POLY_RISCV_HWPROBE_KEY_TIME_CSR_FREQ:
    case POLY_RISCV_HWPROBE_KEY_MISALIGNED_SCALAR_PERF:
    case POLY_RISCV_HWPROBE_KEY_MISALIGNED_VECTOR_PERF:
    case POLY_RISCV_HWPROBE_KEY_VENDOR_EXT_THEAD_0:
    case POLY_RISCV_HWPROBE_KEY_ZICBOM_BLOCK_SIZE:
    case POLY_RISCV_HWPROBE_KEY_VENDOR_EXT_SIFIVE_0:
      return 0;
    default:
      *known = 0;
      return 0;
  }
}

static uint64_t poly_dispatch_riscv_hwprobe(uint64_t pairs_address,
    uint64_t pair_count, uint64_t cpu_count, uint64_t cpus, uint64_t flags) {
  if (flags != 0 || cpu_count != 0 || cpus != 0)
    return (uint64_t) -EINVAL;
  if (pair_count != 0 && pairs_address == 0)
    return (uint64_t) -EFAULT;
  if (pair_count > 4096)
    return (uint64_t) -EINVAL;

  struct poly_riscv_hwprobe_pair *pairs =
    (struct poly_riscv_hwprobe_pair *) (uintptr_t) pairs_address;
  for (uint64_t n = 0; n < pair_count; n++) {
    int known = 0;
    uint64_t value = poly_riscv_hwprobe_value(pairs[n].key, &known);
    if (!known) {
      pairs[n].key = -1;
      pairs[n].value = 0;
      continue;
    }
    pairs[n].value = value;
  }
  return 0;
}

static void reset_process_brk_arena(void) {
  if (process_brk_mapping != NULL)
    munmap(process_brk_mapping, process_brk_mapping_size);
  process_brk_mapping = NULL;
  process_brk_mapping_size = 0;
  process_brk_current = 0;
}

static uint64_t poly_dispatch_process_brk(uint64_t requested_break) {
  const size_t arena_size = 1024 * 1024;
  if (process_brk_mapping == NULL) {
    process_brk_mapping = mmap(NULL, arena_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (process_brk_mapping == MAP_FAILED) {
      process_brk_mapping = NULL;
      return 0;
    }
    process_brk_mapping_size = arena_size;
    memset(process_brk_mapping, 0, process_brk_mapping_size);
    process_brk_current = (uint64_t) (uintptr_t) process_brk_mapping;
  }

  const uint64_t base = (uint64_t) (uintptr_t) process_brk_mapping;
  const uint64_t limit = base + process_brk_mapping_size;
  if (requested_break == 0)
    return process_brk_current;
  if (requested_break < base || requested_break > limit)
    return process_brk_current;
  process_brk_current = requested_break;
  return process_brk_current;
}

static uint64_t poly_dispatch_private_exec_file_mmap(uint64_t address,
    uint64_t length_arg, uint64_t prot, uint64_t flags, uint64_t fd_arg,
    uint64_t offset_arg) {
  if (length_arg == 0 || length_arg > SIZE_MAX || offset_arg > INT64_MAX)
    return (uint64_t) -EINVAL;

  const int fd = (int) fd_arg;
  struct stat st;
  if (fstat(fd, &st) != 0)
    return (uint64_t) -errno;
  if (st.st_size < 0)
    return (uint64_t) -EINVAL;

  int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
  if ((flags & MAP_FIXED) != 0)
    mmap_flags |= MAP_FIXED;
  if ((flags & MAP_FIXED_NOREPLACE) != 0)
    mmap_flags |= MAP_FIXED_NOREPLACE;

  const size_t length = (size_t) length_arg;
  const int load_prot = PROT_READ | PROT_WRITE | (prot & PROT_EXEC);
  void *mapping = mmap((void *) (uintptr_t) address, length, load_prot,
    mmap_flags, -1, 0);
  if (mapping == MAP_FAILED)
    return (uint64_t) -errno;

  uint64_t available = 0;
  const uint64_t file_size = (uint64_t) st.st_size;
  if (offset_arg < file_size) {
    available = file_size - offset_arg;
    if (available > length_arg)
      available = length_arg;
  }

  uint8_t *dest = (uint8_t *) mapping;
  size_t copied = 0;
  while (copied < available) {
    const size_t remaining = (size_t) (available - copied);
    ssize_t got = pread(fd, dest + copied, remaining,
      (off_t) (offset_arg + copied));
    if (got < 0) {
      if (errno == EINTR)
        continue;
      const int saved_errno = errno;
      munmap(mapping, length);
      return (uint64_t) -saved_errno;
    }
    if (got == 0)
      break;
    copied += (size_t) got;
  }

  if (mprotect(mapping, length, (int) prot) != 0) {
    const int saved_errno = errno;
    munmap(mapping, length);
    return (uint64_t) -saved_errno;
  }

  return (uint64_t) (uintptr_t) mapping;
}

static void poly_sanitize_guest_sigaction(struct poly_linux_ksigaction *action) {
  if (action->handler != (uint64_t) (uintptr_t) SIG_DFL &&
      action->handler != (uint64_t) (uintptr_t) SIG_IGN) {
    action->handler = (uint64_t) (uintptr_t) SIG_DFL;
    action->flags = 0;
  }
  action->restorer = 0;
}

static int poly_guest_write_ksigaction(uint64_t address,
    const struct poly_linux_ksigaction *value);
static int poly_guest_read_ksigaction(uint64_t address,
    struct poly_linux_ksigaction *out);

static void poly_store_default_guest_sigaction(uint64_t oldact) {
  if (oldact == 0)
    return;
  struct poly_linux_ksigaction guest_oldact;
  memset(&guest_oldact, 0, sizeof(guest_oldact));
  guest_oldact.handler = (uint64_t) (uintptr_t) SIG_DFL;
  if (!poly_guest_write_ksigaction(oldact, &guest_oldact)) {
    poly_write_literal_stderr("POLYEXEC_GUEST_SIGACTION_BADPTR: oldact=0x");
    poly_write_hex64_stderr(oldact);
    poly_write_literal_stderr("\n");
  }
}

static void poly_store_guest_signal_action(uint64_t signum, uint64_t oldact) {
  if (oldact == 0)
    return;
  struct poly_linux_ksigaction guest_oldact;
  if (signum < sizeof(poly_guest_signal_action_shadow_valid) &&
      poly_guest_signal_action_shadow_valid[signum]) {
    guest_oldact = poly_guest_signal_action_shadow[signum];
  } else {
    memset(&guest_oldact, 0, sizeof(guest_oldact));
    guest_oldact.handler = (uint64_t) (uintptr_t) SIG_DFL;
  }
  if (!poly_guest_write_ksigaction(oldact, &guest_oldact)) {
    poly_write_literal_stderr("POLYEXEC_GUEST_SIGACTION_BADPTR: oldact=0x");
    poly_write_hex64_stderr(oldact);
    poly_write_literal_stderr("\n");
  }
}

static uint64_t poly_sigset_clear_signal(uint64_t mask, int signum) {
  if (signum <= 0 || signum > 64)
    return mask;
  return mask & ~(1ULL << (uint64_t) (signum - 1));
}

static uint64_t poly_sigset_add_signal(uint64_t mask, int signum) {
  if (signum <= 0 || signum > 64)
    return mask;
  return mask | (1ULL << (uint64_t) (signum - 1));
}

static int poly_protect_runtime_signals_enabled(void);
static int poly_virtual_sigchld_enabled(void);

static uint64_t poly_host_visible_virtual_signal_mask(uint64_t mask) {
  if (polyexec_watchdog_signal > 0)
    mask = poly_sigset_clear_signal(mask, polyexec_watchdog_signal);
  if (poly_protect_runtime_signals_enabled()) {
    mask = poly_sigset_clear_signal(mask, SIGALRM);
    mask = poly_sigset_clear_signal(mask, SIGUSR1);
    mask = poly_sigset_clear_signal(mask, SIGUSR2);
  }
  if (poly_virtual_sigchld_enabled())
    mask = poly_sigset_clear_signal(mask, SIGCHLD);
  return mask;
}

static uint64_t poly_sanitize_host_runtime_sigset(uint64_t mask) {
  mask = poly_sigset_clear_signal(mask, SIGSEGV);
  mask = poly_sigset_clear_signal(mask, SIGBUS);
  mask = poly_sigset_clear_signal(mask, SIGILL);
  return poly_host_visible_virtual_signal_mask(mask);
}

static uint64_t poly_guest_initial_sigmask_from_host(uint64_t host_mask) {
  return poly_host_visible_virtual_signal_mask(host_mask);
}

static uint64_t poly_update_guest_sigmask(uint64_t current, uint64_t how,
    uint64_t set) {
  switch (how) {
    case SIG_BLOCK:
      return current | set;
    case SIG_UNBLOCK:
      return current & ~set;
    case SIG_SETMASK:
      return set;
    default:
      return current;
  }
}

static int poly_protect_runtime_signals_enabled(void) {
  if (polyexec_protect_runtime_signals < 0) {
    const char *value = getenv("POLYEXEC_PROTECT_RUNTIME_SIGNALS");
    polyexec_protect_runtime_signals = value != NULL && value[0] != '\0' &&
      strcmp(value, "0") != 0;
  }
  return polyexec_protect_runtime_signals;
}

static int poly_trace_protected_signal_waits_enabled(void) {
  if (polyexec_trace_protected_signal_waits < 0) {
    const char *value = getenv("POLYEXEC_TRACE_PROTECTED_SIGNAL_WAITS");
    polyexec_trace_protected_signal_waits =
      value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
  }
  return polyexec_trace_protected_signal_waits;
}

static int poly_virtual_sigchld_enabled(void) {
  if (polyexec_virtual_sigchld < 0) {
    const char *value = getenv("POLYEXEC_VIRTUAL_SIGCHLD");
    polyexec_virtual_sigchld =
      value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
  }
  return polyexec_virtual_sigchld;
}

static int poly_is_protected_runtime_signal(uint64_t signum) {
  if (!poly_protect_runtime_signals_enabled())
    return 0;
  return signum == SIGALRM || signum == SIGUSR1 || signum == SIGUSR2;
}

static void poly_protected_runtime_signal_noop(int signum) {
  (void) signum;
}

static void poly_virtual_signal_host_handler(int signum) {
  if (signum > 0 && signum < 8 * (int) sizeof(poly_pending_virtual_signal_mask))
    poly_pending_virtual_signal_mask |=
      (sig_atomic_t) (1U << (unsigned) (signum - 1));
}

static int poly_set_host_signal_handler(uint64_t signum,
    void (*handler)(int)) {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  if (sigaction((int) signum, &action, NULL) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: host signal handler install failed signum=%llu: %s\n",
      (unsigned long long) signum, strerror(errno));
    return -1;
  }
  return 0;
}

static int poly_set_host_virtual_signal_handler(uint64_t signum,
    const struct poly_linux_ksigaction *guest_act) {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = poly_virtual_signal_host_handler;
  sigemptyset(&action.sa_mask);
#ifdef SA_NOCLDSTOP
  if (signum == SIGCHLD && guest_act != NULL &&
      (guest_act->flags & SA_NOCLDSTOP) != 0)
    action.sa_flags |= SA_NOCLDSTOP;
#endif
#ifdef SA_NOCLDWAIT
  if (signum == SIGCHLD && guest_act != NULL &&
      (guest_act->flags & SA_NOCLDWAIT) != 0)
    action.sa_flags |= SA_NOCLDWAIT;
#endif
  if (sigaction((int) signum, &action, NULL) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: host virtual signal handler install failed signum=%llu: %s\n",
      (unsigned long long) signum, strerror(errno));
    return -1;
  }
  return 0;
}

static int poly_install_protected_runtime_signal_handlers(void) {
  if (!poly_protect_runtime_signals_enabled())
    return 0;
  if (poly_set_host_signal_handler(SIGALRM,
        poly_virtual_signal_host_handler) < 0 ||
      poly_set_host_signal_handler(SIGUSR1,
        poly_virtual_signal_host_handler) < 0 ||
      poly_set_host_signal_handler(SIGUSR2,
        poly_virtual_signal_host_handler) < 0)
    return -1;
  return 0;
}

static uint64_t poly_dispatch_virtual_sigchld_action(uint64_t act,
    uint64_t oldact) {
  struct poly_linux_ksigaction guest_act;
  int have_guest_act = 0;
  if (act != 0) {
    have_guest_act = poly_guest_read_ksigaction(act, &guest_act);
    if (!have_guest_act) {
      poly_write_literal_stderr("POLYEXEC_GUEST_SIGACTION_BADPTR: act=0x");
      poly_write_hex64_stderr(act);
      poly_write_literal_stderr("\n");
      return (uint64_t) -EFAULT;
    }
  }

  if (poly_trace_protected_signal_waits_enabled()) {
    poly_write_literal_stderr("POLYEXEC_GUEST_SIGACTION_SIGCHLD: act=0x");
    poly_write_hex64_stderr(act);
    poly_write_literal_stderr(" handler=0x");
    poly_write_hex64_stderr(have_guest_act ? guest_act.handler : 0);
    poly_write_literal_stderr(" flags=0x");
    poly_write_hex64_stderr(have_guest_act ? guest_act.flags : 0);
    poly_write_literal_stderr(" oldact=0x");
    poly_write_hex64_stderr(oldact);
    poly_write_literal_stderr("\n");
  }

  poly_store_guest_signal_action(SIGCHLD, oldact);
  if (have_guest_act) {
    if (guest_act.handler == (uint64_t) (uintptr_t) SIG_DFL ||
        guest_act.handler == (uint64_t) (uintptr_t) SIG_IGN) {
      void (*host_handler)(int) =
        guest_act.handler == (uint64_t) (uintptr_t) SIG_IGN ? SIG_IGN :
          SIG_DFL;
      if (poly_set_host_signal_handler(SIGCHLD, host_handler) < 0)
        return (uint64_t) -errno;
    } else if (poly_set_host_virtual_signal_handler(SIGCHLD, &guest_act) < 0) {
      return (uint64_t) -errno;
    }
    poly_guest_signal_action_shadow[SIGCHLD] = guest_act;
    poly_guest_signal_action_shadow_valid[SIGCHLD] = 1;
  }
  return 0;
}

static uint64_t polyexec_strtoull_c(const char *nptr, char **endptr, int base);

static int poly_guest_range_is_mapped(uint64_t address, size_t length,
    int writable) {
  if (length == 0)
    return 1;
  if (address == 0 || address > UINT64_MAX - (uint64_t) length)
    return 0;

  int fd = (int) poly_x86_syscall6(SYS_openat, AT_FDCWD,
    (uint64_t) (uintptr_t) "/proc/self/maps", O_RDONLY | O_CLOEXEC,
    0, 0, 0);
  if (fd < 0)
    return 0;

  const uint64_t end_address = address + (uint64_t) length;
  char buffer[8192];
  char line[512];
  size_t line_len = 0;
  int mapped = 0;
  for (;;) {
    long count = poly_x86_syscall6(SYS_read, fd,
      (uint64_t) (uintptr_t) buffer, sizeof(buffer), 0, 0, 0);
    if (count <= 0)
      break;
    for (long n = 0; n < count; n++) {
      if (buffer[n] == '\n') {
        line[line_len] = '\0';
        char *parse_end = NULL;
        const uint64_t start = polyexec_strtoull_c(line, &parse_end, 16);
        if (parse_end != line && *parse_end == '-') {
          const char *stop_text = parse_end + 1;
          const uint64_t stop = polyexec_strtoull_c(stop_text, &parse_end, 16);
          if (parse_end != stop_text) {
            while (*parse_end == ' ')
              parse_end++;
            if (parse_end[0] == 'r' &&
                (!writable || parse_end[1] == 'w') &&
                start <= address && end_address <= stop) {
              mapped = 1;
              break;
            }
          }
        }
        line_len = 0;
      }
      else if (line_len + 1 < sizeof(line)) {
        line[line_len++] = buffer[n];
      }
    }
    if (mapped)
      break;
  }
  (void) poly_x86_syscall6(SYS_close, fd, 0, 0, 0, 0, 0);
  return mapped;
}

static int poly_guest_read_u64(uint64_t address, uint64_t *out) {
  if (!poly_guest_range_is_mapped(address, sizeof(*out), 0))
    return 0;
  *out = *(const uint64_t *) (uintptr_t) address;
  return 1;
}

static int poly_guest_read_ksigaction(uint64_t address,
    struct poly_linux_ksigaction *out) {
  if (!poly_guest_range_is_mapped(address, sizeof(*out), 0))
    return 0;
  *out = *(const struct poly_linux_ksigaction *) (uintptr_t) address;
  return 1;
}

static int poly_guest_write_u64(uint64_t address, uint64_t value) {
  if (!poly_guest_range_is_mapped(address, sizeof(value), 1))
    return 0;
  *(uint64_t *) (uintptr_t) address = value;
  return 1;
}

static int poly_guest_write_ksigaction(uint64_t address,
    const struct poly_linux_ksigaction *value) {
  if (!poly_guest_range_is_mapped(address, sizeof(*value), 1))
    return 0;
  *(struct poly_linux_ksigaction *) (uintptr_t) address = *value;
  return 1;
}

static int poly_guest_signal_action_deliverable(uint64_t signum,
    uint64_t *handler_out, uint64_t *restorer_out, uint64_t *flags_out,
    uint64_t *mask_out) {
  if (signum >= sizeof(poly_guest_signal_action_shadow_valid) ||
      !poly_guest_signal_action_shadow_valid[signum])
    return 0;
  const struct poly_linux_ksigaction *action =
    &poly_guest_signal_action_shadow[signum];
  const uint64_t handler = action->handler;
  if (handler == (uint64_t) (uintptr_t) SIG_DFL ||
      handler == (uint64_t) (uintptr_t) SIG_IGN ||
      handler == 0)
    return 0;
  uint64_t restorer = action->restorer;
  if (restorer == 0)
    restorer = poly_aarch64_vdso_rt_sigreturn;
  if (restorer == 0) {
    poly_write_literal_stderr(
      "POLYEXEC_GUEST_SIGNAL_NO_RESTORER: signum=0x");
    poly_write_hex64_stderr(signum);
    poly_write_literal_stderr(" handler=0x");
    poly_write_hex64_stderr(handler);
    poly_write_literal_stderr("\n");
    return 0;
  }
  *handler_out = handler;
  *restorer_out = restorer;
  if (flags_out != NULL)
    *flags_out = action->flags;
  if (mask_out != NULL)
    *mask_out = action->mask;
  return 1;
}

static int poly_guest_signal_mask_blocks(uint64_t signum) {
  if (!poly_guest_sigmask_shadow_valid)
    return 0;
  return (poly_guest_sigmask_shadow &
    (1ULL << (uint64_t) (signum - 1))) != 0;
}

static int64_t poly_dispatch_guest_sigaltstack(uint64_t mode,
    uint64_t new_stack_addr, uint64_t old_stack_addr) {
  uint64_t *guest_sp = mode == POLY_MODE_RAW_RISCV ?
    &poly_riscv_guest_sigaltstack_sp : &poly_aarch64_guest_sigaltstack_sp;
  uint64_t *guest_size = mode == POLY_MODE_RAW_RISCV ?
    &poly_riscv_guest_sigaltstack_size : &poly_aarch64_guest_sigaltstack_size;
  int32_t *guest_flags = mode == POLY_MODE_RAW_RISCV ?
    &poly_riscv_guest_sigaltstack_flags : &poly_aarch64_guest_sigaltstack_flags;
  const int on_signal_stack = mode == POLY_MODE_RAW_AARCH64 &&
    poly_aarch64_active_signal_frame != 0;

  if (old_stack_addr != 0) {
    if (!poly_guest_range_is_mapped(old_stack_addr,
          sizeof(struct poly_linux_stack_t), 1))
      return -EFAULT;
    struct poly_linux_stack_t old_stack;
    memset(&old_stack, 0, sizeof(old_stack));
    old_stack.sp = *guest_sp;
    old_stack.size = *guest_size;
    old_stack.flags = *guest_flags;
    if (on_signal_stack &&
        (old_stack.flags & POLY_LINUX_SS_DISABLE) == 0)
      old_stack.flags |= POLY_LINUX_SS_ONSTACK;
    memcpy((void *) (uintptr_t) old_stack_addr, &old_stack,
      sizeof(old_stack));
  }

  if (new_stack_addr == 0)
    return 0;
  if (on_signal_stack)
    return -EPERM;
  if (!poly_guest_range_is_mapped(new_stack_addr,
        sizeof(struct poly_linux_stack_t), 0))
    return -EFAULT;

  struct poly_linux_stack_t new_stack;
  memcpy(&new_stack, (const void *) (uintptr_t) new_stack_addr,
    sizeof(new_stack));
  const int32_t supported_flags = POLY_LINUX_SS_DISABLE;
  if ((new_stack.flags & ~supported_flags) != 0)
    return -EINVAL;
  if ((new_stack.flags & POLY_LINUX_SS_DISABLE) != 0) {
    *guest_sp = 0;
    *guest_size = 0;
    *guest_flags = POLY_LINUX_SS_DISABLE;
    return 0;
  }
  if (new_stack.size < POLY_AARCH64_MINSIGSTKSZ)
    return -ENOMEM;
  if (!poly_guest_range_is_mapped(new_stack.sp, (size_t) new_stack.size, 1)) {
    if (poly_trace_protected_signal_waits_enabled()) {
      poly_write_literal_stderr("POLYEXEC_GUEST_SIGALTSTACK_BADRANGE: sp=0x");
      poly_write_hex64_stderr(new_stack.sp);
      poly_write_literal_stderr(" size=0x");
      poly_write_hex64_stderr(new_stack.size);
      poly_write_literal_stderr("\n");
    }
    return -EFAULT;
  }
  *guest_sp = new_stack.sp;
  *guest_size = new_stack.size;
  *guest_flags = 0;
  if (poly_trace_protected_signal_waits_enabled()) {
    poly_write_literal_stderr("POLYEXEC_GUEST_SIGALTSTACK: sp=0x");
    poly_write_hex64_stderr(new_stack.sp);
    poly_write_literal_stderr(" size=0x");
    poly_write_hex64_stderr(new_stack.size);
    poly_write_literal_stderr("\n");
  }
  return 0;
}

static int poly_try_deliver_aarch64_signal(struct poly_xsave_state *state,
    uint64_t forced_signum, uint64_t fault_address, uint64_t fault_code) {
  if (state == NULL || poly_aarch64_active_signal_frame != 0)
    return 0;

  uint64_t signum = 0;
  uint64_t handler = 0;
  uint64_t restorer = 0;
  uint64_t action_flags = 0;
  uint64_t action_mask = 0;
  if (forced_signum != 0) {
    if (state->header.current_mode != POLY_MODE_RAW_AARCH64 ||
        !poly_guest_signal_action_deliverable(forced_signum, &handler,
          &restorer, &action_flags, &action_mask))
      return 0;
    signum = forced_signum;
  } else {
    if (poly_pending_virtual_signal_mask == 0 ||
        state->trap.source_mode != POLY_MODE_RAW_AARCH64)
      return 0;
    for (uint64_t candidate = 1; candidate <= 31; candidate++) {
      const sig_atomic_t bit =
        (sig_atomic_t) (1U << (unsigned) (candidate - 1));
      if ((poly_pending_virtual_signal_mask & bit) == 0)
        continue;
      if (poly_guest_signal_mask_blocks(candidate))
        continue;
      if (poly_guest_signal_action_deliverable(candidate, &handler, &restorer,
            &action_flags, &action_mask)) {
        signum = candidate;
        break;
      }
      poly_pending_virtual_signal_mask &= ~bit;
    }
  }
  if (signum == 0)
    return 0;

  const size_t call_area_size = 32U;
  const size_t frame_size =
    (sizeof(struct poly_aarch64_virtual_signal_frame) + 15U) & ~(size_t) 15U;
  const size_t total_size = call_area_size + frame_size;
  const size_t stack_size = POLY_AARCH64_VSIG_STACK_SIZE;
  uint64_t saved_gpr[32];
  struct poly_u128 saved_fp[32];
  uint64_t saved_nzcv = state->aarch64_status.nzcv;
  uint64_t saved_fpcr = state->aarch64_status.fpcr;
  uint64_t saved_fpsr = state->aarch64_status.fpsr;
  memcpy(saved_gpr, state->aarch64_gpr, sizeof(saved_gpr));
  memcpy(saved_fp, state->aarch64_fp, sizeof(saved_fp));
  if ((state->trap_restore.flags &
        (POLY_TRAP_RESTORE_FLAG_VALID |
         POLY_TRAP_RESTORE_FLAG_AARCH64_STATE_VALID)) ==
        (POLY_TRAP_RESTORE_FLAG_VALID |
         POLY_TRAP_RESTORE_FLAG_AARCH64_STATE_VALID) &&
      state->trap_restore.mode == POLY_MODE_RAW_AARCH64 &&
      poly_aarch64_trap_restore_gpr_mask_usable(
        state->trap_restore.aarch64_gpr_valid_mask)) {
    memcpy(saved_gpr, state->trap_restore.aarch64_gpr, sizeof(saved_gpr));
    memcpy(saved_fp, state->trap_restore.aarch64_fp, sizeof(saved_fp));
    saved_nzcv = state->trap_restore.aarch64_nzcv;
    saved_fpcr = state->trap_restore.aarch64_fpcr;
    saved_fpsr = state->trap_restore.aarch64_fpsr;
  }
  const uint64_t sp = saved_gpr[31];
  uint64_t interrupted_pc = state->header.foreign_pc;
  if (forced_signum == 0 &&
      state->trap.source_mode == POLY_MODE_RAW_AARCH64 &&
      state->trap.resume_pc != 0)
    interrupted_pc = state->trap.resume_pc;
  if (interrupted_pc == 0 && state->trap.resume_pc != 0)
    interrupted_pc = state->trap.resume_pc;
  if (interrupted_pc == 0 && state->trap.trap_pc != 0)
    interrupted_pc = state->trap.trap_pc;
  if (interrupted_pc == 0 && state->aarch64_gpr[30] != 0 &&
      poly_guest_range_is_mapped(state->aarch64_gpr[30], 4, 0))
    interrupted_pc = state->aarch64_gpr[30];
  if (total_size >= stack_size)
    return 0;
  void *signal_stack = NULL;
  uint64_t stack_top = 0;
  const int use_altstack =
    (action_flags & POLY_LINUX_SA_ONSTACK) != 0 &&
    (poly_aarch64_guest_sigaltstack_flags & POLY_LINUX_SS_DISABLE) == 0 &&
    poly_aarch64_guest_sigaltstack_size >= total_size &&
    poly_guest_range_is_mapped(poly_aarch64_guest_sigaltstack_sp,
      (size_t) poly_aarch64_guest_sigaltstack_size, 1);
  if (use_altstack) {
    stack_top = poly_aarch64_guest_sigaltstack_sp +
      poly_aarch64_guest_sigaltstack_size;
  } else {
    signal_stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (signal_stack == MAP_FAILED) {
      poly_write_literal_stderr(
        "POLYEXEC_GUEST_SIGNAL_STACK_MMAP_FAIL: signum=0x");
      poly_write_hex64_stderr(signum);
      poly_write_literal_stderr(" errno=0x");
      poly_write_hex64_stderr((uint64_t) errno);
      poly_write_literal_stderr("\n");
      return 0;
    }
    stack_top = (uint64_t) (uintptr_t) signal_stack + (uint64_t) stack_size;
  }
  const uint64_t signal_sp = (stack_top - (uint64_t) total_size) & ~0xfULL;
  const uint64_t frame_sp = signal_sp + (uint64_t) call_area_size;
  if (!poly_guest_range_is_mapped(signal_sp, total_size, 1)) {
    poly_write_literal_stderr(
      "POLYEXEC_GUEST_SIGNAL_FRAME_BADSP: signum=0x");
    poly_write_hex64_stderr(signum);
    poly_write_literal_stderr(" sp=0x");
    poly_write_hex64_stderr(sp);
    poly_write_literal_stderr(" frame=0x");
    poly_write_hex64_stderr(frame_sp);
    poly_write_literal_stderr("\n");
    if (signal_stack != NULL)
      munmap(signal_stack, stack_size);
    return 0;
  }

  struct poly_aarch64_virtual_signal_frame frame;
  memset(&frame, 0, sizeof(frame));
  frame.magic = POLY_AARCH64_VSIGFRAME_MAGIC;
  frame.size = sizeof(frame);
  frame.signum = signum;
  frame.handler_args[1] = signum;
  frame.handler_args[2] = frame_sp +
    offsetof(struct poly_aarch64_virtual_signal_frame, siginfo);
  frame.handler_args[3] = frame_sp +
    offsetof(struct poly_aarch64_virtual_signal_frame, ucontext);
  frame.siginfo.si_signo = (int32_t) signum;
  frame.siginfo.si_code = (int32_t) fault_code;
  frame.siginfo.fault_addr = fault_address;
  frame.ucontext.mcontext.fault_address = fault_address;
  memcpy(frame.ucontext.mcontext.regs, saved_gpr,
    sizeof(frame.ucontext.mcontext.regs));
  frame.ucontext.mcontext.sp = saved_gpr[31];
  frame.ucontext.mcontext.pc = interrupted_pc;
  frame.ucontext.mcontext.pstate = saved_nzcv;
  frame.ucontext.stack_sp = poly_aarch64_guest_sigaltstack_sp;
  frame.ucontext.stack_flags = poly_aarch64_guest_sigaltstack_flags;
  if (use_altstack)
    frame.ucontext.stack_flags |= POLY_LINUX_SS_ONSTACK;
  frame.ucontext.stack_size = poly_aarch64_guest_sigaltstack_size;
  frame.saved_trap = state->trap;
  memcpy(frame.saved_trap_args, state->trap_args, sizeof(frame.saved_trap_args));
  frame.saved_trap_restore = state->trap_restore;
  if (frame.saved_trap.reason == 0)
    memset(frame.saved_trap_args, 0, sizeof(frame.saved_trap_args));
  if (frame.saved_trap_restore.x86_gpr[15] == 0)
    frame.saved_trap_restore.x86_gpr[15] = poly_current_x86_rsp();
  frame.saved_native_return = state->native_return;
  poly_clear_native_return_state(state);
  memcpy(frame.aarch64_gpr, saved_gpr, sizeof(frame.aarch64_gpr));
  memcpy(frame.aarch64_fp, saved_fp, sizeof(frame.aarch64_fp));
  frame.aarch64_nzcv = saved_nzcv;
  frame.aarch64_fpcr = saved_fpcr;
  frame.aarch64_fpsr = saved_fpsr;
  frame.guest_sigmask = poly_guest_sigmask_shadow_valid ?
    poly_guest_sigmask_shadow : 0;
  frame.interrupted_pc = interrupted_pc;
  uint64_t *handler_args = (uint64_t *) (uintptr_t) signal_sp;
  handler_args[1] = signum;
  handler_args[2] = frame.handler_args[2];
  handler_args[3] = frame.handler_args[3];
  memcpy((void *) (uintptr_t) frame_sp, &frame, sizeof(frame));

  if (forced_signum == 0)
    poly_pending_virtual_signal_mask &=
      ~((sig_atomic_t) (1U << (unsigned) (signum - 1)));
  uint64_t handler_sigmask = frame.guest_sigmask | action_mask;
  if ((action_flags & SA_NODEFER) == 0)
    handler_sigmask = poly_sigset_add_signal(handler_sigmask, (int) signum);
  poly_guest_sigmask_shadow = handler_sigmask;
  poly_guest_sigmask_shadow_valid = 1;
  poly_aarch64_active_signal_frame = frame_sp;
  state->header.current_mode = POLY_MODE_RAW_AARCH64;
  state->header.foreign_pc = handler;
  memset(&state->trap, 0, sizeof(state->trap));
  state->trap.source_mode = POLY_MODE_RAW_AARCH64;
  state->trap.reason = POLY_TRAP_BREAK;
  state->trap.trap_pc = interrupted_pc != 0 ? interrupted_pc : handler;
  state->trap.resume_pc = handler;
  state->trap.flags = POLY_TRAP_PACKET_REQUIRED_FLAGS |
    POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE;
  state->aarch64_gpr[0] = signum;
  state->aarch64_gpr[1] = frame.handler_args[2];
  state->aarch64_gpr[2] = frame.handler_args[3];
  state->aarch64_gpr[30] = restorer;
  state->aarch64_gpr[31] = signal_sp;
  memcpy(state->trap_restore.aarch64_gpr, frame.aarch64_gpr,
    sizeof(state->trap_restore.aarch64_gpr));
  memcpy(state->trap_restore.aarch64_fp, frame.aarch64_fp,
    sizeof(state->trap_restore.aarch64_fp));
  state->trap_restore.aarch64_nzcv = frame.aarch64_nzcv;
  state->trap_restore.aarch64_fpcr = frame.aarch64_fpcr;
  state->trap_restore.aarch64_fpsr = frame.aarch64_fpsr;
  state->trap_restore.aarch64_gpr[0] = signum;
  state->trap_restore.aarch64_gpr[1] = frame.handler_args[2];
  state->trap_restore.aarch64_gpr[2] = frame.handler_args[3];
  state->trap_restore.aarch64_gpr[30] = restorer;
  state->trap_restore.aarch64_gpr[31] = signal_sp;
  state->trap_restore.x86_gpr[15] = frame.saved_trap_restore.x86_gpr[15];
  state->trap_restore.flags = POLY_TRAP_RESTORE_FLAG_VALID |
    POLY_TRAP_RESTORE_FLAG_AARCH64_STATE_VALID;
  state->trap_restore.mode = POLY_MODE_RAW_AARCH64;
  state->trap_restore.aarch64_gpr_valid_mask = POLY_AARCH64_GPR_VALID_MASK;
  poly_sanitize_aarch64_trap_restore_payload(&state->trap_restore);
  /*
   * Keep the monitor on its native stack while the foreign frontend sees the
   * synthetic AArch64 signal stack through x31/sp.  Trap/syscall handling from
   * inside a guest handler runs C code and must not borrow guest stack memory.
   */

  if (poly_trace_protected_signal_waits_enabled()) {
    poly_write_literal_stderr(
      "POLYEXEC_GUEST_SIGNAL_DELIVER: signum=0x");
    poly_write_hex64_stderr(signum);
    poly_write_literal_stderr(" handler=0x");
    poly_write_hex64_stderr(handler);
    poly_write_literal_stderr(" restorer=0x");
    poly_write_hex64_stderr(restorer);
    poly_write_literal_stderr(" frame=0x");
    poly_write_hex64_stderr(frame_sp);
    poly_write_literal_stderr(" entry_sp=0x");
    poly_write_hex64_stderr(signal_sp);
    poly_write_literal_stderr(" altstack=0x");
    poly_write_hex64_stderr((uint64_t) use_altstack);
    poly_write_literal_stderr(" saved_pc=0x");
    poly_write_hex64_stderr(frame.saved_trap.trap_pc);
    poly_write_literal_stderr(" saved_resume=0x");
    poly_write_hex64_stderr(frame.saved_trap.resume_pc);
    poly_write_literal_stderr(" saved_rsp=0x");
    poly_write_hex64_stderr(frame.saved_trap_restore.x86_gpr[15]);
    poly_write_literal_stderr(" saved_x0=0x");
    poly_write_hex64_stderr(frame.aarch64_gpr[0]);
    poly_write_literal_stderr(" fault_addr=0x");
    poly_write_hex64_stderr(fault_address);
    poly_write_literal_stderr(" fault_mapped=0x");
    poly_write_hex64_stderr((uint64_t)
      (fault_address != 0 && poly_guest_range_is_mapped(fault_address, 1, 0)));
    poly_write_literal_stderr(" fault_writable=0x");
    poly_write_hex64_stderr((uint64_t)
      (fault_address != 0 && poly_guest_range_is_mapped(fault_address, 1, 1)));
    poly_write_literal_stderr(" fault_writable16=0x");
    poly_write_hex64_stderr((uint64_t)
      (fault_address != 0 && poly_guest_range_is_mapped(fault_address, 16, 1)));
    poly_write_literal_stderr(" saved_x30=0x");
    poly_write_hex64_stderr(frame.aarch64_gpr[30]);
    poly_write_literal_stderr(" saved_x28=0x");
    poly_write_hex64_stderr(frame.aarch64_gpr[28]);
    poly_write_literal_stderr(" mcontext=0x");
    poly_write_hex64_stderr(frame.handler_args[3] +
      offsetof(struct poly_aarch64_ucontext, mcontext));
    poly_write_literal_stderr(" mc_pc=0x");
    poly_write_hex64_stderr(frame.ucontext.mcontext.pc);
    poly_write_literal_stderr(" mc_sp=0x");
    poly_write_hex64_stderr(frame.ucontext.mcontext.sp);
    poly_write_literal_stderr("\n");
  }
  if ((signum == SIGSEGV || signum == SIGBUS || signum == SIGILL) &&
      poly_dump_maps_on_fault_enabled())
    poly_dump_proc_maps_stderr();
  return 1;
}

static int poly_handle_aarch64_rt_sigreturn(struct poly_xsave_state *state,
    int has_trap_state, uint64_t *result_out) {
  if (result_out == NULL)
    return 0;
  if (!has_trap_state || state == NULL) {
    *result_out = (uint64_t) -EINVAL;
    return 1;
  }

  uint64_t frame_addr = poly_aarch64_active_signal_frame;
  if (frame_addr == 0)
    frame_addr = state->aarch64_gpr[31];
  if (!poly_guest_range_is_mapped(frame_addr,
        sizeof(struct poly_aarch64_virtual_signal_frame), 0)) {
    *result_out = (uint64_t) -EFAULT;
    return 1;
  }

  struct poly_aarch64_virtual_signal_frame frame;
  memcpy(&frame, (const void *) (uintptr_t) frame_addr, sizeof(frame));
  if (frame.magic != POLY_AARCH64_VSIGFRAME_MAGIC ||
      frame.size != sizeof(frame) || frame.signum == 0 ||
      frame.signum >= sizeof(poly_guest_signal_action_shadow_valid)) {
    *result_out = (uint64_t) -EINVAL;
    return 1;
  }

  const uint64_t native_return_sp = state->aarch64_gpr[31];
  state->trap = frame.saved_trap;
  memcpy(state->trap_args, frame.saved_trap_args, sizeof(state->trap_args));
  state->native_return = frame.saved_native_return;
  if (frame.signum == SIGCHLD)
    poly_clear_native_return_state(state);
  memcpy(state->aarch64_gpr, frame.ucontext.mcontext.regs,
    sizeof(state->aarch64_gpr));
	  const uint64_t restored_sp = frame.ucontext.mcontext.sp;
	  const uint64_t monitor_sp = native_return_sp != 0 ?
	    native_return_sp : frame.saved_trap_restore.x86_gpr[15];
  state->aarch64_gpr[31] = monitor_sp;
  uint64_t restored_pc = frame.ucontext.mcontext.pc;
  if (restored_pc == 0) {
    if (frame.saved_trap.resume_pc != 0)
      restored_pc = frame.saved_trap.resume_pc;
    else if (frame.saved_trap.trap_pc != 0)
      restored_pc = frame.saved_trap.trap_pc;
  }
  const int redirected_context =
    (frame.interrupted_pc != 0 && restored_pc != frame.interrupted_pc) ||
    restored_sp != frame.aarch64_gpr[31];
  if (redirected_context) {
    poly_clear_return_transition_state(state);
  }
  state->header.current_mode = POLY_MODE_RAW_AARCH64;
  state->header.foreign_pc = restored_pc;
  memcpy(state->aarch64_fp, frame.aarch64_fp, sizeof(state->aarch64_fp));
  state->aarch64_status.nzcv = frame.aarch64_nzcv;
  state->aarch64_status.fpcr = frame.aarch64_fpcr;
  state->aarch64_status.fpsr = frame.aarch64_fpsr;
  if (state->trap.reason != 0) {
    state->trap.resume_pc = restored_pc;
    state->trap.flags |= POLY_TRAP_PACKET_REQUIRED_FLAGS |
      POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE;
    state->trap_restore = frame.saved_trap_restore;
    state->trap_restore.flags = POLY_TRAP_RESTORE_FLAG_VALID |
      POLY_TRAP_RESTORE_FLAG_AARCH64_STATE_VALID;
    state->trap_restore.mode = POLY_MODE_RAW_AARCH64;
    state->trap_restore.aarch64_gpr_valid_mask = POLY_AARCH64_GPR_VALID_MASK;
    memcpy(state->trap_restore.aarch64_gpr, frame.ucontext.mcontext.regs,
      sizeof(state->trap_restore.aarch64_gpr));
    state->trap_restore.aarch64_gpr[31] = restored_sp;
    memcpy(state->trap_restore.aarch64_fp, frame.aarch64_fp,
      sizeof(state->trap_restore.aarch64_fp));
    state->trap_restore.aarch64_nzcv = frame.aarch64_nzcv;
    state->trap_restore.aarch64_fpcr = frame.aarch64_fpcr;
    state->trap_restore.aarch64_fpsr = frame.aarch64_fpsr;
    poly_sanitize_aarch64_trap_restore_payload(&state->trap_restore);
    /*
     * Import-time C execution stays on state->aarch64_gpr[31] above. The trap
     * return restore stack is the AArch64 ucontext SP that raw code resumes on.
     */
    state->trap_restore.x86_gpr[15] = restored_sp;
  } else {
    memset(&state->trap, 0, sizeof(state->trap));
    memset(state->trap_args, 0, sizeof(state->trap_args));
    state->trap.source_mode = POLY_MODE_RAW_AARCH64;
    state->trap.reason = POLY_TRAP_BREAK;
    state->trap.trap_pc = restored_pc;
    state->trap.resume_pc = restored_pc;
    state->trap.flags = POLY_TRAP_PACKET_REQUIRED_FLAGS |
      POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE;
    memset(&state->trap_restore, 0, sizeof(state->trap_restore));
    state->trap_restore.flags = POLY_TRAP_RESTORE_FLAG_VALID |
      POLY_TRAP_RESTORE_FLAG_AARCH64_STATE_VALID;
    state->trap_restore.mode = POLY_MODE_RAW_AARCH64;
    state->trap_restore.aarch64_gpr_valid_mask = POLY_AARCH64_GPR_VALID_MASK;
    memcpy(state->trap_restore.aarch64_gpr, frame.ucontext.mcontext.regs,
      sizeof(state->trap_restore.aarch64_gpr));
    state->trap_restore.aarch64_gpr[31] = restored_sp;
    memcpy(state->trap_restore.aarch64_fp, frame.aarch64_fp,
      sizeof(state->trap_restore.aarch64_fp));
    state->trap_restore.aarch64_nzcv = frame.aarch64_nzcv;
    state->trap_restore.aarch64_fpcr = frame.aarch64_fpcr;
    state->trap_restore.aarch64_fpsr = frame.aarch64_fpsr;
    poly_sanitize_aarch64_trap_restore_payload(&state->trap_restore);
    state->trap_restore.x86_gpr[15] = restored_sp;
  }
  poly_guest_sigmask_shadow = frame.guest_sigmask;
  poly_guest_sigmask_shadow_valid = 1;
  poly_aarch64_active_signal_frame = 0;
  *result_out = frame.ucontext.mcontext.regs[0];

  if (poly_trace_protected_signal_waits_enabled()) {
    poly_write_literal_stderr(
      "POLYEXEC_GUEST_SIGNAL_RETURN: signum=0x");
    poly_write_hex64_stderr(frame.signum);
    poly_write_literal_stderr(" frame=0x");
    poly_write_hex64_stderr(frame_addr);
    poly_write_literal_stderr(" resume=0x");
    poly_write_hex64_stderr(frame.saved_trap.resume_pc);
	    poly_write_literal_stderr(" restore_rsp=0x");
	    poly_write_hex64_stderr(frame.saved_trap_restore.x86_gpr[15]);
	    poly_write_literal_stderr(" monitor_sp=0x");
	    poly_write_hex64_stderr(monitor_sp);
    poly_write_literal_stderr(" restore_x0=0x");
    poly_write_hex64_stderr(frame.aarch64_gpr[0]);
    poly_write_literal_stderr(" restore_x30=0x");
    poly_write_hex64_stderr(frame.aarch64_gpr[30]);
    poly_write_literal_stderr(" restore_sp=0x");
    poly_write_hex64_stderr(frame.aarch64_gpr[31]);
	    poly_write_literal_stderr(" restored_pc=0x");
	    poly_write_hex64_stderr(restored_pc);
	    poly_write_literal_stderr(" restored_uc_x28=0x");
	    poly_write_hex64_stderr(frame.ucontext.mcontext.regs[28]);
	    poly_write_literal_stderr(" restored_uc_x29=0x");
	    poly_write_hex64_stderr(frame.ucontext.mcontext.regs[29]);
	    poly_write_literal_stderr(" restored_uc_x30=0x");
	    poly_write_hex64_stderr(frame.ucontext.mcontext.regs[30]);
	    poly_write_literal_stderr(" restored_uc_sp=0x");
	    poly_write_hex64_stderr(frame.ucontext.mcontext.sp);
	    poly_write_literal_stderr(" redirected=0x");
	    poly_write_hex64_stderr((uint64_t) redirected_context);
	    poly_write_literal_stderr("\n");
	  }
  return 1;
}

static uint64_t poly_dispatch_rt_sigprocmask(uint64_t how, uint64_t set,
    uint64_t oldset, uint64_t sigsetsize) {
  if (sigsetsize != 8)
    return (uint64_t) -EINVAL;
  if (how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK)
    return (uint64_t) -EINVAL;

  uint64_t guest_set = 0;
  uint64_t host_set = 0;
  uint64_t host_oldset = 0;
  uint64_t *host_set_ptr = NULL;
  uint64_t *host_oldset_ptr = (oldset != 0 ||
    (set != 0 && !poly_guest_sigmask_shadow_valid)) ? &host_oldset : NULL;

  if (set != 0) {
    if (!poly_guest_read_u64(set, &guest_set)) {
      poly_write_literal_stderr("POLYEXEC_GUEST_SIGMASK_BADPTR: set=0x");
      poly_write_hex64_stderr(set);
      poly_write_literal_stderr("\n");
      return (uint64_t) -EFAULT;
    }
    host_set = poly_sanitize_host_runtime_sigset(guest_set);
    if (host_set != guest_set &&
        poly_trace_protected_signal_waits_enabled()) {
      poly_write_literal_stderr("POLYEXEC_GUEST_SIGMASK_VIRTUAL: how=0x");
      poly_write_hex64_stderr(how);
      poly_write_literal_stderr(" mask=0x");
      poly_write_hex64_stderr(guest_set);
      poly_write_literal_stderr(" sanitized=0x");
      poly_write_hex64_stderr(host_set);
      poly_write_literal_stderr("\n");
    }
    host_set_ptr = &host_set;
  }

  long status = poly_x86_syscall6(SYS_rt_sigprocmask, how,
    (uint64_t) (uintptr_t) host_set_ptr,
    (uint64_t) (uintptr_t) host_oldset_ptr, sigsetsize, 0, 0);
  if (status == 0) {
    if (!poly_guest_sigmask_shadow_valid) {
      poly_guest_sigmask_shadow = host_oldset_ptr != NULL ?
        poly_guest_initial_sigmask_from_host(host_oldset) : 0;
      poly_guest_sigmask_shadow_valid = 1;
    }

    const uint64_t guest_oldset = poly_guest_sigmask_shadow;
    if (set != 0)
      poly_guest_sigmask_shadow =
        poly_update_guest_sigmask(poly_guest_sigmask_shadow, how, guest_set);

    if (oldset != 0 && !poly_guest_write_u64(oldset, guest_oldset)) {
      poly_write_literal_stderr("POLYEXEC_GUEST_SIGMASK_BADPTR: oldset=0x");
      poly_write_hex64_stderr(oldset);
      poly_write_literal_stderr("\n");
      return (uint64_t) -EFAULT;
    }
  }
  return (uint64_t) status;
}

static uint64_t poly_dispatch_rt_sigsuspend(uint64_t set,
    uint64_t sigsetsize) {
  if (sigsetsize != 8)
    return (uint64_t) -EINVAL;
  if (set == 0)
    return (uint64_t) -EFAULT;

  uint64_t host_set = 0;
  if (!poly_guest_read_u64(set, &host_set)) {
    poly_write_literal_stderr("POLYEXEC_GUEST_SIGMASK_BADPTR: suspend=0x");
    poly_write_hex64_stderr(set);
    poly_write_literal_stderr("\n");
    return (uint64_t) -EFAULT;
  }
  host_set = poly_sanitize_host_runtime_sigset(host_set);
  long status = poly_x86_syscall6(SYS_rt_sigsuspend,
    (uint64_t) (uintptr_t) &host_set, sigsetsize, 0, 0, 0, 0);
  return (uint64_t) status;
}

static int poly_open_target_is_directory(uint64_t dirfd, uint64_t path) {
  if (path == 0)
    return 0;
  struct stat stat_result;
  long status = poly_x86_syscall6(SYS_newfstatat, dirfd, path,
    (uint64_t) (uintptr_t) &stat_result, AT_SYMLINK_NOFOLLOW, 0, 0);
  return status == 0 && S_ISDIR(stat_result.st_mode);
}

static uint64_t poly_translate_open_flags(uint64_t flags, uint64_t mode,
    uint64_t dirfd, uint64_t path) {
  if (mode != POLY_MODE_RAW_AARCH64)
    return flags;

  const uint64_t translated_mask = POLY_AARCH64_O_DIRECT |
    POLY_AARCH64_O_LARGEFILE | POLY_AARCH64_O_DIRECTORY |
    POLY_AARCH64_O_NOFOLLOW | POLY_AARCH64_O_NOATIME |
    POLY_AARCH64_O_CLOEXEC | POLY_AARCH64_O_PATH |
    POLY_AARCH64___O_TMPFILE;
  uint64_t translated = flags & ~translated_mask;
  if ((flags & POLY_AARCH64_O_DIRECTORY) != 0)
    translated |= O_DIRECTORY;
  if ((flags & POLY_AARCH64_O_DIRECT) != 0) {
    if (poly_open_target_is_directory(dirfd, path))
      translated |= O_DIRECTORY;
    else
      translated |= O_DIRECT;
  }
  if ((flags & POLY_AARCH64_O_LARGEFILE) != 0)
    translated |= O_LARGEFILE;
  if ((flags & POLY_AARCH64_O_NOFOLLOW) != 0)
    translated |= O_NOFOLLOW;
  if ((flags & POLY_AARCH64_O_NOATIME) != 0)
    translated |= O_NOATIME;
  if ((flags & POLY_AARCH64_O_CLOEXEC) != 0)
    translated |= O_CLOEXEC;
  if ((flags & POLY_AARCH64_O_PATH) != 0)
    translated |= O_PATH;
  if ((flags & POLY_AARCH64___O_TMPFILE) != 0)
    translated |= O_TMPFILE;
  return translated;
}

static uint64_t poly_dispatch_rt_sigaction(uint64_t mode, uint64_t signum,
    uint64_t act, uint64_t oldact, uint64_t sigsetsize) {
  if (!poly_is_raw_foreign_mode(mode))
    return (uint64_t) -ENOSYS;
  if (sigsetsize != 8)
    return (uint64_t) -EINVAL;
  const int protected_runtime_signal =
    poly_is_protected_runtime_signal(signum);
  if (signum == SIGCHLD && poly_virtual_sigchld_enabled())
    return poly_dispatch_virtual_sigchld_action(act, oldact);
  if (signum == SIGSEGV || signum == SIGBUS || signum == SIGILL ||
      protected_runtime_signal) {
    struct poly_linux_ksigaction guest_act;
    int have_guest_act = 0;
    if (act != 0) {
      have_guest_act = poly_guest_read_ksigaction(act, &guest_act);
      if (!have_guest_act) {
        poly_write_literal_stderr("POLYEXEC_GUEST_SIGACTION_BADPTR: act=0x");
        poly_write_hex64_stderr(act);
        poly_write_literal_stderr("\n");
      }
    }
    if (poly_trace_protected_signal_waits_enabled()) {
      poly_write_literal_stderr("POLYEXEC_GUEST_SIGACTION_VIRTUAL: signum=0x");
      poly_write_hex64_stderr(signum);
      poly_write_literal_stderr(" act=0x");
      poly_write_hex64_stderr(act);
      poly_write_literal_stderr(" handler=0x");
      poly_write_hex64_stderr(have_guest_act ? guest_act.handler : 0);
      poly_write_literal_stderr(" flags=0x");
      poly_write_hex64_stderr(have_guest_act ? guest_act.flags : 0);
      poly_write_literal_stderr(" oldact=0x");
      poly_write_hex64_stderr(oldact);
      poly_write_literal_stderr("\n");
    }
    poly_store_guest_signal_action(signum, oldact);
    if (have_guest_act) {
      if (protected_runtime_signal) {
        if (guest_act.handler == (uint64_t) (uintptr_t) SIG_DFL ||
            guest_act.handler == (uint64_t) (uintptr_t) SIG_IGN) {
          void (*host_handler)(int) =
            guest_act.handler == (uint64_t) (uintptr_t) SIG_IGN ? SIG_IGN :
              SIG_DFL;
          if (poly_set_host_signal_handler(signum, host_handler) < 0)
            return (uint64_t) -errno;
        } else if (poly_set_host_virtual_signal_handler(signum,
              &guest_act) < 0) {
          return (uint64_t) -errno;
        }
      }
      if (signum < sizeof(poly_guest_signal_action_shadow_valid)) {
        poly_guest_signal_action_shadow[signum] = guest_act;
        poly_guest_signal_action_shadow_valid[signum] = 1;
      }
    }
    if (signum == SIGSEGV || signum == SIGBUS || signum == SIGILL) {
      if (poly_install_auto_spill_signal_actions() < 0)
        return (uint64_t) -errno;
    }
    return 0;
  }

  struct poly_linux_ksigaction host_act;
  struct poly_linux_ksigaction host_oldact;
  struct poly_linux_ksigaction *host_act_ptr = NULL;
  struct poly_linux_ksigaction *host_oldact_ptr = NULL;

  if (act != 0) {
    if (!poly_guest_read_ksigaction(act, &host_act)) {
      poly_write_literal_stderr("POLYEXEC_GUEST_SIGACTION_BADPTR: act=0x");
      poly_write_hex64_stderr(act);
      poly_write_literal_stderr("\n");
      return 0;
    }
    poly_sanitize_guest_sigaction(&host_act);
    host_act_ptr = &host_act;
  }
  if (oldact != 0) {
    memset(&host_oldact, 0, sizeof(host_oldact));
    host_oldact_ptr = &host_oldact;
  }

  long status = poly_x86_syscall6(SYS_rt_sigaction, signum,
    (uint64_t) (uintptr_t) host_act_ptr,
    (uint64_t) (uintptr_t) host_oldact_ptr, sigsetsize, 0, 0);
  if (status == 0 && oldact != 0) {
    poly_sanitize_guest_sigaction(&host_oldact);
    if (!poly_guest_write_ksigaction(oldact, &host_oldact)) {
      poly_write_literal_stderr("POLYEXEC_GUEST_SIGACTION_BADPTR: oldact=0x");
      poly_write_hex64_stderr(oldact);
      poly_write_literal_stderr("\n");
    }
  }
  return (uint64_t) status;
}

static void poly_prefault_range(uint64_t address, uint64_t length,
    int writable) {
  if (!polyexec_use_auto_spill)
    return;
  if (address == 0 || length == 0 || length > (uint64_t) SIZE_MAX)
    return;
  volatile uint8_t *bytes = (volatile uint8_t *) (uintptr_t) address;
  const size_t len = (size_t) length;
  for (size_t offset = 0; offset < len; offset += 4096) {
    uint8_t value = bytes[offset];
    if (writable)
      bytes[offset] = value;
  }
  uint8_t value = bytes[len - 1];
  if (writable)
    bytes[len - 1] = value;
}

static void poly_prefault_writable_range(uint64_t address, uint64_t length) {
  poly_prefault_range(address, length, 1);
}

static void poly_prefault_writable_mapping_line(const char *line) {
  char *end = NULL;
  const uint64_t start = polyexec_strtoull_c(line, &end, 16);
  if (end == line || *end != '-')
    return;
  const char *stop_text = end + 1;
  const uint64_t stop = polyexec_strtoull_c(stop_text, &end, 16);
  if (end == stop_text || start >= stop)
    return;
  while (*end == ' ')
    end++;
  if (end[0] == '\0' || end[1] != 'w')
    return;
  poly_prefault_writable_range(start, stop - start);
}

static void poly_prefault_executable_mapping_line(const char *line) {
  char *end = NULL;
  const uint64_t start = polyexec_strtoull_c(line, &end, 16);
  if (end == line || *end != '-')
    return;
  const char *stop_text = end + 1;
  const uint64_t stop = polyexec_strtoull_c(stop_text, &end, 16);
  if (end == stop_text || start >= stop)
    return;
  while (*end == ' ')
    end++;
  if (end[0] != 'r' || end[2] != 'x')
    return;
  poly_prefault_range(start, stop - start, 0);
}

static void poly_prefault_writable_mappings(void) {
  int fd = (int) poly_x86_syscall6(SYS_openat, AT_FDCWD,
    (uint64_t) (uintptr_t) "/proc/self/maps", O_RDONLY | O_CLOEXEC,
    0, 0, 0);
  if (fd < 0)
    return;

  char buffer[8192];
  char line[512];
  size_t line_len = 0;
  for (;;) {
    long count = poly_x86_syscall6(SYS_read, fd,
      (uint64_t) (uintptr_t) buffer, sizeof(buffer), 0, 0, 0);
    if (count <= 0)
      break;
    for (long n = 0; n < count; n++) {
      char ch = buffer[n];
      if (ch == '\n') {
        line[line_len] = '\0';
        poly_prefault_writable_mapping_line(line);
        line_len = 0;
        continue;
      }
      if (line_len + 1 < sizeof(line))
        line[line_len++] = ch;
    }
  }
  if (line_len != 0) {
    line[line_len] = '\0';
    poly_prefault_writable_mapping_line(line);
  }
  (void) poly_x86_syscall6(SYS_close, fd, 0, 0, 0, 0, 0);
}

static void poly_prefault_executable_mappings(void) {
  int fd = (int) poly_x86_syscall6(SYS_openat, AT_FDCWD,
    (uint64_t) (uintptr_t) "/proc/self/maps", O_RDONLY | O_CLOEXEC,
    0, 0, 0);
  if (fd < 0)
    return;

  char buffer[8192];
  char line[512];
  size_t line_len = 0;
  for (;;) {
    long count = poly_x86_syscall6(SYS_read, fd,
      (uint64_t) (uintptr_t) buffer, sizeof(buffer), 0, 0, 0);
    if (count <= 0)
      break;
    for (long n = 0; n < count; n++) {
      char ch = buffer[n];
      if (ch == '\n') {
        line[line_len] = '\0';
        poly_prefault_executable_mapping_line(line);
        line_len = 0;
        continue;
      }
      if (line_len + 1 < sizeof(line))
        line[line_len++] = ch;
    }
  }
  if (line_len != 0) {
    line[line_len] = '\0';
    poly_prefault_executable_mapping_line(line);
  }
  (void) poly_x86_syscall6(SYS_close, fd, 0, 0, 0, 0, 0);
}

static void poly_trap_vector_handler(void);

static int poly_handle_structured_foreign_syscall(uint64_t number,
    uint64_t mode, uint64_t arg0, uint64_t arg1, uint64_t arg2,
    uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t *result) {
  struct stat stat_result;
  struct statfs statfs_result;
  long status;

  switch (number) {
    case 21: {
      struct poly_x86_epoll_event x86_event;
      uint64_t x86_event_ptr = 0;
      if (arg3 != 0) {
        poly_load_x86_epoll_event(&x86_event, arg3);
        x86_event_ptr = (uint64_t) (uintptr_t) &x86_event;
      }
      *result = (uint64_t) poly_x86_syscall6(SYS_epoll_ctl, arg0, arg1,
        arg2, x86_event_ptr, 0, 0);
      return 1;
    }
    case 22: {
      *result = poly_dispatch_epoll_wait_events(SYS_epoll_pwait, arg0, arg1,
        arg2, arg3, arg4, arg5);
      return 1;
    }
    case 29:
      if (arg0 == STDIN_FILENO && arg1 == TIOCGWINSZ) {
        if (poly_tiocgwinsz_stdin_enotty_cached) {
          __sync_fetch_and_add(&poly_tiocgwinsz_stdin_cache_hits, 1);
          *result = (uint64_t) -ENOTTY;
          return 1;
        }
        status = poly_x86_syscall6(SYS_ioctl, arg0, arg1, arg2, 0, 0, 0);
        if (status == -ENOTTY)
          poly_tiocgwinsz_stdin_enotty_cached = 1;
        __sync_fetch_and_add(&poly_tiocgwinsz_stdin_cache_misses, 1);
        *result = (uint64_t) status;
        return 1;
      }
      return 0;
    case 441: {
      *result = poly_dispatch_epoll_wait_events(SYS_epoll_pwait2, arg0, arg1,
        arg2, arg3, arg4, arg5);
      return 1;
    }
    case 43:
      status = poly_x86_syscall6(SYS_statfs, arg0,
        (uint64_t) (uintptr_t) &statfs_result, 0, 0, 0, 0);
      if (status == 0)
        poly_store_linux_generic_statfs(arg1, &statfs_result);
      *result = (uint64_t) status;
      return 1;
	    case 44:
	      status = poly_x86_syscall6(SYS_fstatfs, arg0,
	        (uint64_t) (uintptr_t) &statfs_result, 0, 0, 0, 0);
	      if (status == 0)
	        poly_store_linux_generic_statfs(arg1, &statfs_result);
	      *result = (uint64_t) status;
	      return 1;
    case 56: {
      uint64_t flags = poly_translate_open_flags(arg2, mode, arg0, arg1);
      status = poly_x86_syscall6(SYS_openat, arg0, arg1, flags, arg3, 0, 0);
      *result = (uint64_t) status;
      return 1;
    }
    case 61: {
      if (arg2 > SIZE_MAX) {
        *result = (uint64_t) -EINVAL;
        return 1;
      }
      size_t buffer_size = (size_t) arg2;
      void *entries = malloc(buffer_size == 0 ? 1 : buffer_size);
      if (entries == NULL) {
        *result = (uint64_t) -ENOMEM;
        return 1;
      }
      status = poly_x86_syscall6(SYS_getdents64, arg0,
        (uint64_t) (uintptr_t) entries, buffer_size, 0, 0, 0);
      if (status > 0)
        memcpy((void *) (uintptr_t) arg1, entries, (size_t) status);
      free(entries);
      *result = (uint64_t) status;
      return 1;
    }
    case 79:
      status = poly_x86_syscall6(SYS_newfstatat, arg0, arg1,
        (uint64_t) (uintptr_t) &stat_result, arg3, 0, 0);
      if (status == 0)
        poly_store_linux_generic_stat(arg2, &stat_result);
      *result = (uint64_t) status;
      return 1;
    case 80:
      status = poly_x86_syscall6(SYS_fstat, arg0,
        (uint64_t) (uintptr_t) &stat_result, 0, 0, 0, 0);
      if (status == 0)
        poly_store_linux_generic_stat(arg1, &stat_result);
      *result = (uint64_t) status;
      return 1;
    case 102:
      if (poly_is_raw_foreign_mode(mode) &&
          poly_protect_runtime_signals_enabled() &&
          arg0 == ITIMER_REAL) {
        if (arg1 != 0) {
          struct itimerval current;
          memset(&current, 0, sizeof(current));
          if (!poly_guest_range_is_mapped(arg1, sizeof(current), 1)) {
            *result = (uint64_t) -EFAULT;
            return 1;
          }
          *(struct itimerval *) (uintptr_t) arg1 = current;
        }
        *result = 0;
        return 1;
      }
      return 0;
    case 103:
      if (poly_is_raw_foreign_mode(mode) &&
          poly_protect_runtime_signals_enabled() &&
          arg0 == ITIMER_REAL) {
        if (arg2 != 0) {
          struct itimerval old_value;
          memset(&old_value, 0, sizeof(old_value));
          if (!poly_guest_range_is_mapped(arg2, sizeof(old_value), 1)) {
            *result = (uint64_t) -EFAULT;
            return 1;
          }
          *(struct itimerval *) (uintptr_t) arg2 = old_value;
        }
        *result = 0;
        return 1;
      }
      return 0;
    case 132:
      if (mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV) {
        *result = (uint64_t) poly_dispatch_guest_sigaltstack(mode, arg0, arg1);
        return 1;
      }
      return 0;
    case 133:
      if (poly_is_raw_foreign_mode(mode) &&
          poly_protect_runtime_signals_enabled()) {
        *result = poly_dispatch_rt_sigsuspend(arg0, arg1);
        return 1;
      }
      return 0;
    case 134:
      *result = poly_dispatch_rt_sigaction(mode, arg0, arg1, arg2, arg3);
      return 1;
    case 135:
      if (!poly_is_raw_foreign_mode(mode) ||
          !poly_protect_runtime_signals_enabled())
        return 0;
      *result = poly_dispatch_rt_sigprocmask(arg0, arg1, arg2, arg3);
      return 1;
    case 160:
      status = poly_x86_syscall6(SYS_uname, arg0, 0, 0, 0, 0, 0);
      if (status == 0 && arg0 != 0) {
        struct poly_utsname *uts = (struct poly_utsname *) (uintptr_t) arg0;
        poly_store_fixed_string(uts->machine, sizeof(uts->machine),
          mode == POLY_MODE_RAW_AARCH64 ? "aarch64" : "riscv64");
      }
      *result = (uint64_t) status;
      return 1;
    case POLY_RISCV_SYS_HWPROBE:
      if (mode != POLY_MODE_RAW_RISCV)
        return 0;
      *result = poly_dispatch_riscv_hwprobe(arg0, arg1, arg2, arg3, arg4);
      return 1;
    case 214:
      if (!poly_process_exit_finalizers.active)
        return 0;
      *result = poly_dispatch_process_brk(arg0);
      return 1;
    case 196: {
      *result = (uint64_t) poly_x86_syscall6(SYS_shmat, arg0, arg1, arg2,
        0, 0, 0);
      if (poly_process_exit_finalizers.active && (int64_t) *result >= 0) {
        struct shmid_ds shm_stat;
        uint64_t prefault_length = 1;
        if (poly_x86_syscall6(SYS_shmctl, arg0, IPC_STAT,
              (uint64_t) (uintptr_t) &shm_stat, 0, 0, 0) == 0 &&
            shm_stat.shm_segsz > 0)
          prefault_length = (uint64_t) shm_stat.shm_segsz;
        poly_prefault_range(*result, prefault_length,
          (arg2 & SHM_RDONLY) == 0);
      }
      return 1;
    }
    case 216:
      if (!poly_process_exit_finalizers.active)
        return 0;
      *result = (uint64_t) poly_x86_syscall6(SYS_mremap, arg0, arg1, arg2,
        arg3, arg4, arg5);
      if ((int64_t) *result >= 0)
        poly_prefault_writable_range(*result, arg2);
      return 1;
    case 226:
      if (!poly_process_exit_finalizers.active)
        return 0;
      *result = (uint64_t) poly_x86_syscall6(SYS_mprotect, arg0, arg1, arg2,
        0, 0, 0);
      if (*result == 0 && (arg2 & (PROT_READ | PROT_WRITE | PROT_EXEC)) != 0)
        poly_prefault_range(arg0, arg1, (arg2 & PROT_WRITE) != 0);
      return 1;
    case 222: {
      uint64_t flags = arg3;
      if (poly_process_exit_finalizers.active &&
          (arg2 & PROT_EXEC) != 0 &&
          (flags & MAP_PRIVATE) != 0 &&
          (flags & MAP_ANONYMOUS) == 0 &&
          (int64_t) arg4 >= 0) {
        *result = poly_dispatch_private_exec_file_mmap(arg0, arg1, arg2,
          flags, arg4, arg5);
        return 1;
      }
      if (poly_prefault_guest_mmaps_enabled() && (arg2 & PROT_WRITE) != 0)
        flags |= MAP_POPULATE;
      *result = (uint64_t) poly_x86_syscall6(SYS_mmap, arg0, arg1, arg2,
        flags, arg4, arg5);
      if (poly_process_exit_finalizers.active &&
          poly_prefault_guest_mmaps_enabled() &&
          (int64_t) *result >= 0 &&
          (arg2 & (PROT_READ | PROT_WRITE | PROT_EXEC)) != 0) {
        uint64_t prefault_length = arg1;
        if ((flags & MAP_ANONYMOUS) == 0 && (int64_t) arg4 >= 0) {
          struct stat mmap_stat;
          if (fstat((int) arg4, &mmap_stat) == 0 && mmap_stat.st_size >= 0) {
            const uint64_t file_size = (uint64_t) mmap_stat.st_size;
            prefault_length =
              arg5 < file_size && file_size - arg5 < prefault_length ?
              file_size - arg5 : prefault_length;
            if (arg5 >= file_size)
              prefault_length = 0;
          }
        }
        poly_prefault_range(*result, prefault_length,
          (arg2 & PROT_WRITE) != 0);
      }
      return 1;
    }
    default:
      return 0;
  }
}

static int poly_fd_returning_x86_syscall(long x86_number) {
  switch (x86_number) {
    case SYS_accept:
    case SYS_accept4:
    case SYS_epoll_create1:
    case SYS_eventfd2:
    case SYS_inotify_init1:
    case SYS_memfd_create:
    case SYS_openat:
    case SYS_openat2:
    case SYS_pidfd_open:
    case SYS_socket:
    case SYS_timerfd_create:
      return 1;
    default:
      return 0;
  }
}

static void poly_clear_stdin_tiocgwinsz_cache(void) {
  poly_tiocgwinsz_stdin_enotty_cached = 0;
}

static void poly_note_syscall_fd_effect(
    const struct poly_runtime_trap_packet *packet, long x86_number,
    uint64_t result) {
  if (!poly_tiocgwinsz_stdin_enotty_cached || packet == NULL ||
      (int64_t) result < 0)
    return;

  if ((packet->number == 57 || x86_number == SYS_close) &&
      packet->args[0] == STDIN_FILENO) {
    poly_clear_stdin_tiocgwinsz_cache();
    return;
  }
  if ((packet->number == 436 || x86_number == SYS_close_range) &&
      packet->args[0] <= STDIN_FILENO &&
      packet->args[1] >= STDIN_FILENO) {
    poly_clear_stdin_tiocgwinsz_cache();
    return;
  }
  if (packet->number == 23 || x86_number == SYS_dup ||
      poly_fd_returning_x86_syscall(x86_number) ||
      packet->number == 20 || packet->number == 26 ||
      packet->number == 56 || packet->number == 198 ||
      packet->number == 437) {
    if (result == STDIN_FILENO)
      poly_clear_stdin_tiocgwinsz_cache();
    return;
  }
  if (packet->number == 24 || x86_number == SYS_dup3) {
    if (packet->args[1] == STDIN_FILENO || result == STDIN_FILENO)
      poly_clear_stdin_tiocgwinsz_cache();
    return;
  }
  if ((packet->number == 25 || x86_number == SYS_fcntl) &&
      (packet->args[1] == F_DUPFD || packet->args[1] == F_DUPFD_CLOEXEC) &&
      result == STDIN_FILENO) {
    poly_clear_stdin_tiocgwinsz_cache();
    return;
  }
  if (packet->number == 59 || x86_number == SYS_pipe2)
    poly_clear_stdin_tiocgwinsz_cache();
}

static int poly_generic_linux_syscall_to_x86(uint64_t number, long *x86_number) {
  switch (number) {
    case 5: *x86_number = SYS_setxattr; return 1;
    case 6: *x86_number = SYS_lsetxattr; return 1;
    case 7: *x86_number = SYS_fsetxattr; return 1;
    case 8: *x86_number = SYS_getxattr; return 1;
    case 9: *x86_number = SYS_lgetxattr; return 1;
    case 10: *x86_number = SYS_fgetxattr; return 1;
    case 11: *x86_number = SYS_listxattr; return 1;
    case 12: *x86_number = SYS_llistxattr; return 1;
    case 13: *x86_number = SYS_flistxattr; return 1;
    case 14: *x86_number = SYS_removexattr; return 1;
    case 15: *x86_number = SYS_lremovexattr; return 1;
    case 16: *x86_number = SYS_fremovexattr; return 1;
    case 17: *x86_number = SYS_getcwd; return 1;
    case 19: *x86_number = SYS_eventfd2; return 1;
    case 20: *x86_number = SYS_epoll_create1; return 1;
    case 21: *x86_number = SYS_epoll_ctl; return 1;
    case 22: *x86_number = SYS_epoll_pwait; return 1;
    case 23: *x86_number = SYS_dup; return 1;
    case 24: *x86_number = SYS_dup3; return 1;
    case 25: *x86_number = SYS_fcntl; return 1;
    case 26: *x86_number = SYS_inotify_init1; return 1;
    case 27: *x86_number = SYS_inotify_add_watch; return 1;
    case 28: *x86_number = SYS_inotify_rm_watch; return 1;
    case 29: *x86_number = SYS_ioctl; return 1;
    case 30: *x86_number = SYS_ioprio_set; return 1;
    case 31: *x86_number = SYS_ioprio_get; return 1;
    case 32: *x86_number = SYS_flock; return 1;
    case 33: *x86_number = SYS_mknodat; return 1;
    case 34: *x86_number = SYS_mkdirat; return 1;
    case 35: *x86_number = SYS_unlinkat; return 1;
    case 36: *x86_number = SYS_symlinkat; return 1;
    case 37: *x86_number = SYS_linkat; return 1;
    case 38: *x86_number = SYS_renameat; return 1;
    case 39: *x86_number = SYS_umount2; return 1;
    case 40: *x86_number = SYS_mount; return 1;
    case 41: *x86_number = SYS_pivot_root; return 1;
    case 43: *x86_number = SYS_statfs; return 1;
    case 44: *x86_number = SYS_fstatfs; return 1;
    case 45: *x86_number = SYS_truncate; return 1;
    case 46: *x86_number = SYS_ftruncate; return 1;
    case 47: *x86_number = SYS_fallocate; return 1;
    case 48: *x86_number = SYS_faccessat; return 1;
    case 49: *x86_number = SYS_chdir; return 1;
    case 50: *x86_number = SYS_fchdir; return 1;
    case 51: *x86_number = SYS_chroot; return 1;
    case 52: *x86_number = SYS_fchmod; return 1;
    case 53: *x86_number = SYS_fchmodat; return 1;
    case 54: *x86_number = SYS_fchownat; return 1;
    case 55: *x86_number = SYS_fchown; return 1;
    case 56: *x86_number = SYS_openat; return 1;
    case 57: *x86_number = SYS_close; return 1;
    case 59: *x86_number = SYS_pipe2; return 1;
    case 61: *x86_number = SYS_getdents64; return 1;
    case 62: *x86_number = SYS_lseek; return 1;
    case 63: *x86_number = SYS_read; return 1;
    case 64: *x86_number = SYS_write; return 1;
    case 65: *x86_number = SYS_readv; return 1;
    case 66: *x86_number = SYS_writev; return 1;
    case 67: *x86_number = SYS_pread64; return 1;
    case 68: *x86_number = SYS_pwrite64; return 1;
    case 69: *x86_number = SYS_preadv; return 1;
    case 70: *x86_number = SYS_pwritev; return 1;
    case 72: *x86_number = SYS_pselect6; return 1;
    case 73: *x86_number = SYS_ppoll; return 1;
    case 74: *x86_number = SYS_signalfd4; return 1;
    case 78: *x86_number = SYS_readlinkat; return 1;
    case 79: *x86_number = SYS_newfstatat; return 1;
    case 80: *x86_number = SYS_fstat; return 1;
    case 81: *x86_number = SYS_sync; return 1;
    case 82: *x86_number = SYS_fsync; return 1;
    case 83: *x86_number = SYS_fdatasync; return 1;
    case 84: *x86_number = SYS_sync_file_range; return 1;
    case 85: *x86_number = SYS_timerfd_create; return 1;
    case 86: *x86_number = SYS_timerfd_settime; return 1;
    case 87: *x86_number = SYS_timerfd_gettime; return 1;
    case 88: *x86_number = SYS_utimensat; return 1;
    case 90: *x86_number = SYS_capget; return 1;
    case 91: *x86_number = SYS_capset; return 1;
    case 92: *x86_number = SYS_personality; return 1;
    case 93: *x86_number = SYS_exit; return 1;
    case 94: *x86_number = SYS_exit_group; return 1;
    case 95: *x86_number = SYS_waitid; return 1;
    case 96: *x86_number = SYS_set_tid_address; return 1;
    case 98: *x86_number = SYS_futex; return 1;
    case 99: *x86_number = SYS_set_robust_list; return 1;
    case 100: *x86_number = SYS_get_robust_list; return 1;
    case 101: *x86_number = SYS_nanosleep; return 1;
    case 102: *x86_number = SYS_getitimer; return 1;
    case 103: *x86_number = SYS_setitimer; return 1;
    case 107: *x86_number = SYS_timer_create; return 1;
    case 108: *x86_number = SYS_timer_gettime; return 1;
    case 109: *x86_number = SYS_timer_getoverrun; return 1;
    case 110: *x86_number = SYS_timer_settime; return 1;
    case 111: *x86_number = SYS_timer_delete; return 1;
    case 113: *x86_number = SYS_clock_gettime; return 1;
    case 114: *x86_number = SYS_clock_getres; return 1;
    case 115: *x86_number = SYS_clock_nanosleep; return 1;
    case 118: *x86_number = SYS_sched_setparam; return 1;
    case 119: *x86_number = SYS_sched_setscheduler; return 1;
    case 120: *x86_number = SYS_sched_getscheduler; return 1;
    case 121: *x86_number = SYS_sched_getparam; return 1;
    case 122: *x86_number = SYS_sched_setaffinity; return 1;
    case 123: *x86_number = SYS_sched_getaffinity; return 1;
    case 124: *x86_number = SYS_sched_yield; return 1;
    case 125: *x86_number = SYS_sched_get_priority_max; return 1;
    case 126: *x86_number = SYS_sched_get_priority_min; return 1;
    case 129: *x86_number = SYS_kill; return 1;
    case 130: *x86_number = SYS_tkill; return 1;
    case 131: *x86_number = SYS_tgkill; return 1;
    case 132: *x86_number = SYS_sigaltstack; return 1;
    case 133: *x86_number = SYS_rt_sigsuspend; return 1;
    case 134: *x86_number = SYS_rt_sigaction; return 1;
    case 135: *x86_number = SYS_rt_sigprocmask; return 1;
    case 136: *x86_number = SYS_rt_sigpending; return 1;
    case 140: *x86_number = SYS_setpriority; return 1;
    case 141: *x86_number = SYS_getpriority; return 1;
    case 143: *x86_number = SYS_setregid; return 1;
    case 144: *x86_number = SYS_setgid; return 1;
    case 145: *x86_number = SYS_setreuid; return 1;
    case 146: *x86_number = SYS_setuid; return 1;
    case 147: *x86_number = SYS_setresuid; return 1;
    case 148: *x86_number = SYS_getresuid; return 1;
    case 149: *x86_number = SYS_setresgid; return 1;
    case 150: *x86_number = SYS_getresgid; return 1;
    case 151: *x86_number = SYS_setfsuid; return 1;
    case 152: *x86_number = SYS_setfsgid; return 1;
    case 153: *x86_number = SYS_times; return 1;
    case 154: *x86_number = SYS_setpgid; return 1;
    case 160: *x86_number = SYS_uname; return 1;
    case 163: *x86_number = SYS_getrlimit; return 1;
    case 164: *x86_number = SYS_setrlimit; return 1;
    case 165: *x86_number = SYS_getrusage; return 1;
    case 166: *x86_number = SYS_umask; return 1;
    case 167: *x86_number = SYS_prctl; return 1;
    case 168: *x86_number = SYS_getcpu; return 1;
    case 169: *x86_number = SYS_gettimeofday; return 1;
    case 155: *x86_number = SYS_getpgid; return 1;
    case 156: *x86_number = SYS_getsid; return 1;
    case 157: *x86_number = SYS_setsid; return 1;
    case 158: *x86_number = SYS_getgroups; return 1;
    case 159: *x86_number = SYS_setgroups; return 1;
    case 172: *x86_number = SYS_getpid; return 1;
    case 173: *x86_number = SYS_getppid; return 1;
    case 174: *x86_number = SYS_getuid; return 1;
    case 175: *x86_number = SYS_geteuid; return 1;
    case 176: *x86_number = SYS_getgid; return 1;
    case 177: *x86_number = SYS_getegid; return 1;
    case 178: *x86_number = SYS_gettid; return 1;
    case 179: *x86_number = SYS_sysinfo; return 1;
#ifdef SYS_mq_open
    case 180: *x86_number = SYS_mq_open; return 1;
#endif
#ifdef SYS_mq_unlink
    case 181: *x86_number = SYS_mq_unlink; return 1;
#endif
#ifdef SYS_mq_timedsend
    case 182: *x86_number = SYS_mq_timedsend; return 1;
#endif
#ifdef SYS_mq_timedreceive
    case 183: *x86_number = SYS_mq_timedreceive; return 1;
#endif
#ifdef SYS_mq_notify
    case 184: *x86_number = SYS_mq_notify; return 1;
#endif
#ifdef SYS_mq_getsetattr
    case 185: *x86_number = SYS_mq_getsetattr; return 1;
#endif
    case 186: *x86_number = SYS_msgget; return 1;
    case 187: *x86_number = SYS_msgctl; return 1;
    case 188: *x86_number = SYS_msgrcv; return 1;
    case 189: *x86_number = SYS_msgsnd; return 1;
    case 190: *x86_number = SYS_semget; return 1;
    case 191: *x86_number = SYS_semctl; return 1;
    case 192: *x86_number = SYS_semtimedop; return 1;
    case 193: *x86_number = SYS_semop; return 1;
    case 194: *x86_number = SYS_shmget; return 1;
    case 195: *x86_number = SYS_shmctl; return 1;
    case 196: *x86_number = SYS_shmat; return 1;
    case 197: *x86_number = SYS_shmdt; return 1;
    case 198: *x86_number = SYS_socket; return 1;
    case 199: *x86_number = SYS_socketpair; return 1;
    case 200: *x86_number = SYS_bind; return 1;
    case 201: *x86_number = SYS_listen; return 1;
    case 202: *x86_number = SYS_accept; return 1;
    case 203: *x86_number = SYS_connect; return 1;
    case 204: *x86_number = SYS_getsockname; return 1;
    case 205: *x86_number = SYS_getpeername; return 1;
    case 206: *x86_number = SYS_sendto; return 1;
    case 207: *x86_number = SYS_recvfrom; return 1;
    case 208: *x86_number = SYS_setsockopt; return 1;
    case 209: *x86_number = SYS_getsockopt; return 1;
    case 210: *x86_number = SYS_shutdown; return 1;
    case 211: *x86_number = SYS_sendmsg; return 1;
    case 212: *x86_number = SYS_recvmsg; return 1;
    case 214: *x86_number = SYS_brk; return 1;
    case 215: *x86_number = SYS_munmap; return 1;
    case 216: *x86_number = SYS_mremap; return 1;
    case 220: *x86_number = SYS_clone; return 1;
    case 221: *x86_number = SYS_execve; return 1;
    case 222: *x86_number = SYS_mmap; return 1;
    case 223: *x86_number = SYS_fadvise64; return 1;
    case 226: *x86_number = SYS_mprotect; return 1;
    case 228: *x86_number = SYS_mlock; return 1;
    case 229: *x86_number = SYS_munlock; return 1;
    case 230: *x86_number = SYS_mlockall; return 1;
    case 231: *x86_number = SYS_munlockall; return 1;
    case 233: *x86_number = SYS_madvise; return 1;
    case 236: *x86_number = SYS_get_mempolicy; return 1;
    case 237: *x86_number = SYS_set_mempolicy; return 1;
    case 238: *x86_number = SYS_migrate_pages; return 1;
    case 239: *x86_number = SYS_move_pages; return 1;
    case 242: *x86_number = SYS_accept4; return 1;
    case 243: *x86_number = SYS_recvmmsg; return 1;
    case 260: *x86_number = SYS_wait4; return 1;
    case 261: *x86_number = SYS_prlimit64; return 1;
    case 269: *x86_number = SYS_sendmmsg; return 1;
    case 276: *x86_number = SYS_renameat2; return 1;
    case 277: *x86_number = SYS_seccomp; return 1;
    case 278: *x86_number = SYS_getrandom; return 1;
    case 279: *x86_number = SYS_memfd_create; return 1;
    case 280: *x86_number = SYS_bpf; return 1;
    case 282: *x86_number = SYS_userfaultfd; return 1;
    case 283: *x86_number = SYS_membarrier; return 1;
    case 284: *x86_number = SYS_mlock2; return 1;
    case 288: *x86_number = SYS_pkey_mprotect; return 1;
    case 289: *x86_number = SYS_pkey_alloc; return 1;
    case 290: *x86_number = SYS_pkey_free; return 1;
    case 291: *x86_number = SYS_statx; return 1;
    case 293: *x86_number = SYS_rseq; return 1;
    case 424: *x86_number = SYS_pidfd_send_signal; return 1;
    case 425: *x86_number = SYS_io_uring_setup; return 1;
    case 426: *x86_number = SYS_io_uring_enter; return 1;
    case 427: *x86_number = SYS_io_uring_register; return 1;
    case 428: *x86_number = SYS_open_tree; return 1;
    case 429: *x86_number = SYS_move_mount; return 1;
    case 430: *x86_number = SYS_fsopen; return 1;
    case 431: *x86_number = SYS_fsconfig; return 1;
    case 432: *x86_number = SYS_fsmount; return 1;
    case 433: *x86_number = SYS_fspick; return 1;
    case 434: *x86_number = SYS_pidfd_open; return 1;
    case 435: *x86_number = SYS_clone3; return 1;
    case 436: *x86_number = SYS_close_range; return 1;
    case 437: *x86_number = SYS_openat2; return 1;
    case 438: *x86_number = SYS_pidfd_getfd; return 1;
    case 439: *x86_number = SYS_faccessat2; return 1;
    case 440: *x86_number = SYS_process_madvise; return 1;
    case 441: *x86_number = SYS_epoll_pwait2; return 1;
    case 442: *x86_number = SYS_mount_setattr; return 1;
#ifdef SYS_quotactl_fd
    case 443: *x86_number = SYS_quotactl_fd; return 1;
#endif
    case 444: *x86_number = SYS_landlock_create_ruleset; return 1;
    case 445: *x86_number = SYS_landlock_add_rule; return 1;
    case 446: *x86_number = SYS_landlock_restrict_self; return 1;
#ifdef SYS_memfd_secret
    case 447: *x86_number = SYS_memfd_secret; return 1;
#endif
    case 448: *x86_number = SYS_process_mrelease; return 1;
    case 449: *x86_number = SYS_futex_waitv; return 1;
    case 450: *x86_number = SYS_set_mempolicy_home_node; return 1;
#ifdef SYS_cachestat
    case 451: *x86_number = SYS_cachestat; return 1;
#endif
#ifdef SYS_fchmodat2
    case 452: *x86_number = SYS_fchmodat2; return 1;
#endif
#ifdef SYS_map_shadow_stack
    case 453: *x86_number = SYS_map_shadow_stack; return 1;
#endif
#ifdef SYS_futex_wake
    case 454: *x86_number = SYS_futex_wake; return 1;
#endif
#ifdef SYS_futex_wait
    case 455: *x86_number = SYS_futex_wait; return 1;
#endif
#ifdef SYS_futex_requeue
    case 456: *x86_number = SYS_futex_requeue; return 1;
#endif
#ifdef SYS_statmount
    case 457: *x86_number = SYS_statmount; return 1;
#endif
#ifdef SYS_listmount
    case 458: *x86_number = SYS_listmount; return 1;
#endif
#ifdef SYS_lsm_get_self_attr
    case 459: *x86_number = SYS_lsm_get_self_attr; return 1;
#endif
#ifdef SYS_lsm_set_self_attr
    case 460: *x86_number = SYS_lsm_set_self_attr; return 1;
#endif
#ifdef SYS_lsm_list_modules
    case 461: *x86_number = SYS_lsm_list_modules; return 1;
#endif
    default: return 0;
  }
}

static int read_poly_monitor_packet(struct poly_runtime_trap_packet *packet) {
  const uint64_t required_flags = POLY_TRAP_PACKET_REQUIRED_FLAGS;
  uint64_t packet_addr = poly_monitor_packet_get_value();
  if (packet_addr != 0 &&
      packet_addr != (uint64_t) (uintptr_t) poly_monitor_packet &&
      !poly_guest_range_is_mapped(packet_addr, sizeof(poly_monitor_packet), 0))
    return -1;
  volatile uint64_t *monitor_packet =
    packet_addr != 0 ? (volatile uint64_t *) (uintptr_t) packet_addr :
      poly_monitor_packet;
  const uint64_t header = monitor_packet[0];
  packet->reason = header & 0xffffffffULL;
  packet->mode = header >> 32;
  packet->number = monitor_packet[1];
  packet->selector = monitor_packet[2];
  packet->pc = monitor_packet[3];
  packet->next_pc = monitor_packet[4];
  packet->flags = monitor_packet[5];
  packet->reserved[0] = monitor_packet[6];
  packet->reserved[1] = monitor_packet[7];
  for (size_t n = 0; n < 8; n++)
    packet->args[n] = monitor_packet[8 + n];

  if (packet->next_pc == 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: monitor packet missing next_pc reason=%llu mode=%llu number=%llu selector=%llu pc=%llu flags=0x%llx\n",
      (unsigned long long) packet->reason,
      (unsigned long long) packet->mode,
      (unsigned long long) packet->number,
      (unsigned long long) packet->selector,
      (unsigned long long) packet->pc,
      (unsigned long long) packet->flags);
    return -1;
  }
  if (packet->reserved[0] != 0 || packet->reserved[1] != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: monitor packet reserved fields nonzero reason=%llu mode=%llu number=%llu selector=%llu pc=%llu reserved=(0x%llx,0x%llx)\n",
      (unsigned long long) packet->reason,
      (unsigned long long) packet->mode,
      (unsigned long long) packet->number,
      (unsigned long long) packet->selector,
      (unsigned long long) packet->pc,
      (unsigned long long) packet->reserved[0],
      (unsigned long long) packet->reserved[1]);
    return -1;
  }
  if ((packet->flags & required_flags) != required_flags) {
    fprintf(stderr,
      "POLYEXEC_FAIL: monitor packet missing flags reason=%llu mode=%llu number=%llu selector=%llu pc=%llu flags=0x%llx required=0x%llx\n",
      (unsigned long long) packet->reason,
      (unsigned long long) packet->mode,
      (unsigned long long) packet->number,
      (unsigned long long) packet->selector,
      (unsigned long long) packet->pc,
      (unsigned long long) packet->flags,
      (unsigned long long) required_flags);
    return -1;
  }

  return 0;
}

static int read_poly_v2_event_frame(struct poly_v2_event_frame *event) {
  const volatile struct poly_v2_event_frame *src =
    &poly_auto_spill_event_frame;
  if (src->magic == 0)
    return 0;
  memcpy(event, (const void *) src, sizeof(*event));
  if (event->magic != POLY_V2_EVENT_MAGIC ||
      event->bytes != POLY_V2_EVENT_BYTES ||
      event->version != POLY_V2_EVENT_VERSION ||
      event->header_bytes != POLY_V2_EVENT_HEADER_BYTES ||
      event->arg_count != POLY_V2_EVENT_ARG_COUNT)
    return -1;
  return 1;
}

static int poly_v2_event_kind_matches_packet(uint16_t event_kind,
    uint64_t packet_reason) {
  switch (event_kind) {
    case POLY_V2_EVENT_KIND_SYSCALL:
      return packet_reason == POLY_TRAP_SYSCALL;
    case POLY_V2_EVENT_KIND_BREAK:
      return packet_reason == POLY_TRAP_BREAK;
    case POLY_V2_EVENT_KIND_IMPORT:
      return packet_reason == POLY_TRAP_IMPORT;
    case POLY_V2_EVENT_KIND_ILLEGAL:
      return packet_reason == POLY_TRAP_ILLEGAL;
    default:
      return 0;
  }
}

static int apply_poly_v2_event_to_packet(
    const struct poly_v2_event_frame *event,
    struct poly_runtime_trap_packet *packet) {
  if (event->sequence == 0 || event->sequence == poly_v2_last_event_sequence)
    return 0;
  if (!poly_v2_event_kind_matches_packet(event->event_kind,
        packet->reason) ||
      event->frontend != packet->mode ||
      event->selector != packet->number ||
      event->insn_pc != packet->pc ||
      event->resume_pc == 0) {
    return 0;
  }

  poly_v2_last_event_sequence = event->sequence;
  packet->pc = event->insn_pc;
  packet->next_pc = event->resume_pc;
  packet->number = event->selector;
  for (size_t n = 0; n < POLY_V2_EVENT_ARG_COUNT; n++)
    packet->args[n] = event->args[n];
  return 0;
}

static char *polyexec_stpcpy(char *dest, const char *src) {
  while ((*dest = *src) != '\0') {
    dest++;
    src++;
  }
  return dest;
}

static char *polyexec_stpncpy(char *dest, const char *src, size_t n) {
  size_t copied = 0;
  while (copied < n && src[copied] != '\0') {
    dest[copied] = src[copied];
    copied++;
  }
  char *ret = dest + copied;
  while (copied < n)
    dest[copied++] = '\0';
  return ret;
}

static void *polyexec_mempcpy(void *dest, const void *src, size_t n) {
  memcpy(dest, src, n);
  return (uint8_t *) dest + n;
}

static void *polyexec_rawmemchr(const void *s, int c) {
  const unsigned char *p = (const unsigned char *) s;
  const unsigned char needle = (unsigned char) c;
  while (*p != needle)
    p++;
  return (void *) p;
}

static char *polyexec_strchrnul(const char *s, int c) {
  const unsigned char needle = (unsigned char) c;
  while (*s != '\0' && (unsigned char) *s != needle)
    s++;
  return (char *) s;
}

static void *polyexec_memrchr(const void *s, int c, size_t n) {
  const unsigned char *p = (const unsigned char *) s + n;
  const unsigned char needle = (unsigned char) c;
  while (p != (const unsigned char *) s) {
    p--;
    if (*p == needle)
      return (void *) p;
  }
  return NULL;
}

static void *polyexec_memmem(const void *haystack, size_t haystack_len,
    const void *needle, size_t needle_len) {
  if (needle_len == 0)
    return (void *) haystack;
  if (needle_len > haystack_len)
    return NULL;
  const unsigned char *h = (const unsigned char *) haystack;
  const unsigned char *n = (const unsigned char *) needle;
  for (size_t offset = 0; offset <= haystack_len - needle_len; offset++) {
    if (h[offset] == n[0] &&
        memcmp(h + offset, n, needle_len) == 0)
      return (void *) (h + offset);
  }
  return NULL;
}

static int polyexec_ascii_isspace(unsigned char c) {
  return c == ' ' || c == '\f' || c == '\n' || c == '\r' ||
    c == '\t' || c == '\v';
}

static int polyexec_ascii_isdigit(unsigned char c) {
  return c >= '0' && c <= '9';
}

static int polyexec_ascii_isalpha(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int polyexec_ascii_isupper(unsigned char c) {
  return c >= 'A' && c <= 'Z';
}

static int polyexec_ascii_islower(unsigned char c) {
  return c >= 'a' && c <= 'z';
}

static int polyexec_ascii_isxdigit(unsigned char c) {
  return polyexec_ascii_isdigit(c) ||
    (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static int polyexec_ascii_isblank(unsigned char c) {
  return c == ' ' || c == '\t';
}

static int polyexec_ascii_iscntrl(unsigned char c) {
  return c < 0x20 || c == 0x7f;
}

static int polyexec_ascii_isprint(unsigned char c) {
  return c >= 0x20 && c < 0x7f;
}

static int polyexec_ascii_isgraph(unsigned char c) {
  return c > 0x20 && c < 0x7f;
}

static int polyexec_ascii_ispunct(unsigned char c) {
  return polyexec_ascii_isgraph(c) &&
    !polyexec_ascii_isalpha(c) && !polyexec_ascii_isdigit(c);
}

static int polyexec_digit_value(unsigned char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 10;
  return -1;
}

static uint64_t polyexec_strtoull_c(const char *nptr, char **endptr,
    int base) {
  const char *p = nptr;
  while (polyexec_ascii_isspace((unsigned char) *p))
    p++;

  int negative = 0;
  if (*p == '+' || *p == '-') {
    negative = *p == '-';
    p++;
  }

  if ((base == 0 || base == 16) && p[0] == '0' &&
      (p[1] == 'x' || p[1] == 'X')) {
    base = 16;
    p += 2;
  } else if (base == 0 && *p == '0') {
    base = 8;
    p++;
  } else if (base == 0) {
    base = 10;
  }
  if (base < 2 || base > 36) {
    if (endptr != NULL)
      *endptr = (char *) nptr;
    return 0;
  }

  const char *first_digit = p;
  uint64_t value = 0;
  for (;;) {
    const int digit = polyexec_digit_value((unsigned char) *p);
    if (digit < 0 || digit >= base)
      break;
    value = value * (uint64_t) base + (uint64_t) digit;
    p++;
  }
  if (p == first_digit) {
    if (endptr != NULL)
      *endptr = (char *) nptr;
    return 0;
  }
  if (endptr != NULL)
    *endptr = (char *) p;
  return negative ? (uint64_t) -value : value;
}

static int64_t polyexec_strtoll_c(const char *nptr, char **endptr, int base) {
  return (int64_t) polyexec_strtoull_c(nptr, endptr, base);
}

static uint64_t poly_handle_foreign_import(uint64_t number,
    const uint64_t args[8]) {
  switch (number) {
    case POLY_IMPORT_FUNC_STRLEN:
      return strlen((const char *) (uintptr_t) args[0]);
    case POLY_IMPORT_FUNC_MEMCPY:
      return (uint64_t) (uintptr_t)
        memcpy((void *) (uintptr_t) args[0],
          (const void *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_MEMMOVE:
      return (uint64_t) (uintptr_t)
        memmove((void *) (uintptr_t) args[0],
          (const void *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_MEMSET:
      return (uint64_t) (uintptr_t)
        memset((void *) (uintptr_t) args[0], (int) args[1],
          (size_t) args[2]);
    case POLY_IMPORT_FUNC_MEMCMP:
      return (uint64_t) memcmp((const void *) (uintptr_t) args[0],
        (const void *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_STRCMP:
      return (uint64_t) strcmp((const char *) (uintptr_t) args[0],
        (const char *) (uintptr_t) args[1]);
    case POLY_IMPORT_FUNC_STRNCMP:
      return (uint64_t) strncmp((const char *) (uintptr_t) args[0],
        (const char *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_MEMCHR:
      return (uint64_t) (uintptr_t)
        memchr((const void *) (uintptr_t) args[0], (int) args[1],
          (size_t) args[2]);
    case POLY_IMPORT_FUNC_STRCHR:
      return (uint64_t) (uintptr_t)
        strchr((const char *) (uintptr_t) args[0], (int) args[1]);
    case POLY_IMPORT_FUNC_STRRCHR:
      return (uint64_t) (uintptr_t)
        strrchr((const char *) (uintptr_t) args[0], (int) args[1]);
    case POLY_IMPORT_FUNC_STRSTR:
      return (uint64_t) (uintptr_t)
        strstr((const char *) (uintptr_t) args[0],
          (const char *) (uintptr_t) args[1]);
    case POLY_IMPORT_FUNC_STRCPY:
      return (uint64_t) (uintptr_t)
        strcpy((char *) (uintptr_t) args[0],
          (const char *) (uintptr_t) args[1]);
    case POLY_IMPORT_FUNC_STRNCPY:
      return (uint64_t) (uintptr_t)
        strncpy((char *) (uintptr_t) args[0],
          (const char *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_STRNLEN:
      return strnlen((const char *) (uintptr_t) args[0], (size_t) args[1]);
    case POLY_IMPORT_FUNC_STPCPY:
      return (uint64_t) (uintptr_t)
        polyexec_stpcpy((char *) (uintptr_t) args[0],
          (const char *) (uintptr_t) args[1]);
    case POLY_IMPORT_FUNC_STPNCPY:
      return (uint64_t) (uintptr_t)
        polyexec_stpncpy((char *) (uintptr_t) args[0],
          (const char *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_MEMPCPY:
      return (uint64_t) (uintptr_t)
        polyexec_mempcpy((void *) (uintptr_t) args[0],
          (const void *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_RAWMEMCHR:
      return (uint64_t) (uintptr_t)
        polyexec_rawmemchr((const void *) (uintptr_t) args[0],
          (int) args[1]);
    case POLY_IMPORT_FUNC_STRCHRNUL:
      return (uint64_t) (uintptr_t)
        polyexec_strchrnul((const char *) (uintptr_t) args[0],
          (int) args[1]);
    case POLY_IMPORT_FUNC_MEMRCHR:
      return (uint64_t) (uintptr_t)
        polyexec_memrchr((const void *) (uintptr_t) args[0],
          (int) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_MEMMEM:
      return (uint64_t) (uintptr_t)
        polyexec_memmem((const void *) (uintptr_t) args[0], (size_t) args[1],
          (const void *) (uintptr_t) args[2], (size_t) args[3]);
    case POLY_IMPORT_FUNC_STRCAT:
      return (uint64_t) (uintptr_t)
        strcat((char *) (uintptr_t) args[0],
          (const char *) (uintptr_t) args[1]);
    case POLY_IMPORT_FUNC_STRNCAT:
      return (uint64_t) (uintptr_t)
        strncat((char *) (uintptr_t) args[0],
          (const char *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_STRSPN:
      return strspn((const char *) (uintptr_t) args[0],
        (const char *) (uintptr_t) args[1]);
    case POLY_IMPORT_FUNC_STRCSPN:
      return strcspn((const char *) (uintptr_t) args[0],
        (const char *) (uintptr_t) args[1]);
    case POLY_IMPORT_FUNC_STRPBRK:
      return (uint64_t) (uintptr_t)
        strpbrk((const char *) (uintptr_t) args[0],
          (const char *) (uintptr_t) args[1]);
    case POLY_IMPORT_FUNC_BCMP:
      return (uint64_t) bcmp((const void *) (uintptr_t) args[0],
        (const void *) (uintptr_t) args[1], (size_t) args[2]);
    case POLY_IMPORT_FUNC_BCOPY:
      bcopy((const void *) (uintptr_t) args[0], (void *) (uintptr_t) args[1],
        (size_t) args[2]);
      return 0;
    case POLY_IMPORT_FUNC_BZERO:
      bzero((void *) (uintptr_t) args[0], (size_t) args[1]);
      return 0;
    case POLY_IMPORT_FUNC_ATOI:
      return (uint64_t) (int64_t)
        (int) polyexec_strtoll_c((const char *) (uintptr_t) args[0],
          NULL, 10);
    case POLY_IMPORT_FUNC_ATOL:
      return (uint64_t)
        polyexec_strtoll_c((const char *) (uintptr_t) args[0], NULL, 10);
    case POLY_IMPORT_FUNC_ATOLL:
      return (uint64_t)
        polyexec_strtoll_c((const char *) (uintptr_t) args[0], NULL, 10);
    case POLY_IMPORT_FUNC_STRTOL:
      return (uint64_t) (int64_t)
        polyexec_strtoll_c((const char *) (uintptr_t) args[0],
          (char **) (uintptr_t) args[1], (int) args[2]);
    case POLY_IMPORT_FUNC_STRTOUL:
      return (uint64_t)
        polyexec_strtoull_c((const char *) (uintptr_t) args[0],
          (char **) (uintptr_t) args[1], (int) args[2]);
    case POLY_IMPORT_FUNC_STRTOLL:
      return (uint64_t) (int64_t)
        polyexec_strtoll_c((const char *) (uintptr_t) args[0],
          (char **) (uintptr_t) args[1], (int) args[2]);
    case POLY_IMPORT_FUNC_STRTOULL:
      return (uint64_t)
        polyexec_strtoull_c((const char *) (uintptr_t) args[0],
          (char **) (uintptr_t) args[1], (int) args[2]);
    case POLY_IMPORT_FUNC_ISALNUM:
      return (uint64_t) (polyexec_ascii_isalpha((unsigned char) args[0]) ||
        polyexec_ascii_isdigit((unsigned char) args[0]));
    case POLY_IMPORT_FUNC_ISALPHA:
      return (uint64_t) polyexec_ascii_isalpha((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISDIGIT:
      return (uint64_t) polyexec_ascii_isdigit((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISLOWER:
      return (uint64_t) polyexec_ascii_islower((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISSPACE:
      return (uint64_t) polyexec_ascii_isspace((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISUPPER:
      return (uint64_t) polyexec_ascii_isupper((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISXDIGIT:
      return (uint64_t) polyexec_ascii_isxdigit((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISBLANK:
      return (uint64_t) polyexec_ascii_isblank((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISCNTRL:
      return (uint64_t) polyexec_ascii_iscntrl((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISGRAPH:
      return (uint64_t) polyexec_ascii_isgraph((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISPRINT:
      return (uint64_t) polyexec_ascii_isprint((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ISPUNCT:
      return (uint64_t) polyexec_ascii_ispunct((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_TOLOWER:
      return (uint64_t) tolower((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_TOUPPER:
      return (uint64_t) toupper((unsigned char) args[0]);
    case POLY_IMPORT_FUNC_ABS:
      return (uint64_t) (int64_t) abs((int) args[0]);
    case POLY_IMPORT_FUNC_LABS:
      return (uint64_t) (int64_t) labs((long) args[0]);
    case POLY_IMPORT_FUNC_LLABS:
      return (uint64_t) (int64_t) llabs((long long) args[0]);
    case POLY_IMPORT_FUNC_FFS:
      return (uint64_t) ffs((int) args[0]);
    case POLY_IMPORT_FUNC_FFSL:
      return (uint64_t) ffsl((long) args[0]);
    case POLY_IMPORT_FUNC_FFSLL:
      return (uint64_t) ffsll((long long) args[0]);
    default:
      return (uint64_t) -ENOSYS;
  }
}

static void report_poly_monitor_packets(void);
static void report_poly_syscall_summary(void);

static int poly_trace_syscalls_enabled(void) {
  if (polyexec_trace_syscalls < 0) {
    const char *value = getenv("POLYEXEC_TRACE_SYSCALLS");
    polyexec_trace_syscalls = value != NULL && value[0] != '\0' &&
      strcmp(value, "0") != 0;
  }
  return polyexec_trace_syscalls;
}

static int poly_trace_postgres_syscalls_enabled(void) {
  if (polyexec_trace_postgres_syscalls < 0) {
    const char *value = getenv("POLYEXEC_TRACE_POSTGRES_SYSCALLS");
    polyexec_trace_postgres_syscalls =
      value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
  }
  return polyexec_trace_postgres_syscalls;
}

static int poly_syscall_summary_enabled(void) {
  if (polyexec_syscall_summary < 0) {
    const char *value = getenv("POLYEXEC_SYSCALL_SUMMARY");
    polyexec_syscall_summary =
      value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
  }
  return polyexec_syscall_summary;
}

static int poly_prefault_guest_mmaps_enabled(void) {
  if (polyexec_prefault_guest_mmaps < 0) {
    const char *value = getenv("POLYEXEC_PREFAULT_GUEST_MMAPS");
    polyexec_prefault_guest_mmaps =
      value == NULL || value[0] == '\0' || strcmp(value, "0") != 0;
  }
  return polyexec_prefault_guest_mmaps;
}

static int poly_trace_trap_returns_enabled(void) {
  if (polyexec_trace_trap_returns < 0) {
    const char *value = getenv("POLYEXEC_TRACE_TRAP_RETURNS");
    polyexec_trace_trap_returns = value != NULL && value[0] != '\0' &&
      strcmp(value, "0") != 0;
  }
  return polyexec_trace_trap_returns;
}

static int poly_disable_io_uring_enabled(void) {
  if (polyexec_disable_io_uring < 0) {
    const char *value = getenv("POLYEXEC_DISABLE_IO_URING");
    polyexec_disable_io_uring = value != NULL && value[0] != '\0' &&
      strcmp(value, "0") != 0;
  }
  return polyexec_disable_io_uring;
}

static int poly_disable_rseq_enabled(void) {
  if (polyexec_disable_rseq < 0) {
    const char *value = getenv("POLYEXEC_DISABLE_RSEQ");
    polyexec_disable_rseq = value != NULL && value[0] != '\0' &&
      strcmp(value, "0") != 0;
  }
  return polyexec_disable_rseq;
}

static int poly_stdout_passthrough_enabled(void) {
  if (polyexec_stdout_passthrough < 0) {
    const char *value = getenv("POLYEXEC_STDOUT_PASSTHROUGH");
    polyexec_stdout_passthrough = value != NULL && value[0] != '\0' &&
      strcmp(value, "0") != 0;
  }
  return polyexec_stdout_passthrough;
}

static void poly_redirect_stdout_diagnostics(void) {
  if (!poly_stdout_passthrough_enabled() || polyexec_saved_stdout >= 0)
    return;
  fflush(stdout);
  int saved = dup(STDOUT_FILENO);
  if (saved < 0)
    return;
  if (dup2(STDERR_FILENO, STDOUT_FILENO) < 0) {
    close(saved);
    return;
  }
  polyexec_saved_stdout = saved;
}

static void poly_restore_passthrough_stdout(void) {
  if (polyexec_saved_stdout < 0)
    return;
  fflush(stdout);
  (void) dup2(polyexec_saved_stdout, STDOUT_FILENO);
}

static void poly_resume_stdout_diagnostics(void) {
  if (polyexec_saved_stdout < 0)
    return;
  fflush(stdout);
  (void) dup2(STDERR_FILENO, STDOUT_FILENO);
}

static const char *poly_aarch64_syscall_name(uint64_t number) {
  switch (number) {
    case 17: return "getcwd";
    case 20: return "epoll_create1";
    case 21: return "epoll_ctl";
    case 22: return "epoll_pwait";
    case 23: return "dup";
    case 24: return "dup3";
    case 25: return "fcntl";
    case 26: return "inotify_init1";
    case 29: return "ioctl";
    case 34: return "mkdirat";
    case 35: return "unlinkat";
    case 38: return "renameat";
    case 46: return "ftruncate";
    case 47: return "fallocate";
    case 48: return "faccessat";
    case 49: return "chdir";
    case 53: return "fchmodat";
    case 54: return "fchmod";
    case 55: return "fchownat";
    case 56: return "openat";
    case 57: return "close";
    case 59: return "pipe2";
    case 61: return "getdents64";
    case 62: return "lseek";
    case 63: return "read";
    case 64: return "write";
    case 65: return "readv";
    case 66: return "writev";
    case 67: return "pread64";
    case 68: return "pwrite64";
    case 69: return "preadv";
    case 70: return "pwritev";
    case 72: return "pselect6";
    case 73: return "ppoll";
    case 78: return "readlinkat";
    case 79: return "newfstatat";
    case 80: return "fstat";
    case 81: return "sync";
    case 82: return "fsync";
    case 83: return "fdatasync";
    case 88: return "utimensat";
    case 93: return "exit";
    case 94: return "exit_group";
    case 95: return "waitid";
    case 96: return "set_tid_address";
    case 98: return "futex";
    case 99: return "set_robust_list";
    case 101: return "nanosleep";
    case 102: return "getitimer";
    case 103: return "setitimer";
    case 113: return "clock_gettime";
    case 115: return "clock_nanosleep";
    case 129: return "kill";
    case 130: return "tkill";
    case 131: return "tgkill";
    case 132: return "sigaltstack";
    case 134: return "rt_sigaction";
    case 135: return "rt_sigprocmask";
    case 139: return "rt_sigreturn";
    case 160: return "uname";
    case 165: return "getrusage";
    case 166: return "umask";
    case 167: return "prctl";
    case 168: return "getcpu";
    case 169: return "gettimeofday";
    case 172: return "getpid";
    case 173: return "getppid";
    case 174: return "getuid";
    case 175: return "geteuid";
    case 176: return "getgid";
    case 177: return "getegid";
    case 178: return "gettid";
    case 179: return "sysinfo";
    case 194: return "shmget";
    case 195: return "shmctl";
    case 196: return "shmat";
    case 197: return "shmdt";
    case 198: return "socket";
    case 199: return "socketpair";
    case 200: return "bind";
    case 201: return "listen";
    case 202: return "accept";
    case 203: return "connect";
    case 204: return "getsockname";
    case 205: return "getpeername";
    case 206: return "sendto";
    case 207: return "recvfrom";
    case 208: return "setsockopt";
    case 209: return "getsockopt";
    case 210: return "shutdown";
    case 214: return "brk";
    case 215: return "munmap";
    case 216: return "mremap";
    case 222: return "mmap";
    case 226: return "mprotect";
    case 220: return "clone";
    case 221: return "execve";
    case 278: return "getrandom";
    case 260: return "wait4";
    case 261: return "prlimit64";
    case 281: return "execveat";
    case 293: return "rseq";
    case 424: return "pidfd_send_signal";
    case 425: return "io_uring_setup";
    case 426: return "io_uring_enter";
    case 427: return "io_uring_register";
    case 435: return "clone3";
    default: return "-";
  }
}

static void poly_record_syscall_result(
    const struct poly_runtime_trap_packet *packet, long x86_number,
    uint64_t result) {
  if (!poly_syscall_summary_enabled() || packet == NULL ||
      packet->reason != POLY_TRAP_SYSCALL)
    return;
  __sync_fetch_and_add(&poly_syscall_summary_total, 1);
  if (packet->mode == POLY_MODE_RAW_AARCH64 &&
      packet->number < POLYEXEC_SYSCALL_SUMMARY_MAX) {
    __sync_fetch_and_add(&poly_syscall_summary_aarch64[packet->number], 1);
    if ((int64_t) result < 0) {
      __sync_fetch_and_add(
        &poly_syscall_summary_error_aarch64[packet->number], 1);
    }
  }
  if (x86_number >= 0 && x86_number < POLYEXEC_SYSCALL_SUMMARY_MAX)
    __sync_fetch_and_add(&poly_syscall_summary_x86[x86_number], 1);
  if ((int64_t) result < 0)
    __sync_fetch_and_add(&poly_syscall_summary_errors, 1);
}

static int poly_trace_protected_signal_wait_syscall(
    const struct poly_runtime_trap_packet *packet, long x86_number) {
  if (!poly_protect_runtime_signals_enabled() ||
      !poly_trace_protected_signal_waits_enabled() ||
      packet->mode != POLY_MODE_RAW_AARCH64)
    return 0;
  switch (packet->number) {
    case 22:
    case 72:
    case 73:
    case 74:
    case 95:
    case 98:
    case 101:
    case 115:
    case 130:
    case 131:
    case 133:
    case 134:
    case 135:
    case 136:
    case 260:
    case 424:
    case 441:
    case 449:
    case 454:
    case 455:
    case 456:
      return 1;
    default:
      break;
  }
  return x86_number == SYS_kill || x86_number == SYS_tkill ||
    x86_number == SYS_tgkill || x86_number == SYS_pidfd_send_signal ||
    x86_number == SYS_rt_sigaction || x86_number == SYS_rt_sigprocmask;
}

static void poly_trace_syscall_result(
    const struct poly_runtime_trap_packet *packet, const char *path,
    long x86_number, uint64_t result) {
  poly_record_syscall_result(packet, x86_number, result);
  poly_note_syscall_fd_effect(packet, x86_number, result);
  const int trace_postgres =
    poly_trace_postgres_syscalls_enabled() && process_cross_report_path != NULL &&
    (strcmp(process_cross_report_path, "/usr/libexec/postgresql18/postgres") == 0 ||
     strcmp(process_cross_report_path, "/usr/libexec/postgresql18/postgres.polyreal") == 0);
  if (!poly_trace_syscalls_enabled() &&
      !poly_trace_protected_signal_wait_syscall(packet, x86_number) &&
      !trace_postgres)
    return;
  const char *open_path = NULL;
  const char *stat_path = NULL;
  uint64_t translated_open_flags = 0;
  if (packet->number == 56 && packet->args[1] != 0) {
    open_path = (const char *) (uintptr_t) packet->args[1];
    translated_open_flags =
      poly_translate_open_flags(packet->args[2], packet->mode,
        packet->args[0], packet->args[1]);
  }
  if (packet->number == 79 && packet->args[1] != 0)
    stat_path = (const char *) (uintptr_t) packet->args[1];
  fprintf(stderr,
    "POLYEXEC_SYSCALL: path=%s mode=%llu nr=%llu x86=%ld result=%lld pc=0x%llx next=0x%llx flags=0x%llx args=0x%llx,0x%llx,0x%llx,0x%llx,0x%llx,0x%llx",
    path,
    (unsigned long long) packet->mode,
    (unsigned long long) packet->number,
    x86_number,
    (long long) result,
    (unsigned long long) packet->pc,
    (unsigned long long) packet->next_pc,
    (unsigned long long) packet->flags,
    (unsigned long long) packet->args[0],
    (unsigned long long) packet->args[1],
    (unsigned long long) packet->args[2],
    (unsigned long long) packet->args[3],
    (unsigned long long) packet->args[4],
    (unsigned long long) packet->args[5]);
  if (open_path != NULL)
    fprintf(stderr, " open_path=%s open_flags=0x%llx translated_flags=0x%llx",
      open_path,
      (unsigned long long) packet->args[2],
      (unsigned long long) translated_open_flags);
  if (stat_path != NULL)
    fprintf(stderr, " stat_path=%s", stat_path);
  if (trace_postgres && process_cross_report_path != NULL)
    fprintf(stderr, " program=%s", process_cross_report_path);
  fputc('\n', stderr);
}

static int poly_path_is_aarch64_elf(const char *path) {
  if (path == NULL || path[0] == '\0')
    return 0;
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return 0;
  Elf64_Ehdr ehdr;
  ssize_t n = read(fd, &ehdr, sizeof(ehdr));
  close(fd);
  if (n != (ssize_t) sizeof(ehdr))
    return 0;
  return memcmp(ehdr.e_ident, ELFMAG, SELFMAG) == 0 &&
    ehdr.e_ident[EI_CLASS] == ELFCLASS64 &&
    ehdr.e_ident[EI_DATA] == ELFDATA2LSB &&
    ehdr.e_machine == EM_AARCH64;
}

static int poly_count_guest_argv(uint64_t argv_addr, size_t *argc_out) {
  if (argc_out == NULL)
    return -1;
  *argc_out = 0;
  if (argv_addr == 0)
    return -1;
  const uint64_t *argv = (const uint64_t *) (uintptr_t) argv_addr;
  const size_t max_argc = 4096;
  for (size_t n = 0; n < max_argc; n++) {
    uint64_t value = 0;
    memcpy(&value, argv + n, sizeof(value));
    if (value == 0) {
      *argc_out = n;
      return 0;
    }
  }
  errno = E2BIG;
  return -1;
}

static int poly_handle_raw_aarch64_execve(uint64_t path_addr,
    uint64_t argv_addr, uint64_t envp_addr, uint64_t *result_out) {
  if (result_out == NULL)
    return 0;
  const char *path = (const char *) (uintptr_t) path_addr;
  if (!poly_path_is_aarch64_elf(path))
    return 0;

  size_t old_argc = 0;
  if (poly_count_guest_argv(argv_addr, &old_argc) < 0) {
    *result_out = (uint64_t) -errno;
    return 1;
  }

  const uint64_t *old_argv = (const uint64_t *) (uintptr_t) argv_addr;
  const size_t extra_argc = old_argc > 0 ? old_argc - 1 : 0;
  char **new_argv = __builtin_alloca((extra_argc + 4) * sizeof(*new_argv));
  new_argv[0] = (char *) "/usr/bin/polyexec";
  new_argv[1] = (char *) "--process";
  new_argv[2] = (char *) path;
  for (size_t n = 0; n < extra_argc; n++)
    new_argv[n + 3] = (char *) (uintptr_t) old_argv[n + 1];
  new_argv[extra_argc + 3] = NULL;

  char *const *old_envp = envp_addr != 0 ?
    (char *const *) (uintptr_t) envp_addr : environ;
  size_t envc = 0;
  const size_t max_envc = 4096;
  while (old_envp[envc] != NULL) {
    if (envc >= max_envc) {
      *result_out = (uint64_t) -E2BIG;
      return 1;
    }
    envc++;
  }
  char **new_envp = __builtin_alloca((envc + 2) * sizeof(*new_envp));
  for (size_t n = 0; n < envc; n++)
    new_envp[n] = old_envp[n];
  new_envp[envc] = (char *) "POLYEXEC_STDOUT_PASSTHROUGH=1";
  new_envp[envc + 1] = NULL;

  execve(new_argv[0], new_argv, new_envp);
  const int saved_errno = errno;
  *result_out = (uint64_t) -saved_errno;
  return 1;
}

static void poly_trace_trap_return_result(
    const struct poly_runtime_trap_packet *packet,
    const struct poly_xsave_state *trap_state, int has_trap_state,
    uint64_t result) {
  if (!poly_trace_trap_returns_enabled())
    return;
  const struct poly_trap_restore_state *restore =
    has_trap_state && trap_state != NULL ? &trap_state->trap_restore : NULL;
  fprintf(stderr,
    "POLYEXEC_TRAP_RETURN: reason=%llu mode=%llu nr=%llu result=%lld pc=0x%llx next=0x%llx flags=0x%llx has_restore=%d",
    (unsigned long long) packet->reason,
    (unsigned long long) packet->mode,
    (unsigned long long) packet->number,
    (long long) result,
    (unsigned long long) packet->pc,
    (unsigned long long) packet->next_pc,
    (unsigned long long) packet->flags,
    has_trap_state);
  if (restore != NULL) {
    fprintf(stderr,
      " restore_flags=0x%llx restore_mode=%u restore_rsp=0x%llx a64_mask=0x%llx a64_x0=0x%llx a64_x1=0x%llx a64_x2=0x%llx a64_x3=0x%llx a64_x7=0x%llx a64_x8=0x%llx a64_x29=0x%llx a64_x30=0x%llx a64_nzcv=0x%llx a64_fpcr=0x%llx a64_fpsr=0x%llx rv_mask=0x%llx rv_a0=0x%llx rv_a7=0x%llx",
      (unsigned long long) restore->flags,
      restore->mode,
      (unsigned long long) restore->x86_gpr[15],
      (unsigned long long) restore->aarch64_gpr_valid_mask,
      (unsigned long long) restore->aarch64_gpr[0],
      (unsigned long long) restore->aarch64_gpr[1],
      (unsigned long long) restore->aarch64_gpr[2],
      (unsigned long long) restore->aarch64_gpr[3],
      (unsigned long long) restore->aarch64_gpr[7],
      (unsigned long long) restore->aarch64_gpr[8],
      (unsigned long long) restore->aarch64_gpr[29],
      (unsigned long long) restore->aarch64_gpr[30],
      (unsigned long long) restore->aarch64_nzcv,
      (unsigned long long) restore->aarch64_fpcr,
      (unsigned long long) restore->aarch64_fpsr,
      (unsigned long long) restore->riscv_gpr_valid_mask,
      (unsigned long long) restore->riscv_gpr[10],
      (unsigned long long) restore->riscv_gpr[17]);
  }
  fputc('\n', stderr);
}

static void poly_sanitize_trap_state_for_import(struct poly_xsave_state *state) {
  state->trap.reserved[0] = 0;
  state->trap.reserved[1] = 0;
  if (state->trap.reason == 0)
    memset(state->trap_args, 0, sizeof(state->trap_args));
  memset(state->pre_trap_restore_reserved, 0,
    sizeof(state->pre_trap_restore_reserved));
  memset(state->reserved, 0, sizeof(state->reserved));
  memset(state->state_key.reserved, 0, sizeof(state->state_key.reserved));
  if (state->import_return.depth != POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH ||
      state->import_return.top > POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH) {
    state->import_return.top = 0;
    state->import_return.depth = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH;
    memset(state->import_return.frames, 0,
      sizeof(state->import_return.frames));
  } else {
    for (uint64_t n = state->import_return.top;
         n < POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH; n++)
      memset(&state->import_return.frames[n], 0,
        sizeof(state->import_return.frames[n]));
  }
  memset(state->import_return.reserved, 0,
    sizeof(state->import_return.reserved));
  if (state->cross_return.depth != POLY_STATE_XSAVE_CROSS_RETURN_DEPTH ||
      state->cross_return.top > POLY_STATE_XSAVE_CROSS_RETURN_DEPTH) {
    state->cross_return.top = 0;
    state->cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
    memset(state->cross_return.frames, 0,
      sizeof(state->cross_return.frames));
  } else {
    for (uint64_t n = 0; n < state->cross_return.top; n++)
      state->cross_return.frames[n].reserved0 = 0;
    for (uint64_t n = state->cross_return.top;
         n < POLY_STATE_XSAVE_CROSS_RETURN_DEPTH; n++)
      memset(&state->cross_return.frames[n], 0,
        sizeof(state->cross_return.frames[n]));
  }
  memset(state->cross_return.reserved, 0,
    sizeof(state->cross_return.reserved));
  if (state->native_return.active_valid == 0)
    memset(&state->native_return.active, 0, sizeof(state->native_return.active));
  else
    memset(state->native_return.active.reserved, 0,
      sizeof(state->native_return.active.reserved));
  if (state->native_return.top <= POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH) {
    for (uint64_t n = 0; n < state->native_return.top; n++)
      memset(state->native_return.frames[n].reserved, 0,
        sizeof(state->native_return.frames[n].reserved));
    for (uint64_t n = state->native_return.top;
         n < POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH; n++)
      memset(&state->native_return.frames[n], 0,
        sizeof(state->native_return.frames[n]));
  }
  memset(state->native_return.reserved, 0,
    sizeof(state->native_return.reserved));
  if (state->trap_restore.flags == 0) {
    memset(&state->trap_restore, 0, sizeof(state->trap_restore));
    return;
  }
  state->trap_restore.reserved0 = 0;
  state->trap_restore.aarch64_reserved = 0;
  memset(state->trap_restore.riscv_reserved, 0,
    sizeof(state->trap_restore.riscv_reserved));
  memset(state->trap_restore.reserved, 0,
    sizeof(state->trap_restore.reserved));
  if (state->trap_restore.mode == POLY_MODE_RAW_AARCH64)
    poly_sanitize_aarch64_trap_restore_payload(&state->trap_restore);
  else if (state->trap_restore.mode == POLY_MODE_RAW_RISCV)
    poly_sanitize_riscv_trap_restore_payload(&state->trap_restore);
}

static void poly_set_clone_child_trap_packet_state(
    const struct poly_runtime_trap_packet *packet,
    struct poly_xsave_state *trap_state);

static int poly_aarch64_signal_delivery_uses_packet_resume_pc(
    const struct poly_runtime_trap_packet *packet) {
  if (packet == NULL || packet->reason != POLY_TRAP_SYSCALL ||
      packet->mode != POLY_MODE_RAW_AARCH64)
    return 0;
  switch (packet->number) {
    case 22:  /* epoll_pwait */
    case 72:  /* pselect6 */
    case 73:  /* ppoll */
    case 98:  /* futex */
    case 101: /* nanosleep */
    case 115: /* clock_nanosleep */
    case 260: /* wait4 */
    case 441: /* epoll_pwait2 */
      return 1;
    default:
      return 0;
  }
}

static int poly_aarch64_rt_sigreturn_packet(
    const struct poly_runtime_trap_packet *packet) {
  return packet != NULL && packet->reason == POLY_TRAP_SYSCALL &&
    packet->mode == POLY_MODE_RAW_AARCH64 && packet->number == 139;
}

static uint64_t poly_trap_vector_return_result(uint64_t result,
    const struct poly_runtime_trap_packet *packet,
    const struct poly_xsave_state *trap_state, int has_trap_state) {
  volatile uint64_t saved_result = result;
  struct poly_xsave_state *return_state = &poly_trap_return_state;
  const struct poly_xsave_state *import_state = trap_state;
  int delivered_guest_signal = 0;
  if (has_trap_state && trap_state != NULL) {
    const int deliver_guest_illegal =
      packet->reason == POLY_TRAP_ILLEGAL &&
      packet->mode == POLY_MODE_RAW_AARCH64;
    const int aarch64_rt_sigreturn =
      poly_aarch64_rt_sigreturn_packet(packet);
    uint64_t rt_sigreturn_resume_pc = 0;
    if (aarch64_rt_sigreturn)
      rt_sigreturn_resume_pc = trap_state->trap.resume_pc != 0 ?
        trap_state->trap.resume_pc : trap_state->header.foreign_pc;
    memcpy(return_state, trap_state, sizeof(*return_state));
    poly_set_clone_child_trap_packet_state(packet, return_state);
    if (aarch64_rt_sigreturn && rt_sigreturn_resume_pc != 0) {
      return_state->trap.trap_pc = rt_sigreturn_resume_pc;
      return_state->trap.resume_pc = rt_sigreturn_resume_pc;
      return_state->header.foreign_pc = rt_sigreturn_resume_pc;
    }
    if (packet->mode == POLY_MODE_RAW_AARCH64) {
      if (deliver_guest_illegal) {
        return_state->header.foreign_pc = packet->pc;
      } else if (poly_aarch64_signal_delivery_uses_packet_resume_pc(packet)) {
        uint64_t foreign_pc = packet->next_pc != 0 ? packet->next_pc : packet->pc;
        return_state->header.foreign_pc = foreign_pc;
      }
    }
    if (packet->mode == POLY_MODE_RAW_AARCH64 && !deliver_guest_illegal) {
      return_state->aarch64_gpr[0] = result;
      return_state->trap_restore.aarch64_gpr[0] = result;
    }
    else if (packet->mode == POLY_MODE_RAW_RISCV) {
      return_state->riscv_gpr[10] = result;
      return_state->trap_restore.riscv_gpr[10] = result;
    }
    import_state = return_state;
    if (deliver_guest_illegal &&
        poly_try_deliver_aarch64_signal(return_state, SIGILL, packet->pc,
          ILL_ILLOPC)) {
      delivered_guest_signal = 1;
      saved_result = return_state->trap_restore.aarch64_gpr[0];
    }
    else if (packet->mode == POLY_MODE_RAW_AARCH64 &&
        poly_try_deliver_aarch64_signal(return_state, 0, 0, 0)) {
      delivered_guest_signal = 1;
      saved_result = return_state->trap_restore.aarch64_gpr[0];
    }
  }
  poly_trace_trap_return_result(packet, import_state, has_trap_state, result);
  if (polyexec_use_auto_spill)
    (void) refresh_poly_auto_spill();
  if (poly_trap_vector_active) {
    poly_monitor_packet_set_value((uint64_t) (uintptr_t) poly_monitor_packet);
    poly_trap_vector_mode_set_value(POLY_MODE_X86);
    poly_trap_vector_set_value((uint64_t) (void *) poly_trap_vector_handler);
  }
  if (has_trap_state && import_state != NULL) {
    if (packet->reason == POLY_TRAP_SYSCALL &&
        packet->mode == POLY_MODE_RAW_AARCH64) {
      return_state->aarch64_gpr[31] = poly_current_x86_rsp();
      poly_clear_native_return_state(return_state);
    }
    poly_sanitize_trap_state_for_import(return_state);
    poly_state_import(return_state);
  }
  return saved_result;
}

static void poly_set_clone_child_frontend_state(uint64_t mode,
    uint64_t child_stack, uint64_t tls_base,
    struct poly_xsave_state *trap_state, int has_trap_state) {
  if (!has_trap_state)
    return;
  if (mode == POLY_MODE_RAW_AARCH64) {
    if (child_stack != 0) {
      trap_state->aarch64_gpr[31] = child_stack;
      trap_state->trap_restore.aarch64_gpr[31] = child_stack;
    }
    if (tls_base != 0) {
      trap_state->frontend_tls.flags = 1;
      trap_state->frontend_tls.active_mode = mode;
      trap_state->frontend_tls.aarch64_tls_base = tls_base;
    }
  }
  else if (mode == POLY_MODE_RAW_RISCV) {
    if (child_stack != 0) {
      trap_state->riscv_gpr[2] = child_stack;
      trap_state->trap_restore.riscv_gpr[2] = child_stack;
    }
    if (tls_base != 0) {
      trap_state->frontend_tls.flags = 1;
      trap_state->frontend_tls.active_mode = mode;
      trap_state->frontend_tls.riscv_tls_base = tls_base;
    }
  }
  else {
    return;
  }
}

static void poly_set_clone_child_trap_packet_state(
    const struct poly_runtime_trap_packet *packet,
    struct poly_xsave_state *trap_state) {
  trap_state->trap.reason = (uint32_t) packet->reason;
  trap_state->trap.source_mode = (uint32_t) packet->mode;
  trap_state->trap.number = packet->number;
  trap_state->trap.selector = packet->selector;
  trap_state->trap.trap_pc = packet->pc;
  trap_state->trap.resume_pc = packet->next_pc;
  trap_state->trap.flags = packet->flags;
  trap_state->trap.reserved[0] = 0;
  trap_state->trap.reserved[1] = 0;
  for (size_t n = 0; n < POLY_TRAP_PACKET_ARG_COUNT; n++)
    trap_state->trap_args[n] = packet->args[n];
}

static void poly_clear_native_return_state(
    struct poly_xsave_state *state) {
  state->native_return.active_valid = 0;
  state->native_return.top = 0;
  state->native_return.depth = POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH;
  state->native_return.supported_flags =
    POLY_NATIVE_RETURN_FRAME_FLAGS_SUPPORTED;
  memset(&state->native_return.active, 0,
    sizeof(state->native_return.active));
  memset(state->native_return.frames, 0,
    sizeof(state->native_return.frames));
  memset(state->native_return.reserved, 0,
    sizeof(state->native_return.reserved));
}

static void poly_clear_return_transition_state(
    struct poly_xsave_state *state) {
  memset(&state->transition, 0, sizeof(state->transition));
  state->import_return.top = 0;
  state->import_return.depth = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH;
  memset(state->import_return.frames, 0, sizeof(state->import_return.frames));
  memset(state->import_return.reserved, 0, sizeof(state->import_return.reserved));
  state->cross_return.top = 0;
  state->cross_return.depth = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
  memset(state->cross_return.frames, 0, sizeof(state->cross_return.frames));
  memset(state->cross_return.reserved, 0, sizeof(state->cross_return.reserved));
  poly_clear_native_return_state(state);
}

#define POLY_CLONE_MONITOR_STACK_SIZE (256U * 1024U)
#define POLY_CLONE_VFORK_FOREIGN_STACK_SIZE (256U * 1024U)
#define POLY_CLONE_NATIVE_TLS_BEFORE (256U * 1024U)
#define POLY_CLONE_NATIVE_TLS_AFTER (256U * 1024U)
#define POLY_CLONE_NATIVE_TLS_PAGE 4096U

static int poly_prepare_vfork_foreign_stack(uint64_t original_stack,
    uint64_t *child_stack_out) {
  if (!poly_guest_range_is_mapped(original_stack, 16, 0))
    return -1;
  void *mapping = mmap(NULL, POLY_CLONE_VFORK_FOREIGN_STACK_SIZE,
    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED)
    return -1;
  uintptr_t stack_top = (uintptr_t) mapping + POLY_CLONE_VFORK_FOREIGN_STACK_SIZE;
  stack_top &= ~(uintptr_t) 0xf;
  uint64_t child_stack = (uint64_t) (uintptr_t) (stack_top - 16);
  memcpy((void *) (uintptr_t) child_stack,
    (const void *) (uintptr_t) original_stack, 16);
  *child_stack_out = child_stack;
  return 0;
}

static int poly_prepare_clone_native_tls(
    struct poly_clone_child_handoff *handoff) {
  const uint64_t parent_fs = get_x86_fs_base();
  if (parent_fs == 0)
    return 0;

  const size_t mapping_size =
    POLY_CLONE_NATIVE_TLS_BEFORE + POLY_CLONE_NATIVE_TLS_AFTER;
  uint8_t *mapping = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED)
    return -1;

  memset(mapping, 0, mapping_size);
  const uint64_t source_base =
    parent_fs >= POLY_CLONE_NATIVE_TLS_BEFORE ?
      parent_fs - POLY_CLONE_NATIVE_TLS_BEFORE : 0;
  const uint64_t source_end = source_base + mapping_size;
  for (size_t offset = 0; offset < mapping_size;
       offset += POLY_CLONE_NATIVE_TLS_PAGE) {
    const uint64_t source = source_base + (uint64_t) offset;
    if (source_base == 0 || source < source_base)
      continue;
    if (poly_guest_range_is_mapped(source, POLY_CLONE_NATIVE_TLS_PAGE, 0))
      memcpy(mapping + offset, (const void *) (uintptr_t) source,
        POLY_CLONE_NATIVE_TLS_PAGE);
  }

  handoff->native_tls_mapping = mapping;
  handoff->native_tls_mapping_size = mapping_size;
  handoff->native_tls_base =
    (uint64_t) (uintptr_t) (mapping + POLY_CLONE_NATIVE_TLS_BEFORE);

  uint64_t *tcb = (uint64_t *) (uintptr_t) handoff->native_tls_base;
  tcb[0] = handoff->native_tls_base;
  if (tcb[1] >= source_base && tcb[1] < source_end)
    tcb[1] = (uint64_t) (uintptr_t) mapping + (tcb[1] - source_base);
  tcb[2] = handoff->native_tls_base;
  return 0;
}

static int poly_prepare_clone_child_handoff(
    const struct poly_runtime_trap_packet *packet,
    const struct poly_xsave_state *trap_state,
    uint64_t foreign_stack, uint64_t foreign_tls,
    struct poly_clone_child_handoff **handoff_out,
    uint64_t *native_child_stack_out) {
  const size_t mapping_size = sizeof(struct poly_clone_child_handoff) +
    POLY_CLONE_MONITOR_STACK_SIZE + 64;
  struct poly_clone_child_handoff *handoff = mmap(NULL, mapping_size,
    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (handoff == MAP_FAILED)
    return -1;

  memset(handoff, 0, sizeof(*handoff));
  const char *native_tls_env = getenv("POLYEXEC_CLONE_NATIVE_TLS");
  if (native_tls_env == NULL || strcmp(native_tls_env, "0") != 0) {
    if (poly_prepare_clone_native_tls(handoff) < 0) {
      munmap(handoff, mapping_size);
      return -1;
    }
  }
  handoff->packet = *packet;
  memcpy(&handoff->state, trap_state, sizeof(handoff->state));
  poly_set_clone_child_trap_packet_state(packet, &handoff->state);
  poly_set_clone_child_frontend_state(packet->mode, foreign_stack,
    foreign_tls, &handoff->state, 1);
  handoff->state.header.current_mode = (uint32_t) packet->mode;
  handoff->state.header.foreign_pc = packet->next_pc;
  if (packet->mode == POLY_MODE_RAW_AARCH64) {
    handoff->state.aarch64_gpr[0] = 0;
    handoff->state.trap_restore.aarch64_gpr[0] = 0;
    handoff->state.trap_restore.aarch64_gpr_valid_mask =
      POLY_AARCH64_GPR_VALID_MASK;
    poly_clear_native_return_state(&handoff->state);
  }
  if (packet->mode == POLY_MODE_RAW_AARCH64)
    handoff->state.header.foreign_tls_base = foreign_tls;
  handoff->state.header.monitor_packet_addr =
    (uint64_t) (uintptr_t) handoff->monitor_packet;
  if (polyexec_use_explicit_state_key) {
    handoff->state.state_key.flags = POLY_STATE_KEY_FLAG_EXPLICIT;
    handoff->state.state_key.explicit_key =
      (uint64_t) (uintptr_t) &handoff->state_key_anchor;
    handoff->state.state_key.supported_flags = POLY_STATE_KEY_FLAG_EXPLICIT;
  }
  handoff->mapping = handoff;
  handoff->mapping_size = mapping_size;
  handoff->foreign_stack = foreign_stack;
  handoff->foreign_tls = foreign_tls;

  uintptr_t stack_top = (uintptr_t) handoff + mapping_size;
  stack_top &= ~(uintptr_t) 0xf;
  uint64_t *handoff_slot = (uint64_t *) (stack_top - sizeof(uint64_t));
  *handoff_slot = (uint64_t) (uintptr_t) handoff;
  if ((handoff->state.trap_restore.flags & POLY_TRAP_RESTORE_FLAG_VALID) != 0)
    handoff->state.trap_restore.x86_gpr[15] =
      (uint64_t) stack_top;
  *handoff_out = handoff;
  *native_child_stack_out = (uint64_t) (uintptr_t) handoff_slot;
  return 0;
}

__attribute__((noreturn, noinline, used))
static void poly_clone_child_entry(struct poly_clone_child_handoff *handoff) {
  if (handoff == NULL)
    poly_x86_exit_group_now(127);
  poly_auto_spill_installed = 0;
  if (polyexec_use_explicit_state_key) {
    const uint64_t key = (uint64_t) (uintptr_t) &handoff->state_key_anchor;
    if (key == 0 || poly_state_key_set(key) != 0 ||
        poly_state_key_get() != key)
      poly_x86_exit_group_now(126);
  }
  if (polyexec_use_auto_spill) {
    poly_auto_spill_state_active = &handoff->state;
    poly_auto_spill_installed = 1;
    if (poly_install_auto_spill_signal_actions() < 0)
      poly_x86_exit_group_now(125);
    if (refresh_poly_auto_spill() < 0)
      poly_x86_exit_group_now(125);
  }
  if (poly_trace_syscalls_enabled()) {
    fprintf(stderr,
      "POLYEXEC_CLONE_CHILD_ENTRY: mode=%llu nr=%llu pc=0x%llx next=0x%llx foreign_stack=0x%llx foreign_tls=0x%llx native_tls=0x%llx state_key=0x%llx auto_spill=%d\n",
      (unsigned long long) handoff->packet.mode,
      (unsigned long long) handoff->packet.number,
      (unsigned long long) handoff->packet.pc,
      (unsigned long long) handoff->packet.next_pc,
      (unsigned long long) handoff->foreign_stack,
      (unsigned long long) handoff->foreign_tls,
      (unsigned long long) handoff->native_tls_base,
      (unsigned long long) handoff->state.state_key.explicit_key,
      polyexec_use_auto_spill);
  }
  poly_monitor_packet_set_value(
    (uint64_t) (uintptr_t) handoff->monitor_packet);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value((uint64_t) (void *) poly_trap_vector_handler);
  if (handoff->packet.mode == POLY_MODE_RAW_AARCH64 &&
      handoff->packet.number == 220) {
    handoff->state.trap_restore.x86_gpr[15] = handoff->foreign_stack;
    poly_sanitize_trap_state_for_import(&handoff->state);
    poly_state_import(&handoff->state);
  } else if (handoff->state.header.foreign_pc != handoff->packet.next_pc) {
    poly_sanitize_trap_state_for_import(&handoff->state);
    poly_state_import(&handoff->state);
  } else {
    (void) poly_trap_vector_return_result(0, &handoff->packet,
      &handoff->state, 1);
  }
  if (poly_trace_syscalls_enabled()) {
    fprintf(stderr,
      "POLYEXEC_CLONE_CHILD_TRAP_RETURN: mode=%llu pc=0x%llx sp=0x%llx x0=0x%llx restore_rsp=0x%llx\n",
      (unsigned long long) handoff->packet.mode,
      (unsigned long long) handoff->state.header.foreign_pc,
      (unsigned long long) handoff->state.aarch64_gpr[31],
      (unsigned long long) handoff->state.aarch64_gpr[0],
      (unsigned long long) handoff->state.trap_restore.x86_gpr[15]);
  }
  asm volatile(POLY_OP_TRAP_RETURN ::: "memory");
  __builtin_unreachable();
}

__attribute__((noinline, used))
uint64_t poly_trap_vector_dispatch(void) {
  struct poly_runtime_trap_packet packet;
  struct poly_v2_event_frame event;
  struct poly_xsave_state trap_state __attribute__((aligned(64)));
  int has_trap_state = 0;
  int has_v2_event = 0;

  if (read_poly_monitor_packet(&packet) < 0)
    return (uint64_t) -EIO;
  has_v2_event = read_poly_v2_event_frame(&event);
  if (has_v2_event < 0)
    return (uint64_t) -EIO;
  if (has_v2_event > 0 &&
      apply_poly_v2_event_to_packet(&event, &packet) < 0)
    return (uint64_t) -EIO;
  if ((packet.flags & POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE) != 0) {
    poly_state_export(&trap_state);
    has_trap_state = 1;
  }
  poly_monitor_packet_count++;
  if (packet.reason == POLY_TRAP_SYSCALL &&
      packet.mode == POLY_MODE_RAW_AARCH64)
    poly_monitor_packet_syscall_aarch64_count++;
  else if (packet.reason == POLY_TRAP_SYSCALL &&
      packet.mode == POLY_MODE_RAW_RISCV)
    poly_monitor_packet_syscall_riscv_count++;
  else if (packet.reason == POLY_TRAP_BREAK &&
      packet.mode == POLY_MODE_RAW_AARCH64)
    poly_monitor_packet_break_aarch64_count++;
  else if (packet.reason == POLY_TRAP_BREAK &&
      packet.mode == POLY_MODE_RAW_RISCV)
    poly_monitor_packet_break_riscv_count++;
  else if (packet.reason == POLY_TRAP_IMPORT)
    poly_monitor_packet_import_count++;
  else if (packet.reason == POLY_TRAP_ILLEGAL)
    poly_monitor_packet_illegal_count++;
  else
    poly_monitor_packet_other_count++;

  if (!poly_is_raw_foreign_mode(packet.mode))
    return poly_trap_vector_return_result((uint64_t) -ENOSYS,
      &packet, &trap_state, has_trap_state);

  if (packet.reason == POLY_TRAP_SYSCALL) {
    if (packet.mode == POLY_MODE_RAW_AARCH64 && packet.number == 139) {
      uint64_t sigreturn_result = 0;
      if (poly_handle_aarch64_rt_sigreturn(&trap_state, has_trap_state,
            &sigreturn_result)) {
        poly_trace_syscall_result(&packet, "aarch64-rt-sigreturn", -1,
          sigreturn_result);
        return poly_trap_vector_return_result(sigreturn_result,
          &packet, &trap_state, has_trap_state);
      }
    }

    uint64_t structured_result = 0;
    if (poly_handle_structured_foreign_syscall(packet.number, packet.mode,
          packet.args[0], packet.args[1], packet.args[2], packet.args[3],
          packet.args[4], packet.args[5], &structured_result)) {
      poly_trace_syscall_result(&packet, "structured", -1,
        structured_result);
      return poly_trap_vector_return_result(structured_result,
        &packet, &trap_state, has_trap_state);
    }

    long x86_number = -1;
    if (!poly_generic_linux_syscall_to_x86(packet.number, &x86_number)) {
      if (packet.number == 240 && poly_trace_syscalls_enabled()) {
        fprintf(stderr,
          "POLYEXEC_SIGNAL_SYSCALL_UNMAPPED: nr=%llu mode=%llu args=0x%llx,0x%llx,0x%llx,0x%llx\n",
          (unsigned long long) packet.number,
          (unsigned long long) packet.mode,
          (unsigned long long) packet.args[0],
          (unsigned long long) packet.args[1],
          (unsigned long long) packet.args[2],
          (unsigned long long) packet.args[3]);
      }
      poly_record_syscall_result(&packet, x86_number, (uint64_t) -ENOSYS);
      return poly_trap_vector_return_result((uint64_t) -ENOSYS,
        &packet, &trap_state, has_trap_state);
    }
    if (packet.mode == POLY_MODE_RAW_AARCH64 && x86_number == SYS_execve) {
      uint64_t exec_result = 0;
      if (poly_handle_raw_aarch64_execve(packet.args[0], packet.args[1],
            packet.args[2], &exec_result)) {
        poly_trace_syscall_result(&packet, "aarch64-execve-polyexec",
          x86_number, exec_result);
        return poly_trap_vector_return_result(exec_result, &packet,
          &trap_state, has_trap_state);
      }
    }
    if (poly_trace_syscalls_enabled() &&
        (x86_number == SYS_kill || x86_number == SYS_tkill ||
         x86_number == SYS_tgkill ||
         x86_number == SYS_pidfd_send_signal)) {
      fprintf(stderr,
        "POLYEXEC_SIGNAL_SYSCALL: foreign_nr=%llu x86_nr=%ld mode=%llu args=0x%llx,0x%llx,0x%llx,0x%llx\n",
        (unsigned long long) packet.number, x86_number,
        (unsigned long long) packet.mode,
        (unsigned long long) packet.args[0],
        (unsigned long long) packet.args[1],
        (unsigned long long) packet.args[2],
        (unsigned long long) packet.args[3]);
    }
    if (x86_number == SYS_exit) {
      long pid = poly_x86_syscall6(SYS_getpid, 0, 0, 0, 0, 0, 0);
      long tid = poly_x86_syscall6(SYS_gettid, 0, 0, 0, 0, 0, 0);
      if (pid != tid)
        poly_x86_exit_thread_now(packet.args[0]);
    }
    if (x86_number == SYS_exit || x86_number == SYS_exit_group) {
      poly_record_syscall_result(&packet, x86_number, 0);
      poly_process_terminal_exit_code = packet.args[0];
      if (run_process_exit_finalizers() < 0)
        poly_process_terminal_exit_code = 125;
      poly_resume_stdout_diagnostics();
      report_poly_monitor_packets();
      report_poly_syscall_summary();
      if (poly_process_exit_finalizers.program != NULL) {
        printf("POLYEXEC_PROCESS_EXIT: arch=%s code=%llu path=%s\n",
          poly_process_exit_finalizers.program->arch_name,
          (unsigned long long) poly_process_terminal_exit_code,
          poly_process_exit_finalizers.program->path);
      }
      /*
       * Leave by raw exit_group immediately after terminal guest exit.  At
       * this point process-mode state may reflect foreign signal/TLS changes;
       * running broad libc flush machinery has caused host-side faults in
       * multi-process workloads such as PostgreSQL postmaster children.
       */
      poly_x86_exit_group_now(poly_process_terminal_exit_code);
    }
    uint64_t args[6];
    for (size_t n = 0; n < 6; n++)
      args[n] = packet.args[n];
    if (poly_disable_io_uring_enabled() &&
        (x86_number == SYS_io_uring_setup ||
         x86_number == SYS_io_uring_enter ||
         x86_number == SYS_io_uring_register)) {
      poly_trace_syscall_result(&packet, "io-uring-disabled", x86_number,
        (uint64_t) -ENOSYS);
      return poly_trap_vector_return_result((uint64_t) -ENOSYS,
        &packet, &trap_state, has_trap_state);
    }
    if (poly_disable_rseq_enabled() && x86_number == SYS_rseq) {
      poly_trace_syscall_result(&packet, "rseq-disabled", x86_number,
        (uint64_t) -ENOSYS);
      return poly_trap_vector_return_result((uint64_t) -ENOSYS,
        &packet, &trap_state, has_trap_state);
    }
    uint64_t foreign_clone_child_stack = 0;
    uint64_t foreign_clone_tls = 0;
    int raw_foreign_clone = 0;
    uint64_t native_clone_child_stack = 0;
    struct poly_clone_child_handoff *clone_handoff = NULL;
    if (x86_number == SYS_clone3 &&
        (packet.mode == POLY_MODE_RAW_AARCH64 ||
         packet.mode == POLY_MODE_RAW_RISCV)) {
      uint64_t clone3_result = (uint64_t) -ENOSYS;
      if (args[1] > 4096)
        clone3_result = (uint64_t) -E2BIG;
      poly_trace_syscall_result(&packet, "raw-clone3-disabled", x86_number,
        clone3_result);
      return poly_trap_vector_return_result(clone3_result,
        &packet, &trap_state, has_trap_state);
    }
    if (x86_number == SYS_clone &&
        (packet.mode == POLY_MODE_RAW_AARCH64 ||
         packet.mode == POLY_MODE_RAW_RISCV)) {
      foreign_clone_child_stack = args[1];
      foreign_clone_tls =
        (args[0] & (uint64_t) CLONE_SETTLS) != 0 ? args[3] : 0;
      const uint64_t child_tid = args[4];
      args[3] = child_tid;
      args[4] = foreign_clone_tls;
      if (foreign_clone_child_stack != 0) {
        raw_foreign_clone = 1;
        if ((args[0] & (uint64_t) CLONE_VM) != 0 &&
            (args[0] & (uint64_t) CLONE_VFORK) != 0 &&
            (args[0] & (uint64_t) CLONE_THREAD) == 0) {
          uint64_t expanded_foreign_stack = 0;
          if (poly_prepare_vfork_foreign_stack(foreign_clone_child_stack,
                &expanded_foreign_stack) < 0) {
            poly_trace_syscall_result(&packet, "clone-vfork-stack", x86_number,
              (uint64_t) -ENOMEM);
            return poly_trap_vector_return_result((uint64_t) -ENOMEM,
              &packet, &trap_state, has_trap_state);
          }
          foreign_clone_child_stack = expanded_foreign_stack;
        }
        if (!has_trap_state ||
            poly_prepare_clone_child_handoff(&packet, &trap_state,
              foreign_clone_child_stack, foreign_clone_tls, &clone_handoff,
              &native_clone_child_stack) < 0) {
          poly_trace_syscall_result(&packet, "clone-handoff", x86_number,
            (uint64_t) -ENOMEM);
            return poly_trap_vector_return_result((uint64_t) -ENOMEM,
            &packet, &trap_state, has_trap_state);
        }
        uint64_t foreign_stack_word0 = 0;
        uint64_t foreign_stack_word1 = 0;
        uint64_t musl_default_exec = 0;
        uint64_t musl_expected_exec = 0;
        const int foreign_stack_words_ok =
          poly_guest_range_is_mapped(foreign_clone_child_stack, 16, 0);
        if (foreign_stack_words_ok) {
          memcpy(&foreign_stack_word0,
            (const void *) (uintptr_t) foreign_clone_child_stack, 8);
          memcpy(&foreign_stack_word1,
            (const void *) (uintptr_t) (foreign_clone_child_stack + 8), 8);
          if (packet.mode == POLY_MODE_RAW_AARCH64 &&
              packet.pc > 0x60850) {
            const uint64_t musl_base = packet.pc - 0x60850;
            musl_expected_exec = musl_base + 0x48714;
            if (poly_guest_range_is_mapped(musl_base + 0xbff88, 8, 0))
              memcpy(&musl_default_exec,
                (const void *) (uintptr_t) (musl_base + 0xbff88), 8);
          }
        }
        if (poly_trace_syscalls_enabled()) {
          uint64_t spawn_exec_arg = 0;
          uint64_t spawn_file_actions = 0;
          uint64_t spawn_thread = 0;
          uint64_t spawn_exec_arg1 = 0;
          uint64_t spawn_exec_arg2 = 0;
          uint64_t spawn_thread_exec = 0;
          uint64_t start_record0 = 0;
          uint64_t start_record1 = 0;
          uint64_t start_record2 = 0;
          uint64_t start_record3 = 0;
          uint64_t start_arg0 = 0;
          uint64_t start_arg1 = 0;
          uint64_t start_arg2 = 0;
          uint64_t start_arg3 = 0;
          uint64_t start_global = 0;
          uint64_t aarch64_clone_start = 0;
          uint64_t aarch64_clone_arg = 0;
          uint64_t aarch64_pthread_start = 0;
          uint64_t aarch64_pthread_arg = 0;
          if (packet.mode == POLY_MODE_RAW_AARCH64) {
            aarch64_clone_start = clone_handoff->state.aarch64_gpr[10];
            aarch64_clone_arg = clone_handoff->state.aarch64_gpr[12];
            if (poly_guest_range_is_mapped(aarch64_clone_arg + 1080, 16, 0)) {
              memcpy(&aarch64_pthread_start,
                (const void *) (uintptr_t) (aarch64_clone_arg + 1080), 8);
              memcpy(&aarch64_pthread_arg,
                (const void *) (uintptr_t) (aarch64_clone_arg + 1088), 8);
            }
          }
          if (foreign_stack_words_ok) {
            if (poly_guest_range_is_mapped(foreign_stack_word1 + 136,
                  40, 0)) {
              memcpy(&spawn_exec_arg,
                (const void *) (uintptr_t) (foreign_stack_word1 + 136), 8);
              memcpy(&spawn_file_actions,
                (const void *) (uintptr_t) (foreign_stack_word1 + 144), 8);
              memcpy(&spawn_thread,
                (const void *) (uintptr_t) (foreign_stack_word1 + 152), 8);
              memcpy(&spawn_exec_arg1,
                (const void *) (uintptr_t) (foreign_stack_word1 + 160), 8);
              memcpy(&spawn_exec_arg2,
                (const void *) (uintptr_t) (foreign_stack_word1 + 168), 8);
            }
            if (poly_guest_range_is_mapped(foreign_stack_word1, 32, 0)) {
              memcpy(&start_record0,
                (const void *) (uintptr_t) foreign_stack_word1, 8);
              memcpy(&start_record1,
                (const void *) (uintptr_t) (foreign_stack_word1 + 8), 8);
              memcpy(&start_record2,
                (const void *) (uintptr_t) (foreign_stack_word1 + 16), 8);
              memcpy(&start_record3,
                (const void *) (uintptr_t) (foreign_stack_word1 + 24), 8);
            }
            if (start_record1 != 0 &&
                poly_guest_range_is_mapped(start_record1, 32, 0)) {
              memcpy(&start_arg0,
                (const void *) (uintptr_t) start_record1, 8);
              memcpy(&start_arg1,
                (const void *) (uintptr_t) (start_record1 + 8), 8);
              memcpy(&start_arg2,
                (const void *) (uintptr_t) (start_record1 + 16), 8);
              memcpy(&start_arg3,
                (const void *) (uintptr_t) (start_record1 + 24), 8);
            }
            if (start_record0 > 0x155d44c) {
              const uint64_t main_base = start_record0 - 0x155d44c;
              const uint64_t start_global_addr = main_base + 0x2af0210;
              if (poly_guest_range_is_mapped(start_global_addr, 8, 0))
                memcpy(&start_global,
                  (const void *) (uintptr_t) start_global_addr, 8);
            }
            if (spawn_thread != 0 &&
                poly_guest_range_is_mapped(spawn_thread + 272, 8, 0)) {
              memcpy(&spawn_thread_exec,
                (const void *) (uintptr_t) (spawn_thread + 272), 8);
            }
          }
          fprintf(stderr,
            "POLYEXEC_CLONE_HANDOFF: mode=%llu flags=0x%llx pc=0x%llx next=0x%llx foreign_stack=0x%llx foreign_tls=0x%llx parent_tid=0x%llx child_tid=0x%llx native_stack=0x%llx native_tls=0x%llx restore_rsp=0x%llx a64_x10=0x%llx a64_x12=0x%llx a64_pd_start=0x%llx a64_pd_arg=0x%llx a64_x29=0x%llx a64_x30=0x%llx stack_ok=%d stack_q0=0x%llx stack_q1=0x%llx start0=0x%llx start1=0x%llx start2=0x%llx start3=0x%llx start_arg0=0x%llx start_arg1=0x%llx start_arg2=0x%llx start_arg3=0x%llx start_global=0x%llx spawn_exec_arg=0x%llx spawn_actions=0x%llx spawn_thread=0x%llx spawn_exec_arg1=0x%llx spawn_exec_arg2=0x%llx spawn_thread_exec=0x%llx musl_default_exec=0x%llx musl_expected_exec=0x%llx state_key=0x%llx\n",
            (unsigned long long) packet.mode,
            (unsigned long long) args[0],
            (unsigned long long) packet.pc,
            (unsigned long long) packet.next_pc,
            (unsigned long long) foreign_clone_child_stack,
            (unsigned long long) foreign_clone_tls,
            (unsigned long long) args[2],
            (unsigned long long) child_tid,
            (unsigned long long) native_clone_child_stack,
            (unsigned long long) clone_handoff->native_tls_base,
            (unsigned long long) clone_handoff->state.trap_restore.x86_gpr[15],
            (unsigned long long) aarch64_clone_start,
            (unsigned long long) aarch64_clone_arg,
            (unsigned long long) aarch64_pthread_start,
            (unsigned long long) aarch64_pthread_arg,
            (unsigned long long) clone_handoff->state.aarch64_gpr[29],
            (unsigned long long) clone_handoff->state.aarch64_gpr[30],
            foreign_stack_words_ok,
            (unsigned long long) foreign_stack_word0,
            (unsigned long long) foreign_stack_word1,
            (unsigned long long) start_record0,
            (unsigned long long) start_record1,
            (unsigned long long) start_record2,
            (unsigned long long) start_record3,
            (unsigned long long) start_arg0,
            (unsigned long long) start_arg1,
            (unsigned long long) start_arg2,
            (unsigned long long) start_arg3,
            (unsigned long long) start_global,
            (unsigned long long) spawn_exec_arg,
            (unsigned long long) spawn_file_actions,
            (unsigned long long) spawn_thread,
            (unsigned long long) spawn_exec_arg1,
            (unsigned long long) spawn_exec_arg2,
            (unsigned long long) spawn_thread_exec,
            (unsigned long long) musl_default_exec,
            (unsigned long long) musl_expected_exec,
            (unsigned long long) clone_handoff->state.state_key.explicit_key);
        }
        if (clone_handoff->native_tls_base != 0)
          args[0] |= (uint64_t) CLONE_SETTLS;
        else
          args[0] &= ~(uint64_t) CLONE_SETTLS;
        args[1] = native_clone_child_stack;
        args[4] = clone_handoff->native_tls_base;
      }
      else if (poly_trace_syscalls_enabled()) {
        fprintf(stderr,
          "POLYEXEC_CLONE_FORK: mode=%llu flags=0x%llx parent_tid=0x%llx child_tid=0x%llx tls=0x%llx\n",
          (unsigned long long) packet.mode,
          (unsigned long long) args[0],
          (unsigned long long) args[2],
          (unsigned long long) child_tid,
          (unsigned long long) foreign_clone_tls);
      }
    }
    uint64_t syscall_result = raw_foreign_clone ?
      (uint64_t) poly_x86_clone_monitor_child(args[0], args[1], args[2],
        args[3], args[4]) :
      (uint64_t) poly_x86_syscall6(x86_number, args[0], args[1], args[2],
        args[3], args[4], args[5]);
    (void) clone_handoff;
    if ((x86_number == SYS_clone || x86_number == SYS_clone3) &&
        (int64_t) syscall_result >= 0) {
      poly_prefault_executable_mappings();
      poly_prefault_writable_mappings();
      if (prefault_poly_auto_spill() < 0)
        syscall_result = (uint64_t) -EFAULT;
    }
    poly_trace_syscall_result(&packet, "generic", x86_number,
      syscall_result);
    return poly_trap_vector_return_result(syscall_result,
      &packet, &trap_state, has_trap_state);
  }

  if (packet.reason == POLY_TRAP_BREAK) {
    return poly_trap_vector_return_result(
      0x4c000000ULL | (packet.mode << 8) | packet.number,
      &packet, &trap_state, has_trap_state);
  }
  if (packet.reason == POLY_TRAP_IMPORT)
    return poly_trap_vector_return_result(
      poly_handle_foreign_import(packet.number, packet.args),
      &packet, &trap_state, has_trap_state);

  return poly_trap_vector_return_result((uint64_t) -ENOSYS,
    &packet, &trap_state, has_trap_state);
}

__attribute__((naked, noinline, used))
static void poly_trap_vector_handler(void) {
  __asm__(
    "lock btsl $0, poly_trap_vector_stack_locks+0(%rip)\n"
    "jnc .Lpoly_trap_vector_slot0\n"
    "lock btsl $0, poly_trap_vector_stack_locks+4(%rip)\n"
    "jnc .Lpoly_trap_vector_slot1\n"
    "lock btsl $0, poly_trap_vector_stack_locks+8(%rip)\n"
    "jnc .Lpoly_trap_vector_slot2\n"
    "lock btsl $0, poly_trap_vector_stack_locks+12(%rip)\n"
    "jnc .Lpoly_trap_vector_slot3\n"
    "lock btsl $0, poly_trap_vector_stack_locks+16(%rip)\n"
    "jnc .Lpoly_trap_vector_slot4\n"
    "lock btsl $0, poly_trap_vector_stack_locks+20(%rip)\n"
    "jnc .Lpoly_trap_vector_slot5\n"
    "lock btsl $0, poly_trap_vector_stack_locks+24(%rip)\n"
    "jnc .Lpoly_trap_vector_slot6\n"
    "lock btsl $0, poly_trap_vector_stack_locks+28(%rip)\n"
    "jnc .Lpoly_trap_vector_slot7\n"
    ".Lpoly_trap_vector_wait:\n"
    "pause\n"
    "lock btsl $0, poly_trap_vector_stack_locks+0(%rip)\n"
    "jc .Lpoly_trap_vector_wait\n"
    ".Lpoly_trap_vector_slot0:\n"
    "movq %r11, poly_trap_vector_stacks+65528(%rip)\n"
    "movq %rsp, poly_trap_vector_stacks+65520(%rip)\n"
    "leaq poly_trap_vector_stacks+65520(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq poly_trap_vector_stacks+65528(%rip)\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq (%rsp), %rsp\n"
    "movl $0, poly_trap_vector_stack_locks+0(%rip)\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    ".Lpoly_trap_vector_slot1:\n"
    "movq %r11, poly_trap_vector_stacks+131064(%rip)\n"
    "movq %rsp, poly_trap_vector_stacks+131056(%rip)\n"
    "leaq poly_trap_vector_stacks+131056(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq poly_trap_vector_stacks+131064(%rip)\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq (%rsp), %rsp\n"
    "movl $0, poly_trap_vector_stack_locks+4(%rip)\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    ".Lpoly_trap_vector_slot2:\n"
    "movq %r11, poly_trap_vector_stacks+196600(%rip)\n"
    "movq %rsp, poly_trap_vector_stacks+196592(%rip)\n"
    "leaq poly_trap_vector_stacks+196592(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq poly_trap_vector_stacks+196600(%rip)\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq (%rsp), %rsp\n"
    "movl $0, poly_trap_vector_stack_locks+8(%rip)\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    ".Lpoly_trap_vector_slot3:\n"
    "movq %r11, poly_trap_vector_stacks+262136(%rip)\n"
    "movq %rsp, poly_trap_vector_stacks+262128(%rip)\n"
    "leaq poly_trap_vector_stacks+262128(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq poly_trap_vector_stacks+262136(%rip)\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq (%rsp), %rsp\n"
    "movl $0, poly_trap_vector_stack_locks+12(%rip)\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    ".Lpoly_trap_vector_slot4:\n"
    "movq %r11, poly_trap_vector_stacks+327672(%rip)\n"
    "movq %rsp, poly_trap_vector_stacks+327664(%rip)\n"
    "leaq poly_trap_vector_stacks+327664(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq poly_trap_vector_stacks+327672(%rip)\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq (%rsp), %rsp\n"
    "movl $0, poly_trap_vector_stack_locks+16(%rip)\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    ".Lpoly_trap_vector_slot5:\n"
    "movq %r11, poly_trap_vector_stacks+393208(%rip)\n"
    "movq %rsp, poly_trap_vector_stacks+393200(%rip)\n"
    "leaq poly_trap_vector_stacks+393200(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq poly_trap_vector_stacks+393208(%rip)\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq (%rsp), %rsp\n"
    "movl $0, poly_trap_vector_stack_locks+20(%rip)\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    ".Lpoly_trap_vector_slot6:\n"
    "movq %r11, poly_trap_vector_stacks+458744(%rip)\n"
    "movq %rsp, poly_trap_vector_stacks+458736(%rip)\n"
    "leaq poly_trap_vector_stacks+458736(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq poly_trap_vector_stacks+458744(%rip)\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq (%rsp), %rsp\n"
    "movl $0, poly_trap_vector_stack_locks+24(%rip)\n"
    POLY_OP_TRAP_RETURN
    "ud2\n"
    ".Lpoly_trap_vector_slot7:\n"
    "movq %r11, poly_trap_vector_stacks+524280(%rip)\n"
    "movq %rsp, poly_trap_vector_stacks+524272(%rip)\n"
    "leaq poly_trap_vector_stacks+524272(%rip), %rsp\n"
    "pushq %rbx\n"
    "pushq %rcx\n"
    "pushq %rdx\n"
    "pushq %rsi\n"
    "pushq %rdi\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq poly_trap_vector_stacks+524280(%rip)\n"
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "andq $-16, %rsp\n"
    "subq $128, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "movq %rbp, %rsp\n"
    "popq %rbp\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rdi\n"
    "popq %rsi\n"
    "popq %rdx\n"
    "popq %rcx\n"
    "popq %rbx\n"
    "movq (%rsp), %rsp\n"
    "movl $0, poly_trap_vector_stack_locks+28(%rip)\n"
    POLY_OP_TRAP_RETURN
    "ud2\n");
}

static void install_poly_trap_vector(void) {
  memset((void *) poly_monitor_packet, 0, sizeof(poly_monitor_packet));
  poly_monitor_packet_count = 0;
  poly_monitor_packet_syscall_aarch64_count = 0;
  poly_monitor_packet_syscall_riscv_count = 0;
  poly_monitor_packet_break_aarch64_count = 0;
  poly_monitor_packet_break_riscv_count = 0;
  poly_monitor_packet_import_count = 0;
  poly_monitor_packet_illegal_count = 0;
  poly_monitor_packet_other_count = 0;
  poly_monitor_packet_set_value((uint64_t) (uintptr_t) poly_monitor_packet);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value((uint64_t) (void *) poly_trap_vector_handler);
  poly_trap_vector_active = 1;
}

static void clear_poly_trap_vector(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_monitor_packet_set_value(0);
  poly_trap_vector_active = 0;
}

static void report_poly_monitor_packets(void) {
  if (poly_monitor_packet_count != 0) {
    printf("POLYEXEC_MONITOR_PACKETS: count=%llu syscall_a64=%llu syscall_rv=%llu break_a64=%llu break_rv=%llu import=%llu illegal=%llu other=%llu path=%s\n",
      (unsigned long long) poly_monitor_packet_count,
      (unsigned long long) poly_monitor_packet_syscall_aarch64_count,
      (unsigned long long) poly_monitor_packet_syscall_riscv_count,
      (unsigned long long) poly_monitor_packet_break_aarch64_count,
      (unsigned long long) poly_monitor_packet_break_riscv_count,
      (unsigned long long) poly_monitor_packet_import_count,
      (unsigned long long) poly_monitor_packet_illegal_count,
      (unsigned long long) poly_monitor_packet_other_count,
      process_cross_report_path ? process_cross_report_path : "-");
  }
}

static void report_poly_syscall_summary(void) {
  if (!poly_syscall_summary_enabled() || poly_syscall_summary_total == 0)
    return;
  printf("POLYEXEC_SYSCALL_SUMMARY: total=%llu errors=%llu path=%s\n",
    (unsigned long long) poly_syscall_summary_total,
    (unsigned long long) poly_syscall_summary_errors,
    process_cross_report_path ? process_cross_report_path : "-");
  if (poly_tiocgwinsz_stdin_cache_hits != 0 ||
      poly_tiocgwinsz_stdin_cache_misses != 0) {
    printf("POLYEXEC_TIOCGWINSZ_CACHE: stdin_hits=%llu stdin_misses=%llu stdin_enotty_cached=%d path=%s\n",
      (unsigned long long) poly_tiocgwinsz_stdin_cache_hits,
      (unsigned long long) poly_tiocgwinsz_stdin_cache_misses,
      poly_tiocgwinsz_stdin_enotty_cached,
      process_cross_report_path ? process_cross_report_path : "-");
  }
  unsigned char reported_aarch64[POLYEXEC_SYSCALL_SUMMARY_MAX] = {0};
  for (size_t rank = 0; rank < 20; rank++) {
    size_t best = POLYEXEC_SYSCALL_SUMMARY_MAX;
    uint64_t best_count = 0;
    for (size_t n = 0; n < POLYEXEC_SYSCALL_SUMMARY_MAX; n++) {
      const uint64_t count = poly_syscall_summary_aarch64[n];
      if (reported_aarch64[n] || count <= best_count)
        continue;
      best = n;
      best_count = count;
    }
    if (best >= POLYEXEC_SYSCALL_SUMMARY_MAX || best_count == 0)
      break;
    printf("POLYEXEC_SYSCALL_SUMMARY_AARCH64: rank=%zu nr=%zu name=%s count=%llu errors=%llu path=%s\n",
      rank + 1,
      best,
      poly_aarch64_syscall_name(best),
      (unsigned long long) best_count,
      (unsigned long long) poly_syscall_summary_error_aarch64[best],
      process_cross_report_path ? process_cross_report_path : "-");
    reported_aarch64[best] = 1;
  }
  unsigned char reported_x86[POLYEXEC_SYSCALL_SUMMARY_MAX] = {0};
  for (size_t rank = 0; rank < 10; rank++) {
    size_t best = POLYEXEC_SYSCALL_SUMMARY_MAX;
    uint64_t best_count = 0;
    for (size_t n = 0; n < POLYEXEC_SYSCALL_SUMMARY_MAX; n++) {
      const uint64_t count = poly_syscall_summary_x86[n];
      if (!reported_x86[n] && count > best_count) {
        best = n;
        best_count = count;
      }
    }
    if (best >= POLYEXEC_SYSCALL_SUMMARY_MAX || best_count == 0)
      break;
    printf("POLYEXEC_SYSCALL_SUMMARY_X86: rank=%zu nr=%zu count=%llu path=%s\n",
      rank + 1, best, (unsigned long long) best_count,
      process_cross_report_path ? process_cross_report_path : "-");
    reported_x86[best] = 1;
  }
}

static void report_poly_auto_spill_status(void) {
  if (!polyexec_use_auto_spill)
    return;
  const uint64_t count = poly_auto_spill_count_status();
  const uint64_t bytes = poly_auto_spill_bytes_status();
  const uint64_t cycles = poly_auto_spill_cycles_status();
  printf("POLYEXEC_AUTO_SPILL_STATUS: count=%llu bytes=%llu cycles=%llu\n",
    (unsigned long long) count, (unsigned long long) bytes,
    (unsigned long long) cycles);
}

static int prepare_syscall_fixture_file(void) {
  int fd = open("user.poly", O_CREAT | O_RDWR | O_TRUNC, 0600);
  if (fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create syscall fixture: %s\n",
      strerror(errno));
    return -1;
  }
  if (fd != 3) {
    if (dup2(fd, 3) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to bind syscall fixture fd: %s\n",
        strerror(errno));
      close(fd);
      return -1;
    }
    close(fd);
    fd = 3;
  }
  static const char fixture_data[] = "poly!";
  if (write(fd, fixture_data, sizeof(fixture_data) - 1) !=
      (ssize_t) (sizeof(fixture_data) - 1)) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to write syscall fixture: %s\n",
      strerror(errno));
    return -1;
  }
  if (lseek(fd, 0, SEEK_SET) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to rewind syscall fixture: %s\n",
      strerror(errno));
    return -1;
  }
  long setxattr_result = poly_x86_syscall6(SYS_setxattr,
    (uint64_t) (uintptr_t) "user.poly",
    (uint64_t) (uintptr_t) "user.poly",
    (uint64_t) (uintptr_t) "user.poly", 4, 0, 0);
  if (setxattr_result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to prepare syscall xattr: %ld\n",
      setxattr_result);
    return -1;
  }
  return 0;
}

static int prepare_timerfd_fixture(int target_fd) {
  int fd = (int) poly_x86_syscall6(SYS_timerfd_create, CLOCK_MONOTONIC, 0, 0, 0, 0, 0);
  if (fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create timerfd fixture: %d\n",
      fd);
    return -1;
  }
  if (fd != target_fd) {
    if (dup2(fd, target_fd) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to bind timerfd fixture fd: %s\n",
        strerror(errno));
      close(fd);
      return -1;
    }
    close(fd);
  }
  return 0;
}

static int prepare_eventfd_fixture(int target_fd) {
  int fd = (int) poly_x86_syscall6(SYS_eventfd2, 0, 0, 0, 0, 0, 0);
  if (fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create eventfd fixture: %d\n",
      fd);
    return -1;
  }
  if (fd != target_fd) {
    if (dup2(fd, target_fd) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to bind eventfd fixture fd: %s\n",
        strerror(errno));
      close(fd);
      return -1;
    }
    close(fd);
  }
  return 0;
}

static int prepare_epoll_fixture(int target_fd) {
  int fd = (int) poly_x86_syscall6(SYS_epoll_create1, 0, 0, 0, 0, 0, 0);
  if (fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create epoll fixture: %d\n",
      fd);
    return -1;
  }
  if (fd != target_fd) {
    if (dup2(fd, target_fd) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to bind epoll fixture fd: %s\n",
        strerror(errno));
      close(fd);
      return -1;
    }
    close(fd);
  }
  return 0;
}

static void close_transient_fixture_fds(void) {
  for (int fd = 4; fd < 32; fd++)
    close(fd);
}

static int bind_fixture_fd(int fd, int target_fd) {
  if (fd < 0)
    return -1;
  if (fd != target_fd) {
    if (dup2(fd, target_fd) < 0) {
      close(fd);
      return -1;
    }
    close(fd);
  }
  return 0;
}

static void init_loopback_sockaddr(struct sockaddr_in *addr) {
  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
}

static void init_abstract_unix_sockaddr(struct sockaddr_un *addr,
    const char *name) {
  memset(addr, 0, sizeof(*addr));
  addr->sun_family = AF_UNIX;
  addr->sun_path[0] = '\0';
  snprintf(addr->sun_path + 1, sizeof(addr->sun_path) - 1, "%s", name);
}

static int prepare_socket_fd_fixture(int target_fd, int bind_socket,
    char *scratch, size_t scratch_size) {
  if (scratch_size < sizeof(struct sockaddr_in))
    return -1;

  int fd = (int) poly_x86_syscall6(SYS_socket, AF_INET, SOCK_STREAM, 0, 0, 0, 0);
  if (fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create socket fixture: %d\n", fd);
    return -1;
  }
  if (bind_socket) {
    struct sockaddr_in *addr = (struct sockaddr_in *) scratch;
    init_loopback_sockaddr(addr);
    long result = poly_x86_syscall6(SYS_bind, fd, (uint64_t) (uintptr_t) addr,
      sizeof(*addr), 0, 0, 0);
    if (result < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to bind socket fixture: %ld\n",
        result);
      close(fd);
      return -1;
    }
  }
  if (bind_fixture_fd(fd, target_fd) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind socket fixture fd: %s\n",
      strerror(errno));
    return -1;
  }
  return 0;
}

static int prepare_listening_socket_fixture(int target_fd, char *scratch,
    size_t scratch_size) {
  if (prepare_socket_fd_fixture(target_fd, 1, scratch, scratch_size) < 0)
    return -1;
  long result = poly_x86_syscall6(SYS_listen, target_fd, 1, 0, 0, 0, 0);
  if (result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to listen on socket fixture: %ld\n",
      result);
    return -1;
  }
  return 0;
}

static int load_socket_name(int fd, char *scratch, size_t scratch_size) {
  if (scratch_size < SCRATCH_SECOND_PATH_OFFSET + sizeof(socklen_t))
    return -1;
  socklen_t *addrlen = (socklen_t *) (scratch + SCRATCH_SECOND_PATH_OFFSET);
  *addrlen = sizeof(struct sockaddr_in);
  long result = poly_x86_syscall6(SYS_getsockname, fd,
    (uint64_t) (uintptr_t) scratch, (uint64_t) (uintptr_t) addrlen, 0, 0, 0);
  if (result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to read socket fixture name: %ld\n",
      result);
    return -1;
  }
  *addrlen = sizeof(struct sockaddr_in);
  return 0;
}

static int prepare_pending_accept_fixture(char *scratch, size_t scratch_size) {
  if (scratch_size < sizeof(struct sockaddr_un))
    return -1;

  struct sockaddr_un *addr = (struct sockaddr_un *) scratch;
  init_abstract_unix_sockaddr(addr, "polyacc");
  int server_fd = (int) poly_x86_syscall6(SYS_socket, AF_UNIX, SOCK_STREAM, 0,
    0, 0, 0);
  if (server_fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create socket server fixture: %d\n",
      server_fd);
    return -1;
  }
  long result = poly_x86_syscall6(SYS_bind, server_fd,
    (uint64_t) (uintptr_t) addr, 16, 0, 0, 0);
  if (result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind socket server fixture: %ld\n",
      result);
    close(server_fd);
    return -1;
  }
  result = poly_x86_syscall6(SYS_listen, server_fd, 1, 0, 0, 0, 0);
  if (result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to listen on socket server fixture: %ld\n",
      result);
    close(server_fd);
    return -1;
  }
  if (bind_fixture_fd(server_fd, 5) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind socket server fixture fd: %s\n",
      strerror(errno));
    return -1;
  }

  int client_fd = (int) poly_x86_syscall6(SYS_socket, AF_UNIX, SOCK_STREAM, 0,
    0, 0, 0);
  if (client_fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create socket client fixture: %d\n",
      client_fd);
    return -1;
  }
  if (bind_fixture_fd(client_fd, 4) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind socket client fixture fd: %s\n",
      strerror(errno));
    return -1;
  }
  result = poly_x86_syscall6(SYS_connect, 4, (uint64_t) (uintptr_t) scratch,
    16, 0, 0, 0);
  if (result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to connect socket client fixture: %ld\n",
      result);
    return -1;
  }
  return 0;
}

static int prepare_connect_fixture(char *scratch, size_t scratch_size) {
  if (scratch_size < sizeof(struct sockaddr_un))
    return -1;

  struct sockaddr_un *addr = (struct sockaddr_un *) scratch;
  init_abstract_unix_sockaddr(addr, "polycon");
  int server_fd = (int) poly_x86_syscall6(SYS_socket, AF_UNIX, SOCK_STREAM, 0,
    0, 0, 0);
  if (server_fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create connect server fixture: %d\n",
      server_fd);
    return -1;
  }
  long result = poly_x86_syscall6(SYS_bind, server_fd,
    (uint64_t) (uintptr_t) addr, 16, 0, 0, 0);
  if (result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind connect server fixture: %ld\n",
      result);
    close(server_fd);
    return -1;
  }
  result = poly_x86_syscall6(SYS_listen, server_fd, 1, 0, 0, 0, 0);
  if (result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to listen on connect server fixture: %ld\n",
      result);
    close(server_fd);
    return -1;
  }
  if (bind_fixture_fd(server_fd, 4) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind connect server fixture fd: %s\n",
      strerror(errno));
    return -1;
  }
  int client_fd = (int) poly_x86_syscall6(SYS_socket, AF_UNIX, SOCK_STREAM, 0,
    0, 0, 0);
  if (client_fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create connect client fixture: %d\n",
      client_fd);
    return -1;
  }
  if (bind_fixture_fd(client_fd, 5) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind connect client fixture fd: %s\n",
      strerror(errno));
    return -1;
  }
  return 0;
}

static int prepare_socketpair_fixture(int fd0, int fd1) {
  int fds[2] = { -1, -1 };
  long result = poly_x86_syscall6(SYS_socketpair, AF_UNIX, SOCK_STREAM, 0,
    (uint64_t) (uintptr_t) fds, 0, 0);
  if (result < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create socketpair fixture: %ld\n",
      result);
    return -1;
  }
  if (bind_fixture_fd(fds[0], fd0) < 0 ||
      bind_fixture_fd(fds[1], fd1) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind socketpair fixture fd: %s\n",
      strerror(errno));
    return -1;
  }
  return 0;
}

static int prepare_directory_fd_fixture(const char *path, int target_fd) {
  rmdir(path);
  unlink(path);
  if (mkdir(path, 0700) < 0 && errno != EEXIST) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to create directory fixture %s: %s\n",
      path, strerror(errno));
    return -1;
  }
  int fd = open(path, O_RDONLY | O_DIRECTORY);
  if (fd < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to open directory fixture %s: %s\n",
      path, strerror(errno));
    return -1;
  }
  if (bind_fixture_fd(fd, target_fd) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to bind directory fixture fd: %s\n",
      strerror(errno));
    return -1;
  }
  return 0;
}

static int prepare_program_scratch(const char *program_path, char *scratch,
    size_t scratch_size) {
  if (scratch_size < sizeof("poly!\0/init"))
    return -1;

  close_transient_fixture_fds();

  if (prepare_syscall_fixture_file() < 0)
    return -1;

  memcpy(scratch, "poly!\0/init", sizeof("poly!\0/init"));

  if (strstr(program_path, "-timerfd-settime.") != NULL ||
      strstr(program_path, "-timerfd-gettime.") != NULL) {
    memset(scratch, 0, scratch_size);
    return prepare_timerfd_fixture(13);
  }

  if (strstr(program_path, "-pselect6.") != NULL ||
      strstr(program_path, "-ppoll.") != NULL ||
      strstr(program_path, "-nanosleep.") != NULL ||
      strstr(program_path, "-setitimer.") != NULL ||
      strstr(program_path, "-sched-setparam.") != NULL ||
      strstr(program_path, "-sched-setscheduler.") != NULL) {
    memset(scratch, 0, scratch_size);
    return 0;
  }

  if (strstr(program_path, "-epoll-ctl.") != NULL ||
      strstr(program_path, "-epoll-pwait.") != NULL) {
    memset(scratch, 0, scratch_size);
    ((uint32_t *) scratch)[0] = 1;
    if (prepare_eventfd_fixture(3) < 0)
      return -1;
    return prepare_epoll_fixture(4);
  }

  if (strstr(program_path, "-sched-setaffinity.") != NULL) {
    memset(scratch, 0, scratch_size);
    ((uint64_t *) scratch)[0] = 1;
    return 0;
  }

  if (strstr(program_path, "-setrlimit.") != NULL) {
    memset(scratch, 0, scratch_size);
    if (getrlimit(RLIMIT_STACK, (struct rlimit *) scratch) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to prepare rlimit fixture: %s\n",
        strerror(errno));
      return -1;
    }
    return 0;
  }

  if (strstr(program_path, "-bind.") != NULL) {
    memset(scratch, 0, scratch_size);
    init_loopback_sockaddr((struct sockaddr_in *) scratch);
    return prepare_socket_fd_fixture(5, 0, scratch, scratch_size);
  }

  if (strstr(program_path, "-listen.") != NULL)
    return prepare_socket_fd_fixture(5, 1, scratch, scratch_size);

  if (strstr(program_path, "-accept.") != NULL ||
      strstr(program_path, "-accept4.") != NULL)
    return prepare_pending_accept_fixture(scratch, scratch_size);

  if (strstr(program_path, "-connect.") != NULL)
    return prepare_connect_fixture(scratch, scratch_size);

  if (strstr(program_path, "-getsockname.") != NULL) {
    if (prepare_socket_fd_fixture(5, 1, scratch, scratch_size) < 0)
      return -1;
    return load_socket_name(5, scratch, scratch_size);
  }

  if (strstr(program_path, "-getpeername.") != NULL) {
    if (prepare_socketpair_fixture(4, 5) < 0)
      return -1;
    memset(scratch, 0, scratch_size);
    *(socklen_t *) (scratch + SCRATCH_SECOND_PATH_OFFSET) =
      sizeof(struct sockaddr_storage);
    return 0;
  }

  if (strstr(program_path, "-sendto.") != NULL ||
      strstr(program_path, "-shutdown.") != NULL)
    return prepare_socketpair_fixture(4, 5);

  if (strstr(program_path, "-recvfrom.") != NULL) {
    static const char data[] = "poly";
    if (prepare_socketpair_fixture(4, 5) < 0)
      return -1;
    if (write(4, data, sizeof(data) - 1) != (ssize_t) (sizeof(data) - 1)) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to seed socket recv fixture: %s\n",
        strerror(errno));
      return -1;
    }
    return 0;
  }

  if (strstr(program_path, "-setsockopt.") != NULL) {
    memset(scratch, 0, scratch_size);
    *(int *) scratch = 1;
    return prepare_socket_fd_fixture(5, 0, scratch, scratch_size);
  }

  if (strstr(program_path, "-getsockopt.") != NULL) {
    memset(scratch, 0, scratch_size);
    *(socklen_t *) (scratch + SCRATCH_SECOND_PATH_OFFSET) = sizeof(int);
    return prepare_socket_fd_fixture(5, 0, scratch, scratch_size);
  }

  if (strstr(program_path, "-capget.") != NULL ||
      strstr(program_path, "-capset.") != NULL) {
    memset(scratch, 0, scratch_size);
    ((uint32_t *) scratch)[0] = 0x20080522U;
    if (strstr(program_path, "-capset.") != NULL)
      ((int32_t *) scratch)[1] = -1;
    return 0;
  }

  if (strstr(program_path, "xattr") != NULL) {
    if (prepare_syscall_fixture_file() < 0)
      return -1;
    memcpy(scratch, "user.poly\0/init", sizeof("user.poly\0/init"));
    return 0;
  }

  if (strstr(program_path, "-statfs.") != NULL &&
      strstr(program_path, "-fstatfs.") == NULL) {
    memcpy(scratch, "/\0/init", sizeof("/\0/init"));
    return 0;
  }

  if (strstr(program_path, "-openat.") != NULL ||
      (strstr(program_path, "-openat-") != NULL &&
       strstr(program_path, "-real-openat-") == NULL)) {
    memcpy(scratch, "/init", sizeof("/init"));
    return 0;
  }

  if (strstr(program_path, "-faccessat.") != NULL) {
    memcpy(scratch, "/init", sizeof("/init"));
    return 0;
  }

  if (strstr(program_path, "-readlinkat.") != NULL) {
    memcpy(scratch, "/polyexec-readlink", sizeof("/polyexec-readlink"));
    unlink(scratch);
    if (symlink("poly!", scratch) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to create readlink fixture: %s\n",
        strerror(errno));
      return -1;
    }
    return 0;
  }

  if (strstr(program_path, "-newfstatat.") != NULL) {
    memcpy(scratch, "/init", sizeof("/init"));
    return 0;
  }

  if (strstr(program_path, "-statx.") != NULL &&
      strstr(program_path, "-real-statx.") == NULL) {
    memcpy(scratch, "/init", sizeof("/init"));
    return 0;
  }

  if (strstr(program_path, "-getdents64.") != NULL)
    return prepare_directory_fd_fixture("/polyexec-getdents64", 3);

  if (strstr(program_path, "-chdir.") != NULL) {
    const char *name = strrchr(program_path, '/');
    name = name ? name + 1 : program_path;
    if (snprintf(scratch, scratch_size, "/polyexec-%s", name) >=
        (int) scratch_size) {
      fprintf(stderr, "POLYEXEC_FAIL: syscall fixture path too long: %s\n",
        program_path);
      return -1;
    }
    unlink(scratch);
    rmdir(scratch);
    if (mkdir(scratch, 0700) < 0 && errno != EEXIST) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to create chdir fixture: %s: %s\n",
        program_path, strerror(errno));
      return -1;
    }
    return 0;
  }

  if (strstr(program_path, "-fchdir.") != NULL) {
    const char *path = "/polyexec-fchdir";
    rmdir(path);
    unlink(path);
    if (mkdir(path, 0700) < 0 && errno != EEXIST) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to create fchdir fixture: %s: %s\n",
        program_path, strerror(errno));
      return -1;
    }
    int fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to open fchdir fixture: %s: %s\n",
        program_path, strerror(errno));
      return -1;
    }
    if (fd != 3) {
      if (dup2(fd, 3) < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: unable to bind fchdir fixture fd: %s\n",
          strerror(errno));
        close(fd);
        return -1;
      }
      close(fd);
    }
    return 0;
  }

  if (strstr(program_path, "-linkat.") != NULL) {
    const char *name = strrchr(program_path, '/');
    name = name ? name + 1 : program_path;
    if (scratch_size <= SCRATCH_SECOND_PATH_OFFSET ||
        snprintf(scratch, scratch_size, "/polyexec-%s-old", name) >=
        (int) scratch_size ||
        snprintf(scratch + SCRATCH_SECOND_PATH_OFFSET,
          scratch_size - SCRATCH_SECOND_PATH_OFFSET, "/polyexec-%s-new",
          name) >= (int) (scratch_size - SCRATCH_SECOND_PATH_OFFSET)) {
      fprintf(stderr, "POLYEXEC_FAIL: syscall fixture path too long: %s\n",
        program_path);
      return -1;
    }
    unlink(scratch);
    unlink(scratch + SCRATCH_SECOND_PATH_OFFSET);
    int fd = open(scratch, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fd < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unable to create link fixture: %s: %s\n",
        program_path, strerror(errno));
      return -1;
    }
    close(fd);
    return 0;
  }

  if (strstr(program_path, "-mknodat.") != NULL ||
      strstr(program_path, "-mkdirat.") != NULL ||
      strstr(program_path, "-unlinkat.") != NULL ||
      strstr(program_path, "-symlinkat.") != NULL ||
      strstr(program_path, "-renameat.") != NULL ||
      strstr(program_path, "-renameat2.") != NULL ||
      strstr(program_path, "-truncate.") != NULL ||
      strstr(program_path, "-fchmodat.") != NULL ||
      strstr(program_path, "-fchownat.") != NULL) {
    const char *name = strrchr(program_path, '/');
    name = name ? name + 1 : program_path;
    if (snprintf(scratch, scratch_size, "/polyexec-%s", name) >=
        (int) scratch_size) {
      fprintf(stderr, "POLYEXEC_FAIL: syscall fixture path too long: %s\n",
        program_path);
      return -1;
    }
    rmdir(scratch);
    unlink(scratch);
    if (strstr(program_path, "-unlinkat.") != NULL ||
        strstr(program_path, "-renameat.") != NULL ||
        strstr(program_path, "-renameat2.") != NULL ||
        strstr(program_path, "-truncate.") != NULL ||
        strstr(program_path, "-fchmodat.") != NULL ||
        strstr(program_path, "-fchownat.") != NULL) {
      int fd = open(scratch, O_CREAT | O_RDWR | O_TRUNC, 0600);
      if (fd < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: unable to create path fixture: %s: %s\n",
          program_path, strerror(errno));
        return -1;
      }
      close(fd);
    }
  }

  return 0;
}

static int parse_u64(const char *text, uint64_t *value) {
  char *end = NULL;
  errno = 0;
  unsigned long long parsed = strtoull(text, &end, 0);
  if (errno || end == text || *end != '\0')
    return -1;
  *value = (uint64_t) parsed;
  return 0;
}

static int parse_request(const char *arg, struct poly_request *request) {
  memset(request, 0, sizeof(*request));
  const char *expected = strchr(arg, '=');
  size_t path_len = expected ? (size_t) (expected - arg) : strlen(arg);
  const char *symbol = memchr(arg, '#', path_len);
  if (symbol) {
    size_t symbol_len = path_len - (size_t) (symbol + 1 - arg);
    path_len = (size_t) (symbol - arg);
    if (symbol_len == 0 || symbol_len >= sizeof(request->symbol)) {
      fprintf(stderr, "POLYEXEC_FAIL: bad symbol argument: %s\n", arg);
      return -1;
    }
    memcpy(request->symbol, symbol + 1, symbol_len);
    request->symbol[symbol_len] = '\0';
  }
  if (path_len == 0 || path_len >= sizeof(request->path)) {
    fprintf(stderr, "POLYEXEC_FAIL: bad argument: %s\n", arg);
    return -1;
  }

  memcpy(request->path, arg, path_len);
  request->path[path_len] = '\0';
  if (expected) {
    if (strcmp(expected + 1, "pid") == 0) {
      request->expected = (uint64_t) getpid();
    }
    else if (strcmp(expected + 1, "ppid") == 0) {
      request->expected = (uint64_t) getppid();
    }
    else if (strcmp(expected + 1, "uid") == 0) {
      request->expected = (uint64_t) getuid();
    }
    else if (strcmp(expected + 1, "euid") == 0) {
      request->expected = (uint64_t) geteuid();
    }
    else if (strcmp(expected + 1, "gid") == 0) {
      request->expected = (uint64_t) getgid();
    }
    else if (strcmp(expected + 1, "egid") == 0) {
      request->expected = (uint64_t) getegid();
    }
    else if (strcmp(expected + 1, "tid") == 0) {
      request->expected = (uint64_t) syscall(SYS_gettid);
    }
    else if (strcmp(expected + 1, "pgid") == 0) {
      request->expected = (uint64_t) getpgid(0);
    }
    else if (strcmp(expected + 1, "sid") == 0) {
      request->expected = (uint64_t) getsid(0);
    }
    else if (strcmp(expected + 1, "stackrlim") == 0) {
      struct rlimit limit;
      if (getrlimit(RLIMIT_STACK, &limit) < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: unable to compute getrlimit expected value: %s\n",
          strerror(errno));
        return -1;
      }
      request->expected = (uint64_t) limit.rlim_cur;
    }
    else if (strcmp(expected + 1, "clockresnsec") == 0) {
      struct timespec resolution;
      if (clock_getres(CLOCK_REALTIME, &resolution) < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: unable to compute clock_getres expected value: %s\n",
          strerror(errno));
        return -1;
      }
      request->expected = (uint64_t) resolution.tv_nsec;
    }
    else if (strcmp(expected + 1, "cwd") == 0) {
      char cwd[256];
      long result = syscall(SYS_getcwd, cwd, sizeof(cwd));
      if (result < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: unable to compute getcwd expected value: %s\n",
          strerror(errno));
        return -1;
      }
      request->expected = (uint64_t) result;
    }
    else if (parse_u64(expected + 1, &request->expected) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: bad expected value: %s\n", arg);
      return -1;
    }
    request->check_expected = 1;
  }
  return 0;
}

static int read_file(const char *path, unsigned char **data, size_t *size) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to open %s: %s\n", path, strerror(errno));
    return -1;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to seek %s\n", path);
    fclose(file);
    return -1;
  }
  long file_size = ftell(file);
  if (file_size <= 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to size %s\n", path);
    fclose(file);
    return -1;
  }
  rewind(file);

  unsigned char *buffer = malloc((size_t) file_size);
  if (!buffer) {
    fprintf(stderr, "POLYEXEC_FAIL: out of memory reading %s\n", path);
    fclose(file);
    return -1;
  }
  if (fread(buffer, 1, (size_t) file_size, file) != (size_t) file_size) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to read %s\n", path);
    free(buffer);
    fclose(file);
    return -1;
  }
  fclose(file);
  *data = buffer;
  *size = (size_t) file_size;
  return 0;
}

static void emit_u32(uint8_t *code, size_t *offset, uint32_t value) {
  code[(*offset)++] = (uint8_t) (value & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 8) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 16) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 24) & 0xff);
}

static void store_u32(uint8_t *code, size_t offset, uint32_t value) {
  code[offset] = (uint8_t) (value & 0xff);
  code[offset + 1] = (uint8_t) ((value >> 8) & 0xff);
  code[offset + 2] = (uint8_t) ((value >> 16) & 0xff);
  code[offset + 3] = (uint8_t) ((value >> 24) & 0xff);
}

static void emit_u64(uint8_t *code, size_t *offset, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    code[(*offset)++] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static void emit_bytes(uint8_t *code, size_t *offset, const uint8_t *bytes, size_t size) {
  memcpy(code + *offset, bytes, size);
  *offset += size;
}

static void emit_x86_poly_control(uint8_t *code, size_t *offset,
    uint8_t subop) {
  code[(*offset)++] = POLY_X86_CTRL_PREFIX_0;
  code[(*offset)++] = POLY_X86_CTRL_PREFIX_1;
  code[(*offset)++] = POLY_X86_CTRL_PREFIX_2;
  code[(*offset)++] = subop;
}

static size_t poly_frontend_entry_alignment(uint32_t frontend) {
  return frontend == POLY_ARCH_AARCH64 || frontend == POLY_ARCH_RISCV ? 4U : 1U;
}

static size_t x86_penter_frontend_size_at(size_t offset, uint32_t frontend) {
  const size_t align = poly_frontend_entry_alignment(frontend);
  const size_t target = offset + POLY_X86_PENTER_BASE_SIZE;
  const size_t pad = align > 1 ? ((align - (target & (align - 1U))) & (align - 1U)) : 0;
  return POLY_X86_PENTER_BASE_SIZE + pad;
}

static void emit_x86_entry_alignment(uint8_t *code, size_t *offset,
    uint32_t frontend) {
  size_t pad = x86_penter_frontend_size_at(*offset, frontend) -
    POLY_X86_PENTER_BASE_SIZE;
  while (pad-- > 0)
    code[(*offset)++] = 0x90;
}

static void emit_x86_penter_frontend(uint8_t *code, size_t *offset,
    uint32_t frontend) {
  emit_x86_entry_alignment(code, offset, frontend);
  code[(*offset)++] = 0x41; // mov r15d,frontend
  code[(*offset)++] = 0xbf;
  emit_u32(code, offset, frontend);
  emit_x86_poly_control(code, offset, POLY_X86_CTRL_PENTER_MODE);
}

static void emit_x86_movabs_rax(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x48;
  code[(*offset)++] = 0xb8;
  emit_u64(code, offset, value);
}

static void emit_x86_movabs_rbx(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x48;
  code[(*offset)++] = 0xbb;
  emit_u64(code, offset, value);
}

static void emit_x86_movabs_r11(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbb;
  emit_u64(code, offset, value);
}

static void emit_x86_mov_r15d_imm(uint8_t *code, size_t *offset,
    uint32_t value) {
  code[(*offset)++] = 0x41;
  code[(*offset)++] = 0xbf;
  emit_u32(code, offset, value);
}

static void emit_x86_state_key_set(uint8_t *code, size_t *offset,
    uint64_t state_key) {
  emit_x86_movabs_rax(code, offset, state_key);
  emit_x86_poly_control(code, offset, POLY_X86_CTRL_STATE_KEY_SET);
}

static void emit_x86_pcall_sig_imm(uint8_t *code, size_t *offset,
    uint32_t frontend, uint32_t signature_slot) {
  emit_x86_mov_r15d_imm(code, offset, frontend);
  emit_x86_poly_control(code, offset,
    (uint8_t) POLY_X86_CTRL_PCALL_SIG_IMM(signature_slot));
}

static void emit_x86_preserve_sret_ptr_rax(uint8_t *code, size_t *offset) {
  code[(*offset)++] = 0x48; // mov rax,rdi: preserve hidden sret pointer.
  code[(*offset)++] = 0x89;
  code[(*offset)++] = 0xf8;
}

static void emit_x86_store_hfa64_return_to_sret(uint8_t *code, size_t *offset,
    uint32_t count) {
  for (uint32_t n = 0; n < count; n++) {
    code[(*offset)++] = 0xf2; // movsd [rax+disp8],xmmN.
    code[(*offset)++] = 0x0f;
    code[(*offset)++] = 0x11;
    code[(*offset)++] = (uint8_t) (0x40U + (n << 3));
    code[(*offset)++] = (uint8_t) (n * 8U);
  }
}

static void emit_x86_load_hfa64_arg_from_stack(uint8_t *code, size_t *offset,
    uint32_t count) {
  const uint32_t scale_reg = count;
  code[(*offset)++] = 0xf2; // movsd xmm<count>,xmm0: preserve scalar arg.
  code[(*offset)++] = 0x0f;
  code[(*offset)++] = 0x10;
  code[(*offset)++] = (uint8_t) (0xc0U + (scale_reg << 3));
  for (uint32_t n = 0; n < count; n++) {
    code[(*offset)++] = 0xf2; // movsd xmmN,[rsp+disp8].
    code[(*offset)++] = 0x0f;
    code[(*offset)++] = 0x10;
    code[(*offset)++] = (uint8_t) (0x44U + (n << 3));
    code[(*offset)++] = 0x24;
    code[(*offset)++] = (uint8_t) (32U + n * 8U);
  }
}

static void emit_x86_exit_group_from_eax(uint8_t *code, size_t *offset) {
  code[(*offset)++] = 0x89; // mov edi,eax
  code[(*offset)++] = 0xc7;
  code[(*offset)++] = 0xb8; // mov eax,SYS_exit_group
  emit_u32(code, offset, (uint32_t) SYS_exit_group);
  code[(*offset)++] = 0x0f;
  code[(*offset)++] = 0x05; // syscall
  code[(*offset)++] = 0xf4; // hlt if the syscall unexpectedly returns
}

static uint64_t get_x86_fs_base(void) {
  uint64_t fs_base = 0;
  register long rax __asm__("rax") = SYS_arch_prctl;
  register long rdi __asm__("rdi") = ARCH_GET_FS;
  register uint64_t rsi __asm__("rsi") = (uint64_t) (uintptr_t) &fs_base;
  __asm__ volatile("syscall"
      : "+a"(rax)
      : "D"(rdi), "S"(rsi)
      : "rcx", "r11", "memory");
  return rax < 0 ? 0 : fs_base;
}

static int load_segment_prot(uint32_t flags) {
  int prot = 0;
  if ((flags & PF_R) != 0)
    prot |= PROT_READ;
  if ((flags & PF_W) != 0)
    prot |= PROT_WRITE;
  if ((flags & PF_X) != 0)
    prot |= PROT_EXEC;
  return prot;
}

static int protect_image_range(const struct poly_program *program,
    uint8_t *image, uint64_t vaddr, uint64_t size, int prot,
    const char *range_name) {
  if (size == 0)
    return 0;
  if (vaddr < program->base_vaddr || vaddr > UINT64_MAX - size) {
    fprintf(stderr, "POLYEXEC_FAIL: bad %s range: %s\n",
      range_name, program->path);
    return -1;
  }

  const uint64_t start = align_down_u64(vaddr, 0x1000);
  const uint64_t end_unaligned = vaddr + size;
  if (end_unaligned > UINT64_MAX - 0xfff) {
    fprintf(stderr, "POLYEXEC_FAIL: bad %s page range: %s\n",
      range_name, program->path);
    return -1;
  }
  const uint64_t end = align_up_u64(end_unaligned, 0x1000);
  const uint64_t mapped_image_size = align_up_u64(program->code_size, 0x1000);
  if (start < program->base_vaddr || end < start ||
      end - start > SIZE_MAX ||
      end - program->base_vaddr > mapped_image_size) {
    fprintf(stderr, "POLYEXEC_FAIL: %s escaped image: %s\n",
      range_name, program->path);
    return -1;
  }

  void *addr = image + (size_t) (start - program->base_vaddr);
  const size_t length = (size_t) (end - start);
  if (mprotect(addr, length, prot) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: %s mprotect failed: %s: %s\n",
      range_name, program->path, strerror(errno));
    return -1;
  }
  return 0;
}

static int protect_load_segments(const struct poly_program *program,
    uint8_t *image) {
  for (size_t n = 0; n < program->load_segment_count; n++) {
    if (protect_image_range(program, image, program->load_segments[n].vaddr,
          program->load_segments[n].memsz,
          load_segment_prot(program->load_segments[n].flags), "PT_LOAD") < 0)
      return -1;
  }
  return 0;
}

static uint32_t aarch64_adr(unsigned rd, int64_t byte_offset);
static void emit_aarch64_movabs(uint8_t *code, size_t *offset, uint32_t rd,
    uint64_t value);
static uint32_t riscv_auipc(unsigned rd, int64_t byte_offset);
static uint32_t riscv_addi(unsigned rd, unsigned rs1, int64_t byte_offset);
static uint32_t riscv_jalr(unsigned rd, unsigned rs1, int16_t byte_offset);

static uint64_t run_poly_entry(const uint8_t *code, uint8_t *scratch) {
  uint64_t rax = (uint64_t) (uintptr_t) scratch;
  if (refresh_poly_auto_spill() < 0)
    return (uint64_t) -EIO;
  asm volatile(
      "pushq %%rbx\n"
      "pushq %%rbp\n"
      "pushq %%r12\n"
      "pushq %%r13\n"
      "pushq %%r14\n"
      "pushq %%r15\n"
      "movq %%rax, %%rdx\n"
      "movq %%rax, %%rcx\n"
      "movq %%rax, %%rdi\n"
      "movq %%rax, %%rsi\n"
      "call *%1\n"
      "popq %%r15\n"
      "popq %%r14\n"
      "popq %%r13\n"
      "popq %%r12\n"
      "popq %%rbp\n"
      "popq %%rbx"
      : "+a"(rax)
      : "r"(code)
      : "rdi", "rsi", "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

__attribute__((noreturn))
static void run_poly_process_entry(const uint8_t *code,
    uint64_t initial_sp, uint64_t tls_base, uint64_t x86_fs_base,
    uint64_t state_key, int use_trap_vector, int arch) {
  (void) arch;
  if (x86_fs_base != 0) {
    register long rax __asm__("rax") = SYS_arch_prctl;
    register long rdi __asm__("rdi") = ARCH_SET_FS;
    register uint64_t rsi __asm__("rsi") = x86_fs_base;
    __asm__ volatile("syscall"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi)
        : "rcx", "r11", "memory");
    if (rax < 0) {
      register long exit_rax __asm__("rax") = SYS_exit_group;
      register long exit_rdi __asm__("rdi") = 127;
      __asm__ volatile("syscall"
          :
          : "a"(exit_rax), "D"(exit_rdi)
          : "rcx", "r11", "memory");
      __builtin_unreachable();
    }
  }
  if (state_key != 0) {
    uint64_t value = state_key;
    asm volatile(POLY_OP_STATE_KEY_SET : "+a"(value) :: "memory");
  }
  if (x86_fs_base != 0) {
    uint64_t slot = process_native_signature_slot;
    uint64_t signature =
      poly_abi_signature_control_value_with_flags(
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS,
        POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE);
    asm volatile(POLY_OP_ABI_SIGNATURE_SET
      : "+a"(slot), "+d"(signature)
      :
      : "memory");
  }
  if (use_trap_vector) {
    uint64_t value = (uint64_t) (uintptr_t) poly_monitor_packet;
    asm volatile(POLY_OP_MONITOR_PACKET_SET : "+a"(value) :: "memory");
    value = POLY_MODE_X86;
    asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET : "+a"(value) :: "memory");
    value = (uint64_t) (uintptr_t) poly_trap_vector_handler;
    asm volatile(POLY_OP_TRAP_VECTOR_SET : "+a"(value) :: "memory");
  }
  if (refresh_poly_auto_spill() < 0) {
    register long exit_rax __asm__("rax") = SYS_exit_group;
    register long exit_rdi __asm__("rdi") = 125;
    __asm__ volatile("syscall"
        :
        : "a"(exit_rax), "D"(exit_rdi)
        : "rcx", "r11", "memory");
    __builtin_unreachable();
  }
  asm volatile(
      "movq %0, %%r11\n"
      "movq %2, %%r13\n"
      "movq %1, %%rsp\n"
      "xorq %%rax, %%rax\n"
      "jmp *%%r11\n"
      :
      : "r"(code), "r"(initial_sp), "r"(tls_base)
      : "rax", "r11", "r13", "memory");
  __builtin_unreachable();
}

static int copy_stack_string(uint8_t *stack, uint8_t **cursor,
    const char *value, uint64_t *guest_addr) {
  size_t len = strlen(value) + 1;
  if ((size_t) (*cursor - stack) < len)
    return -1;
  *cursor -= len;
  memcpy(*cursor, value, len);
  *guest_addr = (uint64_t) (uintptr_t) *cursor;
  return 0;
}

static int parse_u64_env(const char *name, uint64_t *out_value) {
  const char *value = getenv(name);
  if (value == NULL || value[0] == '\0')
    return 0;
  errno = 0;
  char *end = NULL;
  uint64_t parsed = strtoull(value, &end, 0);
  if (errno != 0 || end == value || *end != '\0') {
    fprintf(stderr, "POLYEXEC_FAIL: invalid %s=%s\n", name, value);
    return -1;
  }
  *out_value = parsed;
  return 1;
}

static int lookup_elf_dynsym_value_by_section(const uint8_t *image,
    size_t image_size, const char *symbol_name, uint64_t *value_out) {
  if (image_size < sizeof(Elf64_Ehdr) || symbol_name == NULL ||
      value_out == NULL)
    return -1;
  const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *) image;
  if (ehdr->e_shentsize != sizeof(Elf64_Shdr) ||
      ehdr->e_shoff > image_size ||
      ehdr->e_shnum > (image_size - ehdr->e_shoff) / sizeof(Elf64_Shdr))
    return -1;
  const Elf64_Shdr *shdrs =
    (const Elf64_Shdr *) (const void *) (image + ehdr->e_shoff);
  for (uint16_t n = 0; n < ehdr->e_shnum; n++) {
    const Elf64_Shdr *symtab = &shdrs[n];
    if (symtab->sh_type != SHT_DYNSYM ||
        symtab->sh_entsize < sizeof(Elf64_Sym) ||
        symtab->sh_link >= ehdr->e_shnum ||
        symtab->sh_offset > image_size ||
        symtab->sh_size > image_size - symtab->sh_offset)
      continue;
    const Elf64_Shdr *strtab = &shdrs[symtab->sh_link];
    if (strtab->sh_type != SHT_STRTAB ||
        strtab->sh_offset > image_size ||
        strtab->sh_size > image_size - strtab->sh_offset)
      continue;
    const size_t count = symtab->sh_size / symtab->sh_entsize;
    const char *strings = (const char *) image + strtab->sh_offset;
    for (size_t index = 0; index < count; index++) {
      const Elf64_Sym *sym = (const Elf64_Sym *) (const void *)
        (image + symtab->sh_offset + index * symtab->sh_entsize);
      if (sym->st_name >= strtab->sh_size)
        continue;
      if (strcmp(strings + sym->st_name, symbol_name) == 0) {
        *value_out = sym->st_value;
        return 0;
      }
    }
  }
  return -1;
}

static int map_process_aarch64_vdso(uint64_t *at_sysinfo_ehdr_out) {
  *at_sysinfo_ehdr_out = 0;
  const char *path = getenv("POLY_AARCH64_VDSO_PATH");
  if (!path || path[0] == '\0')
    path = "/usr/lib/polyapps/aarch64-polyexec-vdso.so";

  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    if (errno == ENOENT)
      return 0;
    fprintf(stderr, "POLYEXEC_FAIL: unable to open AArch64 vDSO %s: %s\n",
      path, strerror(errno));
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: invalid AArch64 vDSO stat %s: %s\n",
      path, strerror(errno));
    close(fd);
    return -1;
  }
  if (st.st_size <= 0) {
    fprintf(stderr, "POLYEXEC_FAIL: empty AArch64 vDSO: %s\n", path);
    close(fd);
    return -1;
  }
  if ((uint64_t) st.st_size > SIZE_MAX) {
    fprintf(stderr, "POLYEXEC_FAIL: AArch64 vDSO too large: %s\n", path);
    close(fd);
    return -1;
  }
  const size_t file_size = (size_t) st.st_size;
  uint8_t *file_image = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (file_image == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: AArch64 vDSO file mmap failed %s: %s\n",
      path, strerror(errno));
    return -1;
  }
  if (file_size < sizeof(Elf64_Ehdr)) {
    fprintf(stderr, "POLYEXEC_FAIL: AArch64 vDSO missing ELF header: %s\n",
      path);
    munmap(file_image, file_size);
    return -1;
  }

  const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *) file_image;
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
      ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
      ehdr->e_type != ET_DYN ||
      ehdr->e_machine != EM_AARCH64) {
    fprintf(stderr, "POLYEXEC_FAIL: invalid AArch64 vDSO ELF identity: %s\n",
      path);
    munmap(file_image, file_size);
    return -1;
  }
  if (ehdr->e_phentsize != sizeof(Elf64_Phdr) ||
      ehdr->e_phoff > file_size ||
      ehdr->e_phnum > (file_size - ehdr->e_phoff) / sizeof(Elf64_Phdr)) {
    fprintf(stderr, "POLYEXEC_FAIL: invalid AArch64 vDSO program headers: %s\n",
      path);
    munmap(file_image, file_size);
    return -1;
  }

  const Elf64_Phdr *phdrs =
    (const Elf64_Phdr *) (const void *) (file_image + ehdr->e_phoff);
  uint64_t min_vaddr = UINT64_MAX;
  uint64_t max_vaddr = 0;
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *ph = &phdrs[n];
    if (ph->p_type != PT_LOAD)
      continue;
    if (ph->p_memsz < ph->p_filesz ||
        ph->p_offset > (uint64_t) file_size ||
        ph->p_filesz > (uint64_t) file_size - ph->p_offset ||
        ph->p_vaddr > UINT64_MAX - ph->p_memsz) {
      fprintf(stderr, "POLYEXEC_FAIL: invalid AArch64 vDSO load segment: %s\n",
        path);
      munmap(file_image, file_size);
      return -1;
    }
    const uint64_t seg_start = ph->p_vaddr & ~0xfffULL;
    const uint64_t seg_end = align_up_u64(ph->p_vaddr + ph->p_memsz, 4096);
    if (seg_start < min_vaddr)
      min_vaddr = seg_start;
    if (seg_end > max_vaddr)
      max_vaddr = seg_end;
  }
  if (min_vaddr != 0 || max_vaddr == 0 || max_vaddr > SIZE_MAX) {
    fprintf(stderr,
      "POLYEXEC_FAIL: unsupported AArch64 vDSO virtual address span: %s\n",
      path);
    munmap(file_image, file_size);
    return -1;
  }

  uint8_t *mapping = mmap(NULL, (size_t) max_vaddr,
    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: AArch64 vDSO image mmap failed %s: %s\n",
      path, strerror(errno));
    munmap(file_image, file_size);
    return -1;
  }
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *ph = &phdrs[n];
    if (ph->p_type != PT_LOAD)
      continue;
    memcpy(mapping + ph->p_vaddr, file_image + ph->p_offset, ph->p_filesz);
  }
  uint64_t rt_sigreturn_offset = 0;
  if (lookup_elf_dynsym_value_by_section(file_image, file_size,
        "__kernel_rt_sigreturn", &rt_sigreturn_offset) == 0)
    poly_aarch64_vdso_rt_sigreturn =
      (uint64_t) (uintptr_t) mapping + rt_sigreturn_offset;
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *ph = &phdrs[n];
    if (ph->p_type != PT_LOAD)
      continue;
    const uint64_t seg_start = ph->p_vaddr & ~0xfffULL;
    const uint64_t seg_end = align_up_u64(ph->p_vaddr + ph->p_memsz, 4096);
    int prot = 0;
    if (ph->p_flags & PF_R)
      prot |= PROT_READ;
    if (ph->p_flags & PF_W)
      prot |= PROT_WRITE;
    if (ph->p_flags & PF_X)
      prot |= PROT_EXEC;
    if (mprotect(mapping + seg_start, (size_t) (seg_end - seg_start),
          prot) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: AArch64 vDSO mprotect failed %s: %s\n",
        path, strerror(errno));
      munmap(mapping, (size_t) max_vaddr);
      munmap(file_image, file_size);
      return -1;
    }
  }
  munmap(file_image, file_size);

  *at_sysinfo_ehdr_out = (uint64_t) (uintptr_t) mapping;
  printf("POLYEXEC_VDSO_MAP: arch=aarch64 path=%s addr=0x%llx sigreturn=0x%llx bytes=%zu\n",
    path, (unsigned long long) *at_sysinfo_ehdr_out,
    (unsigned long long) poly_aarch64_vdso_rt_sigreturn, file_size);
  return 0;
}

static int build_process_stack(const struct poly_program *program,
    const struct poly_request *request, const uint8_t *loaded_image,
    uint64_t at_base, uint64_t at_sysinfo_ehdr, int extra_argc,
    char **extra_argv, uint8_t **stack_out, size_t *stack_size_out,
    uint64_t *initial_sp_out) {
  const size_t stack_size = 1024 * 1024;
  uint8_t *stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (stack == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: process stack mmap failed: %s\n",
      strerror(errno));
    return -1;
  }
  memset(stack, 0, stack_size);

  size_t envc = 0;
  while (environ[envc])
    envc++;

  const size_t argc = (size_t) extra_argc + 1;
  uint64_t *argv_ptrs = calloc(argc, sizeof(*argv_ptrs));
  uint64_t *env_ptrs = calloc(envc ? envc : 1, sizeof(*env_ptrs));
  uint64_t execfn_ptr = 0;
  uint64_t platform_ptr = 0;
  uint64_t random_ptr = 0;
  if (!argv_ptrs || !env_ptrs) {
    fprintf(stderr, "POLYEXEC_FAIL: out of memory building process stack\n");
    free(argv_ptrs);
    free(env_ptrs);
    munmap(stack, stack_size);
    return -1;
  }

  uint8_t *cursor = stack + stack_size;
  const char *platform_name = program->arch == POLY_ARCH_AARCH64 ?
    "aarch64" : program->arch == POLY_ARCH_RISCV ? "riscv64" : "x86_64";
  if (copy_stack_string(stack, &cursor, platform_name, &platform_ptr) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: process platform does not fit stack\n");
    free(argv_ptrs);
    free(env_ptrs);
    munmap(stack, stack_size);
    return -1;
  }
  if ((size_t) (cursor - stack) < 16) {
    fprintf(stderr, "POLYEXEC_FAIL: process random bytes do not fit stack\n");
    free(argv_ptrs);
    free(env_ptrs);
    munmap(stack, stack_size);
    return -1;
  }
  cursor -= 16;
  if (syscall(SYS_getrandom, cursor, 16, GRND_NONBLOCK) != 16) {
    for (size_t n = 0; n < 16; n++)
      cursor[n] = (uint8_t) (0xa5U ^ (uint8_t) n ^ (uint8_t) getpid());
  }
  random_ptr = (uint64_t) (uintptr_t) cursor;
  if (copy_stack_string(stack, &cursor, request->path, &argv_ptrs[0]) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: process argv0 does not fit stack\n");
    free(argv_ptrs);
    free(env_ptrs);
    munmap(stack, stack_size);
    return -1;
  }
  for (int n = 0; n < extra_argc; n++) {
    if (copy_stack_string(stack, &cursor, extra_argv[n],
          &argv_ptrs[(size_t) n + 1]) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: process argv does not fit stack\n");
      free(argv_ptrs);
      free(env_ptrs);
      munmap(stack, stack_size);
      return -1;
    }
  }
  for (size_t n = 0; n < envc; n++) {
    if (copy_stack_string(stack, &cursor, environ[n], &env_ptrs[n]) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: process env does not fit stack\n");
      free(argv_ptrs);
      free(env_ptrs);
      munmap(stack, stack_size);
      return -1;
    }
  }
  execfn_ptr = argv_ptrs[0];

  const uint64_t load_bias =
    (uint64_t) (uintptr_t) loaded_image - program->base_vaddr;
  const uint64_t phdr_addr = program->phdr_vaddr ?
    load_bias + program->phdr_vaddr : 0;
  const uint64_t entry_addr = load_bias + program->base_vaddr +
    (uint64_t) program->entry_offset;
  long clock_tick = sysconf(_SC_CLK_TCK);
  if (clock_tick <= 0)
    clock_tick = 100;
  uint64_t hwcap = 0;
  uint64_t hwcap2 = 0;
  if (program->arch == POLY_ARCH_AARCH64) {
    hwcap = POLY_AARCH64_HWCAP_FP |
      POLY_AARCH64_HWCAP_ASIMD |
      POLY_AARCH64_HWCAP_ATOMICS |
      POLY_AARCH64_HWCAP_CPUID;
    int override = parse_u64_env("POLYEXEC_AARCH64_HWCAP", &hwcap);
    if (override < 0) {
      free(argv_ptrs);
      free(env_ptrs);
      munmap(stack, stack_size);
      return -1;
    }
  } else if (program->arch == POLY_ARCH_RISCV) {
    hwcap = POLY_RISCV_HWCAP_ISA_A |
      POLY_RISCV_HWCAP_ISA_C |
      POLY_RISCV_HWCAP_ISA_D |
      POLY_RISCV_HWCAP_ISA_F |
      POLY_RISCV_HWCAP_ISA_I |
      POLY_RISCV_HWCAP_ISA_M;
  }
  const struct {
    uint64_t type;
    uint64_t value;
  } auxv[] = {
    { AT_PHDR, phdr_addr },
    { AT_PHENT, program->phent },
    { AT_PHNUM, program->phnum },
    { AT_PAGESZ, 4096 },
    { AT_BASE, at_base },
    { AT_FLAGS, 0 },
    { AT_ENTRY, entry_addr },
    { AT_CLKTCK, (uint64_t) clock_tick },
    { AT_HWCAP, hwcap },
    { AT_HWCAP2, hwcap2 },
    { AT_UID, (uint64_t) getuid() },
    { AT_EUID, (uint64_t) geteuid() },
    { AT_GID, (uint64_t) getgid() },
    { AT_EGID, (uint64_t) getegid() },
    { AT_SECURE, 0 },
    { AT_RANDOM, random_ptr },
    { AT_EXECFN, execfn_ptr },
    { AT_PLATFORM, platform_ptr },
    { AT_SYSINFO_EHDR, at_sysinfo_ehdr },
    { AT_NULL, 0 }
  };

  const size_t table_words = 1 + argc + 1 + envc + 1 +
    (sizeof(auxv) / sizeof(auxv[0])) * 2;
  const size_t table_bytes = table_words * sizeof(uint64_t);
  uintptr_t sp_addr = ((uintptr_t) cursor - table_bytes) & ~(uintptr_t) 15;
  if (sp_addr < (uintptr_t) stack) {
    fprintf(stderr, "POLYEXEC_FAIL: process initial stack table overflow\n");
    free(argv_ptrs);
    free(env_ptrs);
    munmap(stack, stack_size);
    return -1;
  }

  uint64_t *sp = (uint64_t *) sp_addr;
  size_t out = 0;
  sp[out++] = argc;
  for (size_t n = 0; n < argc; n++)
    sp[out++] = argv_ptrs[n];
  sp[out++] = 0;
  for (size_t n = 0; n < envc; n++)
    sp[out++] = env_ptrs[n];
  sp[out++] = 0;
  for (size_t n = 0; n < sizeof(auxv) / sizeof(auxv[0]); n++) {
    sp[out++] = auxv[n].type;
    sp[out++] = auxv[n].value;
  }

  free(argv_ptrs);
  free(env_ptrs);
  *stack_out = stack;
  *stack_size_out = stack_size;
  *initial_sp_out = (uint64_t) sp_addr;
  return 0;
}

static int emit_poly_trampoline(const struct poly_program *program,
    uint8_t *code, size_t prefix_size, uint64_t return_pc,
    uint64_t target_pc, int tail_entry) {
  size_t offset = 0;
  if (program->arch == POLY_ARCH_AARCH64) {
    emit_x86_penter_frontend(code, &offset, POLY_ARCH_AARCH64);
    emit_u32(code, &offset, aarch64_adr(30,
      (int64_t) return_pc - (int64_t) (uintptr_t) (code + offset)));
    emit_aarch64_movabs(code, &offset, 16, target_pc);
    emit_u32(code, &offset, 0xd61f0200U); // br x16
  }
  else if (program->arch == POLY_ARCH_RISCV) {
    emit_x86_penter_frontend(code, &offset, POLY_ARCH_RISCV);
    int64_t escape_offset =
      (int64_t) return_pc - (int64_t) (uintptr_t) (code + offset);
    emit_u32(code, &offset, riscv_auipc(1, escape_offset));
    emit_u32(code, &offset, riscv_addi(1, 1, escape_offset));
    int64_t target_offset =
      (int64_t) target_pc - (int64_t) (uintptr_t) (code + offset);
    emit_u32(code, &offset, riscv_auipc(5, target_offset));
    emit_u32(code, &offset, riscv_addi(5, 5, target_offset));
    emit_u32(code, &offset, riscv_jalr(0, 5, 0));
  }
  else if (program->arch == POLY_ARCH_X86) {
    emit_x86_movabs_r11(code, &offset, target_pc);
    code[offset++] = 0x41;
    code[offset++] = 0xff;
    code[offset++] = tail_entry ? 0xe3 : 0xd3; // jmp/call *r11
    if (!tail_entry)
      code[offset++] = 0xc3; // ret to the x86 runtime caller.
    else
      code[offset++] = 0x90; // pad to the shared x86 trampoline size.
  }
  if (offset != prefix_size) {
    fprintf(stderr, "POLYEXEC_FAIL: internal trampoline size mismatch: %s\n",
      program->path);
    return -1;
  }
  return 0;
}

static size_t poly_trampoline_prefix_size(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return x86_penter_frontend_size_at(0, POLY_ARCH_AARCH64) + 4 + 20;
  if (arch == POLY_ARCH_RISCV)
    return x86_penter_frontend_size_at(0, POLY_ARCH_RISCV) + 8 + 12;
  if (arch == POLY_ARCH_X86)
    return POLY_X86_TRAMPOLINE_SIZE;
  return 0;
}

static uint32_t aarch64_adr(unsigned rd, int64_t byte_offset) {
  uint32_t imm = (uint32_t) byte_offset & 0x1fffffU;
  return 0x10000000U | ((imm & 0x3U) << 29) | (((imm >> 2) & 0x7ffffU) << 5) | (rd & 0x1fU);
}

static uint32_t riscv_auipc(unsigned rd, int64_t byte_offset) {
  int64_t hi20 = (byte_offset + 0x800) >> 12;
  return (((uint32_t) hi20 & 0xfffffU) << 12) | ((rd & 0x1fU) << 7) | 0x17U;
}

static uint32_t riscv_addi(unsigned rd, unsigned rs1, int64_t byte_offset) {
  int64_t hi20 = (byte_offset + 0x800) >> 12;
  int64_t lo12 = byte_offset - (hi20 << 12);
  return (((uint32_t) lo12 & 0xfffU) << 20) |
    ((rs1 & 0x1fU) << 15) | ((rd & 0x1fU) << 7) | 0x13U;
}

static uint32_t riscv_jalr(unsigned rd, unsigned rs1, int16_t byte_offset) {
  return (((uint32_t) byte_offset & 0xfffU) << 20) |
    ((rs1 & 0x1fU) << 15) | ((rd & 0x1fU) << 7) | 0x67U;
}

static uint32_t aarch64_pcall_sig_imm(uint32_t slot) {
  return POLY_AARCH64_CTRL_CALL_SIG_IMM(slot);
}

static uint32_t aarch64_lsr_imm(uint32_t rd, uint32_t rn, uint32_t shift) {
  return 0xd340fc00U | ((shift & 63U) << 16) | (rn << 5) | rd;
}

static uint32_t aarch64_lsl_imm(uint32_t rd, uint32_t rn, uint32_t shift) {
  const uint32_t immr = (64U - shift) & 63U;
  const uint32_t imms = 63U - shift;
  return 0xd3400000U | (immr << 16) | (imms << 10) | (rn << 5) | rd;
}

static uint32_t aarch64_orr_reg(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0xaa000000U | (rm << 16) | (rn << 5) | rd;
}

static uint32_t aarch64_fmov_s_from_w(uint32_t rd, uint32_t rn) {
  return 0x1e270000U | (rn << 5) | rd;
}

static uint32_t aarch64_fmov_w_from_s(uint32_t rd, uint32_t rn) {
  return 0x1e260000U | (rn << 5) | rd;
}

static uint32_t riscv_pcall_sig_imm(uint32_t slot) {
  return POLY_RISCV_CTRL_CALL_SIG_IMM(slot);
}

static int emit_aarch64_pcall_sig(uint8_t *code, size_t *offset,
    uint32_t signature_slot) {
  if (signature_slot < POLY_ABI_SIGNATURE_SLOT_COUNT) {
    emit_u32(code, offset, aarch64_pcall_sig_imm(signature_slot));
    return 0;
  }
  return -1;
}

static int emit_riscv_pcall_sig(uint8_t *code, size_t *offset,
    uint32_t signature_slot) {
  if (signature_slot < POLY_ABI_SIGNATURE_SLOT_COUNT) {
    emit_u32(code, offset, riscv_pcall_sig_imm(signature_slot));
    return 0;
  }
  return -1;
}

static uint32_t riscv_srli(uint32_t rd, uint32_t rs1, uint32_t shamt) {
  return 0x00005013U | ((shamt & 63U) << 20) | (rs1 << 15) |
    (rd << 7);
}

static uint32_t riscv_slli(uint32_t rd, uint32_t rs1, uint32_t shamt) {
  return 0x00001013U | ((shamt & 63U) << 20) | (rs1 << 15) |
    (rd << 7);
}

static uint32_t riscv_or(unsigned rd, unsigned rs1, unsigned rs2) {
  return 0x00006033U | (rs2 << 20) | (rs1 << 15) | (rd << 7);
}

static uint32_t riscv_fmv_x_w(uint32_t rd, uint32_t rs1) {
  return 0xe0000053U | (rs1 << 15) | (rd << 7);
}

static uint32_t riscv_fmv_w_x(uint32_t rd, uint32_t rs1) {
  return 0xf0000053U | (rs1 << 15) | (rd << 7);
}

static uint32_t riscv_ld(unsigned rd, unsigned rs1, int16_t byte_offset) {
  return (((uint32_t) byte_offset & 0xfffU) << 20) |
    ((rs1 & 0x1fU) << 15) | (3U << 12) | ((rd & 0x1fU) << 7) | 0x03U;
}

static uint32_t riscv_sd(unsigned rs2, unsigned rs1, int16_t byte_offset) {
  const uint32_t imm = (uint32_t) byte_offset & 0xfffU;
  return ((imm >> 5) << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | (3U << 12) |
    ((imm & 0x1fU) << 7) | 0x23U;
}

static uint32_t riscv_add(unsigned rd, unsigned rs1, unsigned rs2) {
  return ((rs2 & 0x1fU) << 20) | ((rs1 & 0x1fU) << 15) |
    ((rd & 0x1fU) << 7) | 0x33U;
}

static void emit_aarch64_movabs(uint8_t *code, size_t *offset, uint32_t rd,
    uint64_t value) {
  emit_u32(code, offset, 0xd2800000U |
    (((uint32_t) value & 0xffffU) << 5) | (rd & 0x1fU));
  emit_u32(code, offset, 0xf2a00000U |
    ((((uint32_t) (value >> 16)) & 0xffffU) << 5) | (rd & 0x1fU));
  emit_u32(code, offset, 0xf2c00000U |
    ((((uint32_t) (value >> 32)) & 0xffffU) << 5) | (rd & 0x1fU));
  emit_u32(code, offset, 0xf2e00000U |
    ((((uint32_t) (value >> 48)) & 0xffffU) << 5) | (rd & 0x1fU));
}

static int emit_poly_resolver_trampoline(const struct poly_program *program,
    uint8_t *code, size_t code_size, uint64_t return_pc,
    uint64_t target_pc) {
  (void) return_pc;
  size_t offset = 0;
  if (program->arch == POLY_ARCH_AARCH64) {
    const size_t penter_size = x86_penter_frontend_size_at(20,
      POLY_ARCH_AARCH64);
    const uint64_t resolver_return_pc =
      (uint64_t) (uintptr_t) (code + 20 + penter_size + 8);
    if (code_size < 20 + penter_size + 8 + 4 + 1)
      return -1;
    code[offset++] = 0x48; // movabs rax,target_pc -> AArch64 x0
    code[offset++] = 0xb8;
    emit_u64(code, &offset, target_pc);
    code[offset++] = 0x48; // movabs rdx,return_pc -> AArch64 x1
    code[offset++] = 0xba;
    emit_u64(code, &offset, resolver_return_pc);
    emit_x86_penter_frontend(code, &offset, POLY_ARCH_AARCH64);
    emit_u32(code, &offset, 0xaa0103feU); // mov x30,x1
    emit_u32(code, &offset, 0xd61f0000U); // br x0
    emit_u32(code, &offset, POLY_AARCH64_CTRL_X86_ESCAPE);
    code[offset++] = 0xc3;
    return 0;
  }

  if (program->arch == POLY_ARCH_RISCV) {
    const size_t penter_size = x86_penter_frontend_size_at(20,
      POLY_ARCH_RISCV);
    const uint64_t resolver_return_pc =
      (uint64_t) (uintptr_t) (code + 20 + penter_size + 8);
    if (code_size < 20 + penter_size + 8 + 4 + 1)
      return -1;
    code[offset++] = 0x48; // movabs rax,target_pc -> RISC-V a0
    code[offset++] = 0xb8;
    emit_u64(code, &offset, target_pc);
    code[offset++] = 0x48; // movabs rdx,return_pc -> RISC-V a1
    code[offset++] = 0xba;
    emit_u64(code, &offset, resolver_return_pc);
    emit_x86_penter_frontend(code, &offset, POLY_ARCH_RISCV);
    emit_u32(code, &offset, riscv_addi(1, 11, 0)); // mv ra,a1
    emit_u32(code, &offset, riscv_jalr(0, 10, 0)); // jr a0
    emit_u32(code, &offset, POLY_RISCV_CTRL_X86_ESCAPE);
    code[offset++] = 0xc3;
    return 0;
  }

  return -1;
}

static int ensure_process_cross_stub_arena(void) {
  if (process_cross_stubs.mapping)
    return 0;
  process_cross_stubs.size = PROCESS_CROSS_STUB_BYTES;
  process_cross_stubs.offset = 0;
  process_cross_stubs.mapping = mmap(NULL, process_cross_stubs.size,
    PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (process_cross_stubs.mapping == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: cross-ISA stub mmap failed: %s\n",
      strerror(errno));
    process_cross_stubs.mapping = NULL;
    process_cross_stubs.size = 0;
    return -1;
  }
  return 0;
}

static int process_bridge_is_compact(int bridge_kind);
static int process_bridge_is_stack_thunk(int bridge_kind);

static void note_process_cross_isa_call_stub(int caller_arch, int callee_arch,
    int bridge_kind, uint32_t signature_slot, int emitted_x86_wrapper) {
  process_cross_state_key_stub_count++;
  process_cross_signature_slot_stub_count++;
  if (process_bridge_is_stack_thunk(bridge_kind))
    process_cross_stack_bridge_stub_count++;
  else if (process_bridge_is_compact(bridge_kind))
    process_cross_compact_shuffle_stub_count++;
  else
    process_cross_register_signature_stub_count++;
  if (emitted_x86_wrapper)
    process_cross_x86_wrapper_stub_count++;

  if (caller_arch == POLY_ARCH_AARCH64 && callee_arch == POLY_ARCH_RISCV)
    process_cross_aarch64_to_riscv_stub_count++;
  else if (caller_arch == POLY_ARCH_RISCV && callee_arch == POLY_ARCH_AARCH64)
    process_cross_riscv_to_aarch64_stub_count++;
  else if (caller_arch == POLY_ARCH_AARCH64 && callee_arch == POLY_ARCH_X86)
    process_cross_aarch64_to_x86_stub_count++;
  else if (caller_arch == POLY_ARCH_RISCV && callee_arch == POLY_ARCH_X86)
    process_cross_riscv_to_x86_stub_count++;
  else if (caller_arch == POLY_ARCH_X86 && callee_arch == POLY_ARCH_AARCH64)
    process_cross_x86_to_aarch64_stub_count++;
  else if (caller_arch == POLY_ARCH_X86 && callee_arch == POLY_ARCH_RISCV)
    process_cross_x86_to_riscv_stub_count++;

  if (!process_cross_state_key_stub_reported) {
    printf("POLYEXEC_CROSS_STUB_STATE_KEY: explicit=%u\n",
      polyexec_use_explicit_state_key ? 1U : 0U);
    process_cross_state_key_stub_reported = 1;
  }
  printf("POLYEXEC_CROSS_STUBS: a64_to_rv=%zu rv_to_a64=%zu a64_to_x86=%zu rv_to_x86=%zu x86_to_a64=%zu x86_to_rv=%zu total=%zu path=%s sig_slots=%zu reg_sig=%zu stack_bridges=%zu compact_shuffles=%zu x86_wrappers=%zu bridge=%d slot=%u\n",
    process_cross_aarch64_to_riscv_stub_count,
    process_cross_riscv_to_aarch64_stub_count,
    process_cross_aarch64_to_x86_stub_count,
    process_cross_riscv_to_x86_stub_count,
    process_cross_x86_to_aarch64_stub_count,
    process_cross_x86_to_riscv_stub_count,
    process_cross_state_key_stub_count,
    process_cross_report_path ? process_cross_report_path : "(unknown)",
    process_cross_signature_slot_stub_count,
    process_cross_register_signature_stub_count,
    process_cross_stack_bridge_stub_count,
    process_cross_compact_shuffle_stub_count,
    process_cross_x86_wrapper_stub_count,
    bridge_kind,
    signature_slot);
  fflush(NULL);
}

static int process_bridge_is_compact(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_U32_F32 ||
    bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_F32_U32;
}

static int process_bridge_is_aarch64_hfa32_ret(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET ||
    bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET;
}

static int process_bridge_is_aarch64_hfa64_ret(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET ||
    bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET;
}

static int process_bridge_is_aarch64_hfa64_arg(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_ARG ||
    bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG;
}

static uint32_t process_bridge_aarch64_hfa64_ret_count(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET ? 4U : 3U;
}

static uint32_t process_bridge_aarch64_hfa64_arg_count(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG ? 4U : 3U;
}

static int process_bridge_is_aarch64_hfa32_arg(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG ||
    bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG;
}

static int process_bridge_is_fpair(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_FPAIR32_ARG ||
    bridge_kind == POLY_PROCESS_BRIDGE_FPAIR64_ARG ||
    bridge_kind == POLY_PROCESS_BRIDGE_FPAIR32_RET ||
    bridge_kind == POLY_PROCESS_BRIDGE_FPAIR64_RET;
}

static int process_bridge_is_fpair32(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_FPAIR32_ARG ||
    bridge_kind == POLY_PROCESS_BRIDGE_FPAIR32_RET;
}

static int process_bridge_is_fpair64(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_FPAIR64_ARG ||
    bridge_kind == POLY_PROCESS_BRIDGE_FPAIR64_RET;
}

static int process_bridge_is_sret(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_SRET_X86_SYSV;
}

static int process_bridge_is_native_sret(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_NATIVE_SRET;
}

static int process_bridge_is_x86_source_signature(int bridge_kind) {
  return process_bridge_is_aarch64_hfa32_ret(bridge_kind) ||
    process_bridge_is_aarch64_hfa64_ret(bridge_kind) ||
    process_bridge_is_aarch64_hfa32_arg(bridge_kind) ||
    process_bridge_is_fpair(bridge_kind) ||
    process_bridge_is_sret(bridge_kind);
}

static int process_bridge_is_stack_thunk(int bridge_kind) {
  return bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9 ||
    process_bridge_is_aarch64_hfa64_ret(bridge_kind) ||
    process_bridge_is_aarch64_hfa64_arg(bridge_kind);
}

static int process_bridge_needs_foreign_signature_set(int caller_arch,
    int callee_arch, int bridge_kind) {
  return caller_arch != POLY_ARCH_X86 &&
    callee_arch == POLY_ARCH_X86 &&
    process_bridge_is_fpair(bridge_kind);
}

static int process_bridge_is_direct_foreign_fpair(int caller_arch,
    int callee_arch, int bridge_kind) {
  return caller_arch != POLY_ARCH_X86 &&
    callee_arch != POLY_ARCH_X86 &&
    caller_arch != callee_arch &&
    process_bridge_is_fpair(bridge_kind);
}

static uint32_t process_bridge_signature_kind(int bridge_kind) {
  if (bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_FPAIR32_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_FPAIR64_ARG ||
      bridge_kind == POLY_PROCESS_BRIDGE_FPAIR32_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_FPAIR64_RET ||
      bridge_kind == POLY_PROCESS_BRIDGE_SRET_X86_SYSV ||
      bridge_kind == POLY_PROCESS_BRIDGE_NATIVE_SRET)
    return poly_legacy_bridge_signature_kind(bridge_kind);
  return UINT32_MAX;
}

static void emit_x86_abi_signature_set(uint8_t *code, size_t *offset,
    uint32_t slot, uint32_t kind) {
  const uint64_t value = poly_abi_signature_control_value(kind);
  code[(*offset)++] = 0x50; // push rax
  code[(*offset)++] = 0x52; // push rdx
  code[(*offset)++] = 0xb8; // mov eax,slot
  emit_u32(code, offset, slot);
  code[(*offset)++] = 0x48; // movabs rdx,value
  code[(*offset)++] = 0xba;
  emit_u64(code, offset, value);
  emit_x86_poly_control(code, offset, POLY_X86_CTRL_ABI_SIGNATURE_SET);
  code[(*offset)++] = 0x5a; // pop rdx
  code[(*offset)++] = 0x58; // pop rax
}

static void emit_process_aarch64_abi_signature_set(uint8_t *code,
    size_t *offset, uint32_t slot, uint32_t kind) {
  emit_aarch64_movabs(code, offset, 0, slot);
  emit_aarch64_movabs(code, offset, 1,
    poly_abi_signature_control_value(kind));
  emit_u32(code, offset, POLY_AARCH64_CTRL_ABI_SIGNATURE_SET);
}

static void emit_process_riscv_abi_signature_set(uint8_t *code,
    size_t *offset, uint32_t slot, uint32_t kind) {
  const uint32_t register_map = poly_abi_signature_register_map(kind);
  emit_u32(code, offset, riscv_addi(10, 0, slot));
  emit_u32(code, offset, riscv_addi(11, 0, register_map));
  emit_u32(code, offset, riscv_slli(11, 11, 32));
  emit_u32(code, offset, riscv_addi(28, 0, kind));
  emit_u32(code, offset, riscv_or(11, 11, 28));
  emit_u32(code, offset, POLY_RISCV_CTRL_ABI_SIGNATURE_SET);
}

static void emit_process_aarch64_compact_pre_pcall(uint8_t *code,
    size_t *offset, int bridge_kind) {
  if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_U32_F32) {
    emit_u32(code, offset, aarch64_lsr_imm(9, 0, 32));
    emit_u32(code, offset, aarch64_fmov_s_from_w(0, 9));
  }
  else if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_F32_U32) {
    emit_u32(code, offset, aarch64_fmov_s_from_w(0, 0));
    emit_u32(code, offset, aarch64_lsr_imm(0, 0, 32));
  }
}

static void emit_process_aarch64_compact_post_pcall(uint8_t *code,
    size_t *offset, int bridge_kind) {
  if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_U32_F32) {
    emit_u32(code, offset, aarch64_fmov_w_from_s(9, 0));
    emit_u32(code, offset, aarch64_lsl_imm(9, 9, 32));
    emit_u32(code, offset, aarch64_orr_reg(0, 0, 9));
  }
  else if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_F32_U32) {
    emit_u32(code, offset, aarch64_fmov_w_from_s(9, 0));
    emit_u32(code, offset, aarch64_lsl_imm(0, 0, 32));
    emit_u32(code, offset, aarch64_orr_reg(0, 0, 9));
  }
}

static void emit_process_riscv_compact_pre_pcall(uint8_t *code,
    size_t *offset, int bridge_kind) {
  if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_U32_F32) {
    emit_u32(code, offset, riscv_fmv_x_w(28, 10));
    emit_u32(code, offset, riscv_slli(28, 28, 32));
    emit_u32(code, offset, riscv_or(10, 10, 28));
  }
  else if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_F32_U32) {
    emit_u32(code, offset, riscv_fmv_x_w(28, 10));
    emit_u32(code, offset, riscv_slli(10, 10, 32));
    emit_u32(code, offset, riscv_or(10, 10, 28));
  }
}

static void emit_process_riscv_compact_post_pcall(uint8_t *code,
    size_t *offset, int bridge_kind) {
  if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_U32_F32) {
    emit_u32(code, offset, riscv_srli(28, 10, 32));
    emit_u32(code, offset, riscv_fmv_w_x(10, 28));
  }
  else if (bridge_kind == POLY_PROCESS_BRIDGE_COMPACT_F32_U32) {
    emit_u32(code, offset, riscv_fmv_w_x(10, 10));
    emit_u32(code, offset, riscv_srli(10, 10, 32));
  }
}

static void emit_x86_set_fs_from_global(uint8_t *code, size_t *offset,
    uint64_t global_addr) {
  emit_x86_movabs_rax(code, offset, global_addr);
  code[(*offset)++] = 0x48; // mov rsi,[rax]
  code[(*offset)++] = 0x8b;
  code[(*offset)++] = 0x30;
  code[(*offset)++] = 0xb8; // mov eax,SYS_arch_prctl
  emit_u32(code, offset, (uint32_t) SYS_arch_prctl);
  code[(*offset)++] = 0xbf; // mov edi,ARCH_SET_FS
  emit_u32(code, offset, (uint32_t) ARCH_SET_FS);
  code[(*offset)++] = 0x0f;
  code[(*offset)++] = 0x05; // syscall
}

static int emit_process_x86_tls_call_wrapper(uint64_t target,
    int bridge_kind, uint64_t *wrapper_addr) {
  if (ensure_process_cross_stub_arena() < 0 ||
      align_up_size(process_cross_stubs.offset, 8,
        &process_cross_stubs.offset) < 0)
    return -1;
  if (process_cross_stubs.size - process_cross_stubs.offset < 192)
    return -1;

  uint8_t *code = process_cross_stubs.mapping;
  size_t offset = process_cross_stubs.offset;
  *wrapper_addr = (uint64_t) (uintptr_t) (code + offset);

  code[offset++] = 0x41; // push r11: direct foreign->x86 PCALL exposes the source SP here.
  code[offset++] = 0x53;
  code[offset++] = 0x57; // push rdi
  code[offset++] = 0x56; // push rsi
  code[offset++] = 0x52; // push rdx
  code[offset++] = 0x51; // push rcx
  code[offset++] = 0x41; // push r8
  code[offset++] = 0x50;
  code[offset++] = 0x41; // push r9
  code[offset++] = 0x51;
  emit_x86_movabs_rax(code, &offset,
    (uint64_t) (uintptr_t) &process_runtime_x86_tls_base);
  code[offset++] = 0x48; // mov rsi,[rax]
  code[offset++] = 0x8b;
  code[offset++] = 0x30;
  code[offset++] = 0x48; // test rsi,rsi
  code[offset++] = 0x85;
  code[offset++] = 0xf6;
  code[offset++] = 0x74; // jz skip_set
  const size_t skip_set_disp = offset++;
  code[offset++] = 0xb8; // mov eax,SYS_arch_prctl
  emit_u32(code, &offset, (uint32_t) SYS_arch_prctl);
  code[offset++] = 0xbf; // mov edi,ARCH_SET_FS
  emit_u32(code, &offset, (uint32_t) ARCH_SET_FS);
  code[offset++] = 0x0f;
  code[offset++] = 0x05; // syscall
  code[skip_set_disp] = (uint8_t) (offset - (skip_set_disp + 1));
  code[offset++] = 0x41; // pop r9
  code[offset++] = 0x59;
  code[offset++] = 0x41; // pop r8
  code[offset++] = 0x58;
  code[offset++] = 0x59; // pop rcx
  code[offset++] = 0x5a; // pop rdx
  code[offset++] = 0x5e; // pop rsi
  code[offset++] = 0x5f; // pop rdi
  code[offset++] = 0x41; // pop r11
  code[offset++] = 0x5b;
  if (bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9) {
    code[offset++] = 0x48; // sub rsp,24: stack args 6..8, aligned for call.
    code[offset++] = 0x83;
    code[offset++] = 0xec;
    code[offset++] = 0x18;
    code[offset++] = 0x4d; // mov r10,[r11]
    code[offset++] = 0x8b;
    code[offset++] = 0x13;
    code[offset++] = 0x4c; // mov [rsp],r10
    code[offset++] = 0x89;
    code[offset++] = 0x14;
    code[offset++] = 0x24;
    code[offset++] = 0x4d; // mov r10,[r11+8]
    code[offset++] = 0x8b;
    code[offset++] = 0x53;
    code[offset++] = 0x08;
    code[offset++] = 0x4c; // mov [rsp+8],r10
    code[offset++] = 0x89;
    code[offset++] = 0x54;
    code[offset++] = 0x24;
    code[offset++] = 0x08;
    code[offset++] = 0x4d; // mov r10,[r11+16]
    code[offset++] = 0x8b;
    code[offset++] = 0x53;
    code[offset++] = 0x10;
    code[offset++] = 0x4c; // mov [rsp+16],r10
    code[offset++] = 0x89;
    code[offset++] = 0x54;
    code[offset++] = 0x24;
    code[offset++] = 0x10;
  }
  emit_x86_movabs_r11(code, &offset, target);
  code[offset++] = 0x41; // call r11
  code[offset++] = 0xff;
  code[offset++] = 0xd3;
  if (bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9) {
    code[offset++] = 0x48; // add rsp,24
    code[offset++] = 0x83;
    code[offset++] = 0xc4;
    code[offset++] = 0x18;
  }
  code[offset++] = 0x50; // push rax
  code[offset++] = 0x52; // push rdx
  emit_x86_movabs_rax(code, &offset,
    (uint64_t) (uintptr_t) &process_runtime_x86_tls_base);
  code[offset++] = 0x48; // mov rsi,[rax]
  code[offset++] = 0x8b;
  code[offset++] = 0x30;
  code[offset++] = 0x48; // test rsi,rsi
  code[offset++] = 0x85;
  code[offset++] = 0xf6;
  code[offset++] = 0x74; // jz skip_restore
  const size_t skip_restore_disp = offset++;
  emit_x86_set_fs_from_global(code, &offset,
    (uint64_t) (uintptr_t) &process_runtime_host_fs_base);
  code[skip_restore_disp] = (uint8_t) (offset - (skip_restore_disp + 1));
  code[offset++] = 0x5a; // pop rdx
  code[offset++] = 0x58; // pop rax
  code[offset++] = 0xc3; // ret

  process_cross_stubs.offset = offset;
  return 0;
}

static int emit_process_cross_isa_call_stub(int caller_arch, int callee_arch,
    uint64_t target, int bridge_kind, uint32_t signature_slot,
    uint64_t *stub_addr) {
  if (caller_arch == callee_arch) {
    *stub_addr = target;
    return 0;
  }
  if (!((caller_arch == POLY_ARCH_AARCH64 &&
          (callee_arch == POLY_ARCH_RISCV || callee_arch == POLY_ARCH_X86)) ||
        (caller_arch == POLY_ARCH_RISCV &&
          (callee_arch == POLY_ARCH_AARCH64 || callee_arch == POLY_ARCH_X86)) ||
        (caller_arch == POLY_ARCH_X86 &&
          (callee_arch == POLY_ARCH_AARCH64 || callee_arch == POLY_ARCH_RISCV))))
    return -1;
  if (process_bridge_is_aarch64_hfa32_ret(bridge_kind) &&
      !(caller_arch == POLY_ARCH_X86 && callee_arch == POLY_ARCH_AARCH64))
    return -1;
  if (process_bridge_is_aarch64_hfa64_ret(bridge_kind) &&
      !(caller_arch == POLY_ARCH_X86 && callee_arch == POLY_ARCH_AARCH64))
    return -1;
  if (process_bridge_is_aarch64_hfa64_arg(bridge_kind) &&
      !(caller_arch == POLY_ARCH_X86 && callee_arch == POLY_ARCH_AARCH64))
    return -1;
  if (process_bridge_is_aarch64_hfa32_arg(bridge_kind) &&
      !(caller_arch == POLY_ARCH_X86 && callee_arch == POLY_ARCH_AARCH64))
    return -1;
  if (process_bridge_is_fpair(bridge_kind) &&
      !((caller_arch == POLY_ARCH_X86 &&
          (callee_arch == POLY_ARCH_AARCH64 ||
           callee_arch == POLY_ARCH_RISCV)) ||
        (caller_arch != POLY_ARCH_X86 && callee_arch == POLY_ARCH_X86) ||
        (caller_arch == POLY_ARCH_AARCH64 &&
          callee_arch == POLY_ARCH_RISCV) ||
        (caller_arch == POLY_ARCH_RISCV &&
          callee_arch == POLY_ARCH_AARCH64)))
    return -1;
  if (process_bridge_is_sret(bridge_kind) &&
      !((caller_arch == POLY_ARCH_X86 &&
          (callee_arch == POLY_ARCH_AARCH64 ||
           callee_arch == POLY_ARCH_RISCV)) ||
        (caller_arch != POLY_ARCH_X86 && callee_arch == POLY_ARCH_X86)))
    return -1;
  if (process_bridge_is_native_sret(bridge_kind) &&
      !((caller_arch == POLY_ARCH_AARCH64 &&
          callee_arch == POLY_ARCH_RISCV) ||
        (caller_arch == POLY_ARCH_RISCV &&
          callee_arch == POLY_ARCH_AARCH64)))
    return -1;
  if (callee_arch == POLY_ARCH_X86 &&
      bridge_kind != POLY_PROCESS_BRIDGE_DEFAULT &&
      bridge_kind != POLY_PROCESS_BRIDGE_VEC128_U32 &&
      bridge_kind != POLY_PROCESS_BRIDGE_COMPACT_U32_F32 &&
      bridge_kind != POLY_PROCESS_BRIDGE_COMPACT_F32_U32 &&
      bridge_kind != POLY_PROCESS_BRIDGE_U64_STACK9 &&
      bridge_kind != POLY_PROCESS_BRIDGE_FP64 &&
      bridge_kind != POLY_PROCESS_BRIDGE_FP32 &&
      bridge_kind != POLY_PROCESS_BRIDGE_FPAIR32_ARG &&
      bridge_kind != POLY_PROCESS_BRIDGE_FPAIR64_ARG &&
      bridge_kind != POLY_PROCESS_BRIDGE_FPAIR32_RET &&
      bridge_kind != POLY_PROCESS_BRIDGE_FPAIR64_RET &&
      bridge_kind != POLY_PROCESS_BRIDGE_SRET_X86_SYSV)
    return -1;
  if (caller_arch == POLY_ARCH_X86 &&
      bridge_kind != POLY_PROCESS_BRIDGE_DEFAULT &&
      bridge_kind != POLY_PROCESS_BRIDGE_VEC128_U32 &&
      bridge_kind != POLY_PROCESS_BRIDGE_COMPACT_U32_F32 &&
      bridge_kind != POLY_PROCESS_BRIDGE_COMPACT_F32_U32 &&
      bridge_kind != POLY_PROCESS_BRIDGE_U64_STACK9 &&
      bridge_kind != POLY_PROCESS_BRIDGE_FP64 &&
      bridge_kind != POLY_PROCESS_BRIDGE_FP32 &&
      bridge_kind != POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET &&
      bridge_kind != POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET &&
      bridge_kind != POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET &&
      bridge_kind != POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET &&
      bridge_kind != POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_ARG &&
      bridge_kind != POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG &&
      bridge_kind != POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG &&
      bridge_kind != POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG &&
      bridge_kind != POLY_PROCESS_BRIDGE_FPAIR32_ARG &&
      bridge_kind != POLY_PROCESS_BRIDGE_FPAIR64_ARG &&
      bridge_kind != POLY_PROCESS_BRIDGE_FPAIR32_RET &&
      bridge_kind != POLY_PROCESS_BRIDGE_FPAIR64_RET &&
      bridge_kind != POLY_PROCESS_BRIDGE_SRET_X86_SYSV)
    return -1;
  if (bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9 &&
      caller_arch != POLY_ARCH_X86 &&
      callee_arch != POLY_ARCH_X86 &&
      !((caller_arch == POLY_ARCH_AARCH64 && callee_arch == POLY_ARCH_RISCV) ||
        (caller_arch == POLY_ARCH_RISCV && callee_arch == POLY_ARCH_AARCH64)))
    return -1;
  if (bridge_kind == POLY_PROCESS_BRIDGE_VEC128_U32 &&
      !((caller_arch == POLY_ARCH_X86 &&
          (callee_arch == POLY_ARCH_AARCH64 ||
           callee_arch == POLY_ARCH_RISCV)) ||
        (caller_arch != POLY_ARCH_X86 && callee_arch == POLY_ARCH_X86) ||
        (caller_arch == POLY_ARCH_AARCH64 &&
          callee_arch == POLY_ARCH_RISCV) ||
        (caller_arch == POLY_ARCH_RISCV &&
          callee_arch == POLY_ARCH_AARCH64)))
    return -1;
  if (ensure_process_cross_stub_arena() < 0 ||
      align_up_size(process_cross_stubs.offset, 8,
        &process_cross_stubs.offset) < 0)
    return -1;

  const int needs_x86_call_wrapper =
    caller_arch != POLY_ARCH_X86 &&
    callee_arch == POLY_ARCH_X86 &&
    (bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9 ||
      process_runtime_needs_x86_tls_wrapper);
  uint64_t pcall_target = target;
  if (needs_x86_call_wrapper) {
    if (emit_process_x86_tls_call_wrapper(target, bridge_kind,
          &pcall_target) < 0 ||
        align_up_size(process_cross_stubs.offset, 8,
          &process_cross_stubs.offset) < 0)
      return -1;
  }

  uint8_t *code = process_cross_stubs.mapping;
  const size_t start = process_cross_stubs.offset;
  const uint64_t start_addr = (uint64_t) (uintptr_t) (code + start);
  /*
   * Process cross-call stubs are shared by every guest thread.  The process
   * entry path and raw-clone handoff select the per-thread key; embedding the
   * creator thread's TLS anchor here would collapse all guest threads back
   * onto one frontend register bank.
   */
  const uint64_t state_key = 0;
  const int is_compact_bridge = process_bridge_is_compact(bridge_kind);
  const int emit_compact_shuffle =
    is_compact_bridge && callee_arch != POLY_ARCH_X86;
  const int is_stack9_x86_callee =
    bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9 &&
    callee_arch == POLY_ARCH_X86;
  if (is_stack9_x86_callee)
    signature_slot = process_native_signature_slot;
  if (process_bridge_is_direct_foreign_fpair(caller_arch, callee_arch,
        bridge_kind)) {
    if (process_bridge_is_fpair32(bridge_kind))
      signature_slot = process_fp32_signature_slot;
    else if (process_bridge_is_fpair64(bridge_kind))
      signature_slot = process_fp64_signature_slot;
    else
      return -1;
  }
  if (signature_slot >= POLY_ABI_SIGNATURE_SLOT_COUNT)
    return -1;
  const uint32_t callee_frontend = callee_arch == POLY_ARCH_AARCH64 ?
    POLY_ARCH_AARCH64 :
    callee_arch == POLY_ARCH_RISCV ? POLY_ARCH_RISCV : POLY_ARCH_X86;

  if (caller_arch == POLY_ARCH_X86) {
    if (process_cross_stubs.size - start < 224)
      return -1;
    size_t offset = start;
    code[offset++] = 0x53; // push rbx
    if (bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9) {
      code[offset++] = 0x41; // push r12
      code[offset++] = 0x54;
    }
    code[offset++] = 0x41; // push r15
    code[offset++] = 0x57;
    code[offset++] = 0x48; // sub rsp,8: keep the foreign ABI entry stack 16-byte aligned.
    code[offset++] = 0x83;
    code[offset++] = 0xec;
    code[offset++] = 0x08;
    code[offset++] = 0x50; // push rax: preserve arg0 and the existing stack layout.
    if (state_key != 0)
      emit_x86_state_key_set(code, &offset, state_key);
    code[offset++] = 0x58; // pop rax

    if (bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9) {
      // Translate x86_64 SysV u64 args 0..8 to the exchange register window:
      // RAX,RDX,RCX,RDI,RSI,R8,R9,R10 -> x0..x7/a0..a7, plus arg8 at [SP].
      code[offset++] = 0x4c; // mov r12,[rsp+56]
      code[offset++] = 0x8b;
      code[offset++] = 0x64;
      code[offset++] = 0x24;
      code[offset++] = 0x38;
      code[offset++] = 0x4c; // lea r11,[rsp-0x4000]
      code[offset++] = 0x8d;
      code[offset++] = 0x9c;
      code[offset++] = 0x24;
      emit_u32(code, &offset, 0xffffc000U);
      code[offset++] = 0x49; // and r11,-16
      code[offset++] = 0x83;
      code[offset++] = 0xe3;
      code[offset++] = 0xf0;
      code[offset++] = 0x4d; // mov [r11],r12
      code[offset++] = 0x89;
      code[offset++] = 0x23;
      code[offset++] = 0x49; // mov r11,rcx
      code[offset++] = 0x89;
      code[offset++] = 0xcb;
      code[offset++] = 0x49; // mov r12,rdx
      code[offset++] = 0x89;
      code[offset++] = 0xd4;
      code[offset++] = 0x48; // mov rax,rdi
      code[offset++] = 0x89;
      code[offset++] = 0xf8;
      code[offset++] = 0x48; // mov rdx,rsi
      code[offset++] = 0x89;
      code[offset++] = 0xf2;
      code[offset++] = 0x4c; // mov rcx,r12
      code[offset++] = 0x89;
      code[offset++] = 0xe1;
      code[offset++] = 0x4c; // mov rdi,r11
      code[offset++] = 0x89;
      code[offset++] = 0xdf;
      code[offset++] = 0x4c; // mov rsi,r8
      code[offset++] = 0x89;
      code[offset++] = 0xc6;
      code[offset++] = 0x4d; // mov r8,r9
      code[offset++] = 0x89;
      code[offset++] = 0xc8;
      code[offset++] = 0x4c; // mov r9,[rsp+40]
      code[offset++] = 0x8b;
      code[offset++] = 0x4c;
      code[offset++] = 0x24;
      code[offset++] = 0x28;
      code[offset++] = 0x4c; // mov r10,[rsp+48]
      code[offset++] = 0x8b;
      code[offset++] = 0x54;
      code[offset++] = 0x24;
      code[offset++] = 0x30;
      signature_slot = POLY_ABI_SIGNATURE_SLOT_EXCHANGE;
    }

    if (process_bridge_is_x86_source_signature(bridge_kind)) {
      const uint32_t signature_kind =
        process_bridge_signature_kind(bridge_kind);
      if (signature_kind == UINT32_MAX)
        return -1;
      emit_x86_abi_signature_set(code, &offset, signature_slot,
        signature_kind);
    }
    if (process_bridge_is_aarch64_hfa64_arg(bridge_kind))
      emit_x86_load_hfa64_arg_from_stack(code, &offset,
        process_bridge_aarch64_hfa64_arg_count(bridge_kind));
    if (process_bridge_is_aarch64_hfa64_ret(bridge_kind))
      emit_x86_preserve_sret_ptr_rax(code, &offset);

    emit_x86_movabs_rbx(code, &offset, target);
    const size_t return_imm_offset = offset + 2;
    emit_x86_movabs_r11(code, &offset, 0);
    emit_x86_pcall_sig_imm(code, &offset, callee_frontend, signature_slot);
    const uint64_t return_addr = (uint64_t) (uintptr_t) (code + offset);
    if (process_bridge_is_aarch64_hfa64_ret(bridge_kind))
      emit_x86_store_hfa64_return_to_sret(code, &offset,
        process_bridge_aarch64_hfa64_ret_count(bridge_kind));
    code[offset++] = 0x48; // add rsp,8
    code[offset++] = 0x83;
    code[offset++] = 0xc4;
    code[offset++] = 0x08;
    code[offset++] = 0x41; // pop r15
    code[offset++] = 0x5f;
    if (bridge_kind == POLY_PROCESS_BRIDGE_U64_STACK9) {
      code[offset++] = 0x41; // pop r12
      code[offset++] = 0x5c;
    }
    code[offset++] = 0x5b; // pop rbx
    code[offset++] = 0xc3; // ret
    for (unsigned n = 0; n < 8; n++)
      code[return_imm_offset + n] =
        (uint8_t) ((return_addr >> (n * 8)) & 0xff);
    process_cross_stubs.offset = offset;
    note_process_cross_isa_call_stub(caller_arch, callee_arch, bridge_kind,
      signature_slot, 0);
    *stub_addr = start_addr;
    return 0;
  }

  if (caller_arch == POLY_ARCH_AARCH64) {
    if (process_cross_stubs.size - start < 160)
      return -1;
    size_t offset = start;
    if (is_stack9_x86_callee) {
      emit_u32(code, &offset, 0xd100c3ffU); // sub sp, sp, #48
      emit_u32(code, &offset, 0xf9401bf2U); // ldr x18, [sp, #48]
      emit_u32(code, &offset, 0xf90003e6U); // str x6, [sp]
      emit_u32(code, &offset, 0xf90007e7U); // str x7, [sp, #8]
      emit_u32(code, &offset, 0xf9000bf2U); // str x18, [sp, #16]
      emit_u32(code, &offset, 0xf9000ffeU); // str x30, [sp, #24]
      emit_u32(code, &offset, 0xf90013e0U); // str x0, [sp, #32]
    }
    else {
      emit_u32(code, &offset, 0xd10083ffU); // sub sp, sp, #32
      emit_u32(code, &offset, 0xf94013f2U); // ldr x18, [sp, #32]
      emit_u32(code, &offset, 0xf90003f2U); // str x18, [sp]
      emit_u32(code, &offset, 0xf90007feU); // str x30, [sp, #8]
      emit_u32(code, &offset, 0xf9000be0U); // str x0, [sp, #16]
    }
    if (state_key != 0) {
      emit_aarch64_movabs(code, &offset, 0, state_key);
      emit_u32(code, &offset, POLY_AARCH64_CTRL_STATE_KEY_SET);
    }
    if (process_bridge_needs_foreign_signature_set(caller_arch, callee_arch,
          bridge_kind)) {
      const uint32_t signature_kind =
        process_bridge_signature_kind(bridge_kind);
      if (signature_kind == UINT32_MAX)
        return -1;
      emit_process_aarch64_abi_signature_set(code, &offset, signature_slot,
        signature_kind);
    }
    emit_u32(code, &offset,
      is_stack9_x86_callee ? 0xf94013e0U : 0xf9400be0U); // ldr x0,saved
    if (emit_compact_shuffle)
      emit_process_aarch64_compact_pre_pcall(code, &offset, bridge_kind);
    emit_aarch64_movabs(code, &offset, 16, pcall_target);
    emit_u32(code, &offset,
      0xd2800011U | ((callee_frontend & 0xffffU) << 5)); // movz x17,frontend
    const uint64_t return_addr =
      (uint64_t) (uintptr_t) (code + offset + 20);
    emit_aarch64_movabs(code, &offset, 18, return_addr);
    if (emit_aarch64_pcall_sig(code, &offset, signature_slot) < 0)
      return -1;
    if (emit_compact_shuffle)
      emit_process_aarch64_compact_post_pcall(code, &offset, bridge_kind);
    emit_u32(code, &offset,
      is_stack9_x86_callee ? 0xf9400ffeU : 0xf94007feU); // ldr x30,saved
    emit_u32(code, &offset,
      is_stack9_x86_callee ? 0x9100c3ffU : 0x910083ffU); // add sp,sp
    emit_u32(code, &offset, 0xd65f03c0U); // ret
    process_cross_stubs.offset = offset;
    note_process_cross_isa_call_stub(caller_arch, callee_arch, bridge_kind,
      signature_slot, needs_x86_call_wrapper);
    *stub_addr = start_addr;
    return 0;
  }

  if (process_cross_stubs.size - start < 160)
    return -1;
  size_t offset = start;
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, callee_frontend)); // frontend
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  if (is_stack9_x86_callee) {
    emit_u32(code, &offset, riscv_addi(2, 2, -48)); // addi sp,sp,-48
    emit_u32(code, &offset, riscv_sd(16, 2, 0)); // sd a6,0(sp)
    emit_u32(code, &offset, riscv_sd(17, 2, 8)); // sd a7,8(sp)
    emit_u32(code, &offset, riscv_ld(29, 2, 48)); // ld t4,48(sp)
    emit_u32(code, &offset, riscv_sd(29, 2, 16)); // sd t4,16(sp)
    emit_u32(code, &offset, riscv_sd(1, 2, 24)); // sd ra,24(sp)
    emit_u32(code, &offset, riscv_sd(10, 2, 32)); // sd a0,32(sp)
  }
  else {
    emit_u32(code, &offset, 0xfe010113U); // addi sp,sp,-32
    emit_u32(code, &offset, riscv_ld(29, 2, 32)); // ld t4,32(sp)
    emit_u32(code, &offset, riscv_sd(29, 2, 0)); // sd t4,0(sp)
    emit_u32(code, &offset, riscv_sd(1, 2, 8)); // sd ra,8(sp)
    emit_u32(code, &offset, riscv_sd(10, 2, 16)); // sd a0,16(sp)
  }
  size_t auipc_state_key_pc = 0;
  size_t ld_state_key_offset = 0;
  if (state_key != 0) {
    auipc_state_key_pc = offset;
    emit_u32(code, &offset, 0x00000517U); // auipc a0,0
    ld_state_key_offset = offset;
    emit_u32(code, &offset, 0);
    emit_u32(code, &offset, POLY_RISCV_CTRL_STATE_KEY_SET);
  }
  if (process_bridge_needs_foreign_signature_set(caller_arch, callee_arch,
        bridge_kind)) {
    const uint32_t signature_kind =
      process_bridge_signature_kind(bridge_kind);
    if (signature_kind == UINT32_MAX)
      return -1;
    emit_process_riscv_abi_signature_set(code, &offset, signature_slot,
      signature_kind);
  }
  emit_u32(code, &offset,
    riscv_ld(10, 2, is_stack9_x86_callee ? 32 : 16)); // ld a0,saved
  if (emit_compact_shuffle)
    emit_process_riscv_compact_pre_pcall(code, &offset, bridge_kind);
  if (emit_riscv_pcall_sig(code, &offset, signature_slot) < 0)
    return -1;
  const size_t return_pc = offset;
  if (emit_compact_shuffle)
    emit_process_riscv_compact_post_pcall(code, &offset, bridge_kind);
  emit_u32(code, &offset,
    riscv_ld(1, 2, is_stack9_x86_callee ? 24 : 8)); // ld ra,saved
  emit_u32(code, &offset,
    riscv_addi(2, 2, is_stack9_x86_callee ? 48 : 32)); // addi sp,sp
  emit_u32(code, &offset, 0x00008067U); // ret
  if (align_up_size(offset, 8, &offset) < 0 ||
      process_cross_stubs.size - offset < 24)
    return -1;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, pcall_target);
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + return_pc));
  size_t state_key_data_offset = 0;
  if (state_key != 0) {
    state_key_data_offset = offset;
    emit_u64(code, &offset, state_key);
  }
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int16_t) ((int64_t) target_data_offset - (int64_t) auipc_target_pc)));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int16_t) ((int64_t) return_data_offset - (int64_t) auipc_return_pc)));
  if (state_key != 0) {
    store_u32(code, ld_state_key_offset, riscv_ld(10, 10,
      (int16_t) ((int64_t) state_key_data_offset -
        (int64_t) auipc_state_key_pc)));
  }
  process_cross_stubs.offset = offset;
  note_process_cross_isa_call_stub(caller_arch, callee_arch, bridge_kind,
    signature_slot, needs_x86_call_wrapper);
  *stub_addr = start_addr;
  return 0;
}

static uint64_t process_helper_for_arch(const uint64_t helpers[POLY_ARCH_COUNT],
    int arch) {
  if (arch < 0 || arch >= POLY_ARCH_COUNT)
    return 0;
  return helpers[arch];
}

static int emit_process_tlsdesc_helper_for_arch(int arch,
    uint8_t *mapping, size_t *offset, uint64_t *helper_pc) {
  if (align_up_size(*offset, 4, offset) < 0 || *offset > SIZE_MAX - 16)
    return -1;
  if (arch != POLY_ARCH_AARCH64) {
    *helper_pc = 0;
    return 0;
  }
  *helper_pc = (uint64_t) (uintptr_t) (mapping + *offset);
  emit_u32(mapping, offset, 0xf9400400U); // ldr x0, [x0, #8]
  emit_u32(mapping, offset, 0xd65f03c0U); // ret
  return 0;
}

static int emit_process_tls_get_addr_helper_for_arch(int arch,
    uint8_t *mapping, size_t *offset, uint64_t *helper_pc) {
  if (align_up_size(*offset, 4, offset) < 0 || *offset > SIZE_MAX - 16)
    return -1;
  if (arch != POLY_ARCH_X86 && arch != POLY_ARCH_AARCH64 &&
      arch != POLY_ARCH_RISCV) {
    *helper_pc = 0;
    return 0;
  }
  *helper_pc = (uint64_t) (uintptr_t) (mapping + *offset);
  if (arch == POLY_ARCH_X86) {
    emit_bytes(mapping, offset, (const uint8_t []) {
      0x48, 0x8b, 0x47, 0x08, // mov rax,[rdi+8]
      0x4c, 0x01, 0xe8,       // add rax,r13
      0xc3                    // ret
    }, 8);
    return 0;
  }
  if (arch == POLY_ARCH_AARCH64) {
    emit_u32(mapping, offset, 0xd53bd041U); // mrs x1, tpidr_el0
    emit_u32(mapping, offset, 0xf9400400U); // ldr x0, [x0, #8]
    emit_u32(mapping, offset, 0x8b000020U); // add x0, x1, x0
    emit_u32(mapping, offset, 0xd65f03c0U); // ret
    return 0;
  }
  emit_u32(mapping, offset, riscv_ld(10, 10, 8)); // ld a0, 8(a0)
  emit_u32(mapping, offset, riscv_add(10, 10, 4)); // add a0, a0, tp
  emit_u32(mapping, offset, riscv_jalr(0, 1, 0)); // ret
  return 0;
}

static int emit_process_tls_helpers_for_arches(uint8_t *mapping,
    size_t *offset, uint64_t tlsdesc_helpers[POLY_ARCH_COUNT],
    uint64_t tls_get_addr_helpers[POLY_ARCH_COUNT]) {
  for (int arch = 0; arch < POLY_ARCH_COUNT; arch++) {
    if (emit_process_tlsdesc_helper_for_arch(arch, mapping, offset,
          &tlsdesc_helpers[arch]) < 0 ||
        emit_process_tls_get_addr_helper_for_arch(arch, mapping, offset,
          &tls_get_addr_helpers[arch]) < 0)
      return -1;
  }
  return 0;
}

static int run_irelative_resolver(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint8_t *scratch, uint64_t resolver_vaddr,
    uint64_t *resolved) {
  size_t resolver_offset = 0;
  if (elf_vaddr_to_image_offset(program, resolver_vaddr, 4,
        &resolver_offset) < 0)
    return -1;
  const uint64_t load_bias =
    (uint64_t) (uintptr_t) loaded_image - program->base_vaddr;
  const uint64_t resolver_pc = load_bias + resolver_vaddr;

  if (program->arch == POLY_ARCH_X86) {
    poly_mode_x86();
    uint64_t (*resolver_fn)(void) =
      (uint64_t (*)(void)) (uintptr_t) resolver_pc;
    *resolved = resolver_fn();
    return 0;
  }

  const size_t expected_prefix_size =
    program->arch == POLY_ARCH_AARCH64 ? 18 :
    program->arch == POLY_ARCH_RISCV ? 30 : 0;
  if (expected_prefix_size != 0 && prefix_size == expected_prefix_size) {
    if (emit_poly_trampoline(program, trampoline_code, prefix_size,
          return_pc, resolver_pc, 0) < 0)
      return -1;
    *resolved = run_poly_entry(trampoline_code, scratch);
    poly_mode_x86();
    return 0;
  }

  const size_t resolver_code_size = 4096;
  uint8_t *resolver_code = mmap(NULL, resolver_code_size,
    PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (resolver_code == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: IFUNC resolver trampoline mmap failed: %s\n",
      strerror(errno));
    return -1;
  }
  if (emit_poly_resolver_trampoline(program, resolver_code, resolver_code_size,
        return_pc, resolver_pc) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: IFUNC resolver trampoline failed: %s\n",
      program->path);
    munmap(resolver_code, resolver_code_size);
    return -1;
  }
  *resolved = run_poly_entry(resolver_code, scratch);
  poly_mode_x86();
  munmap(resolver_code, resolver_code_size);
  return 0;
}

static int apply_relative_relocations(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint64_t tlsdesc_helper_pc,
    uint64_t tls_get_addr_helper_pc, uint8_t *scratch) {
  if (!program->dynamic_size)
    return 0;

  const Elf64_Dyn *dyn = (const Elf64_Dyn *) (loaded_image + program->dynamic_offset);
  const size_t dyn_count = program->dynamic_size / sizeof(Elf64_Dyn);
  uint64_t rela_vaddr = 0, rela_size = 0, rela_ent = sizeof(Elf64_Rela);
  uint64_t rel_vaddr = 0, rel_size = 0, rel_ent = sizeof(Elf64_Rel);
  uint64_t relr_vaddr = 0, relr_size = 0, relr_ent = sizeof(uint64_t);
  uint64_t jmprel_vaddr = 0, pltrel_size = 0, pltrel_type = 0;
  uint64_t symtab_vaddr = 0, syment = sizeof(Elf64_Sym);
  uint64_t strtab_vaddr = 0, strsz = 0;
  uint64_t hash_vaddr = 0, gnu_hash_vaddr = 0;
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_RELA: rela_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_RELASZ: rela_size = dyn[n].d_un.d_val; break;
      case DT_RELAENT: rela_ent = dyn[n].d_un.d_val; break;
      case DT_REL: rel_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_RELSZ: rel_size = dyn[n].d_un.d_val; break;
      case DT_RELENT: rel_ent = dyn[n].d_un.d_val; break;
      case DT_RELR: relr_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_RELRSZ: relr_size = dyn[n].d_un.d_val; break;
      case DT_RELRENT: relr_ent = dyn[n].d_un.d_val; break;
      case DT_JMPREL: jmprel_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_PLTRELSZ: pltrel_size = dyn[n].d_un.d_val; break;
      case DT_PLTREL: pltrel_type = dyn[n].d_un.d_val; break;
      case DT_SYMTAB: symtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRTAB: strtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRSZ: strsz = dyn[n].d_un.d_val; break;
      case DT_SYMENT: syment = dyn[n].d_un.d_val; break;
      case DT_HASH: hash_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_GNU_HASH: gnu_hash_vaddr = dyn[n].d_un.d_ptr; break;
      default: break;
    }
  }

  const uint32_t relative_type = relative_reloc_type_for_arch(program->arch);
  const uint32_t irelative_type = irelative_reloc_type_for_arch(program->arch);
  const uint32_t tls_tprel_type = tls_tprel_reloc_type_for_arch(program->arch);
  const uint32_t none_type = none_reloc_type_for_arch(program->arch);
  const uint32_t copy_type = copy_reloc_type_for_arch(program->arch);
  const uint32_t tlsdesc_type = program->arch == POLY_ARCH_AARCH64 ?
    R_AARCH64_TLSDESC : UINT32_MAX;
  const uint32_t tls_dtpmod_type = program->arch == POLY_ARCH_AARCH64 ?
    R_AARCH64_TLS_DTPMOD64 :
    program->arch == POLY_ARCH_RISCV ? R_RISCV_TLS_DTPMOD64 :
    program->arch == POLY_ARCH_X86 ? R_X86_64_DTPMOD64 : UINT32_MAX;
  const uint32_t tls_dtprel_type = program->arch == POLY_ARCH_AARCH64 ?
    R_AARCH64_TLS_DTPREL64 :
    program->arch == POLY_ARCH_RISCV ? R_RISCV_TLS_DTPREL64 :
    program->arch == POLY_ARCH_X86 ? R_X86_64_DTPOFF64 : UINT32_MAX;
  const uint64_t load_bias = (uint64_t) (uintptr_t) loaded_image - program->base_vaddr;
  if (rela_vaddr && rela_size) {
    if (rela_ent < sizeof(Elf64_Rela) || rela_size % rela_ent)
      return -1;
    size_t rela_offset = 0;
    if (elf_vaddr_to_image_offset(program, rela_vaddr, rela_size, &rela_offset) < 0)
      return -1;
    for (size_t offset = 0; offset < rela_size; offset += (size_t) rela_ent) {
      const Elf64_Rela *rela = (const Elf64_Rela *) (loaded_image + rela_offset + offset);
      const uint64_t symbol_index = ELF64_R_SYM(rela->r_info);
      const uint32_t reloc_type = ELF64_R_TYPE(rela->r_info);
      if (reloc_type == none_type)
        continue;
      if (symbol_index != 0 && reloc_type == copy_type) {
        if (apply_process_copy_relocation(program, loaded_image, symtab_vaddr,
              strtab_vaddr, strsz, syment, hash_vaddr, gnu_hash_vaddr,
              symbol_index, rela->r_offset) < 0)
          return -1;
        continue;
      }
      size_t target = 0;
      if (elf_vaddr_to_image_offset(program, rela->r_offset, 8, &target) < 0)
        return -1;
      uint64_t reloc_value = 0;
      int reloc_value_is_absolute = 0;
      if (symbol_index == 0 && reloc_type == relative_type) {
        reloc_value = (uint64_t) rela->r_addend;
      }
      else if (symbol_index == 0 && reloc_type == irelative_type) {
        if (run_irelative_resolver(program, loaded_image, trampoline_code,
              prefix_size, return_pc, scratch, (uint64_t) rela->r_addend,
              &reloc_value) < 0)
          return -1;
        reloc_value_is_absolute = 1;
      }
      else if (symbol_index != 0 && reloc_type == tls_tprel_type) {
        if (resolve_process_tls_reloc_symbol(program, loaded_image,
              symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
              gnu_hash_vaddr, symbol_index, &reloc_value) < 0)
          return -1;
        reloc_value += (uint64_t) rela->r_addend;
        reloc_value_is_absolute = 1;
      }
      else if (symbol_index != 0 && reloc_type == tlsdesc_type) {
        if (tlsdesc_helper_pc == 0 ||
            elf_vaddr_to_image_offset(program, rela->r_offset, 16,
              &target) < 0 ||
            resolve_process_tls_reloc_symbol(program, loaded_image,
              symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
              gnu_hash_vaddr, symbol_index, &reloc_value) < 0)
          return -1;
        write_u64_le(loaded_image + target, tlsdesc_helper_pc);
        write_u64_le(loaded_image + target + 8,
          reloc_value + (uint64_t) rela->r_addend);
        continue;
      }
      else if (reloc_type == tls_dtpmod_type) {
        reloc_value = 1 + (uint64_t) rela->r_addend;
        reloc_value_is_absolute = 1;
      }
      else if (symbol_index != 0 && reloc_type == tls_dtprel_type) {
        if (resolve_process_tls_reloc_symbol(program, loaded_image,
              symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
              gnu_hash_vaddr, symbol_index, &reloc_value) < 0)
          return -1;
        reloc_value += (uint64_t) rela->r_addend;
        reloc_value_is_absolute = 1;
      }
      else if (symbol_index != 0 &&
          symbolic_64_reloc_type_for_arch(program->arch, reloc_type)) {
        uint8_t symbol_type = 0;
        int from_dependency = 0;
        if (resolve_same_image_reloc_symbol(program, loaded_image,
              symtab_vaddr, syment, hash_vaddr, gnu_hash_vaddr,
              symbol_index, &reloc_value, &symbol_type) < 0) {
          if (resolve_dependency_reloc_symbol(program, loaded_image,
                symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
                gnu_hash_vaddr, symbol_index, &reloc_value,
                &symbol_type, trampoline_code, prefix_size, return_pc,
                tls_get_addr_helper_pc, scratch) < 0)
            return -1;
          from_dependency = 1;
          reloc_value_is_absolute = 1;
        }
        if (!from_dependency && symbol_type == STT_GNU_IFUNC) {
          if (run_irelative_resolver(program, loaded_image, trampoline_code,
                prefix_size, return_pc, scratch, reloc_value,
                &reloc_value) < 0)
            return -1;
          reloc_value_is_absolute = 1;
        }
        reloc_value += (uint64_t) rela->r_addend;
      }
      else {
        fprintf(stderr,
          "POLYEXEC_FAIL: unsupported rela relocation arch=%d type=%u symbol=%llu offset=0x%llx\n",
          program->arch, (unsigned) reloc_type,
          (unsigned long long) symbol_index,
          (unsigned long long) rela->r_offset);
        return -1;
      }
      write_u64_le(loaded_image + target,
        reloc_value_is_absolute ? reloc_value : load_bias + reloc_value);
    }
  }

  if (rel_vaddr && rel_size) {
    if (rel_ent < sizeof(Elf64_Rel) || rel_size % rel_ent)
      return -1;
    size_t rel_offset = 0;
    if (elf_vaddr_to_image_offset(program, rel_vaddr, rel_size, &rel_offset) < 0)
      return -1;
    for (size_t offset = 0; offset < rel_size; offset += (size_t) rel_ent) {
      const Elf64_Rel *rel = (const Elf64_Rel *) (loaded_image + rel_offset + offset);
      const uint64_t symbol_index = ELF64_R_SYM(rel->r_info);
      const uint32_t reloc_type = ELF64_R_TYPE(rel->r_info);
      if (reloc_type == none_type)
        continue;
      if (symbol_index != 0 && reloc_type == copy_type) {
        if (apply_process_copy_relocation(program, loaded_image, symtab_vaddr,
              strtab_vaddr, strsz, syment, hash_vaddr, gnu_hash_vaddr,
              symbol_index, rel->r_offset) < 0)
          return -1;
        continue;
      }
      size_t target = 0;
      if (elf_vaddr_to_image_offset(program, rel->r_offset, 8, &target) < 0)
        return -1;
      uint64_t reloc_value = read_u64_le(loaded_image + target);
      int reloc_value_is_absolute = 0;
      if (symbol_index == 0 && reloc_type == relative_type) {
        // REL stores the addend in-place at the relocation target.
      }
      else if (symbol_index == 0 && reloc_type == irelative_type) {
        if (run_irelative_resolver(program, loaded_image, trampoline_code,
              prefix_size, return_pc, scratch, reloc_value, &reloc_value) < 0)
          return -1;
        reloc_value_is_absolute = 1;
      }
      else if (symbol_index != 0 && reloc_type == tls_tprel_type) {
        uint64_t symbol_value = 0;
        if (resolve_process_tls_reloc_symbol(program, loaded_image,
              symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
              gnu_hash_vaddr, symbol_index, &symbol_value) < 0)
          return -1;
        reloc_value += symbol_value;
        reloc_value_is_absolute = 1;
      }
      else if (symbol_index != 0 && reloc_type == tlsdesc_type) {
        uint64_t symbol_value = 0;
        if (tlsdesc_helper_pc == 0 ||
            elf_vaddr_to_image_offset(program, rel->r_offset, 16,
              &target) < 0 ||
            resolve_process_tls_reloc_symbol(program, loaded_image,
              symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
              gnu_hash_vaddr, symbol_index, &symbol_value) < 0)
          return -1;
        write_u64_le(loaded_image + target, tlsdesc_helper_pc);
        write_u64_le(loaded_image + target + 8,
          read_u64_le(loaded_image + target + 8) + symbol_value);
        continue;
      }
      else if (reloc_type == tls_dtpmod_type) {
        reloc_value += 1;
        reloc_value_is_absolute = 1;
      }
      else if (symbol_index != 0 && reloc_type == tls_dtprel_type) {
        uint64_t symbol_value = 0;
        if (resolve_process_tls_reloc_symbol(program, loaded_image,
              symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
              gnu_hash_vaddr, symbol_index, &symbol_value) < 0)
          return -1;
        reloc_value += symbol_value;
        reloc_value_is_absolute = 1;
      }
      else if (symbol_index != 0 &&
          symbolic_64_reloc_type_for_arch(program->arch, reloc_type)) {
        uint64_t symbol_value = 0;
        uint8_t symbol_type = 0;
        int from_dependency = 0;
        if (resolve_same_image_reloc_symbol(program, loaded_image,
              symtab_vaddr, syment, hash_vaddr, gnu_hash_vaddr,
              symbol_index, &symbol_value, &symbol_type) < 0) {
          if (resolve_dependency_reloc_symbol(program, loaded_image,
                symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
                gnu_hash_vaddr, symbol_index, &symbol_value,
                &symbol_type, trampoline_code, prefix_size, return_pc,
                tls_get_addr_helper_pc, scratch) < 0)
            return -1;
          from_dependency = 1;
          reloc_value_is_absolute = 1;
        }
        if (!from_dependency && symbol_type == STT_GNU_IFUNC) {
          if (run_irelative_resolver(program, loaded_image, trampoline_code,
                prefix_size, return_pc, scratch, symbol_value,
                &symbol_value) < 0)
            return -1;
          reloc_value += symbol_value;
          reloc_value_is_absolute = 1;
        }
        else {
          reloc_value += symbol_value;
        }
      }
      else {
        fprintf(stderr,
          "POLYEXEC_FAIL: unsupported rel relocation arch=%d type=%u symbol=%llu offset=0x%llx\n",
          program->arch, (unsigned) reloc_type,
          (unsigned long long) symbol_index,
          (unsigned long long) rel->r_offset);
        return -1;
      }
      write_u64_le(loaded_image + target,
        reloc_value_is_absolute ? reloc_value : load_bias + reloc_value);
    }
  }

  if (relr_vaddr && relr_size) {
    if (relr_ent != sizeof(uint64_t) || relr_size % sizeof(uint64_t))
      return -1;
    size_t relr_offset = 0;
    if (elf_vaddr_to_image_offset(program, relr_vaddr, relr_size, &relr_offset) < 0)
      return -1;
    uint64_t where = 0;
    for (size_t offset = 0; offset < relr_size; offset += sizeof(uint64_t)) {
      const uint64_t entry = read_u64_le(loaded_image + relr_offset + offset);
      if ((entry & 1) == 0) {
        where = entry;
        size_t target = 0;
        if (elf_vaddr_to_image_offset(program, where, 8, &target) < 0)
          return -1;
        write_u64_le(loaded_image + target, load_bias + read_u64_le(loaded_image + target));
        where += 8;
      }
      else {
        for (unsigned bit = 1; bit < 64; bit++) {
          if (entry & (UINT64_C(1) << bit)) {
            size_t target = 0;
            if (elf_vaddr_to_image_offset(program, where + (uint64_t) (bit - 1) * 8, 8, &target) < 0)
              return -1;
            write_u64_le(loaded_image + target, load_bias + read_u64_le(loaded_image + target));
          }
        }
        where += 63 * 8;
      }
    }
  }

  if (pltrel_size) {
    if (!jmprel_vaddr || (pltrel_type != DT_RELA && pltrel_type != DT_REL))
      return -1;
    if (pltrel_type == DT_RELA) {
      if (pltrel_size % sizeof(Elf64_Rela))
        return -1;
      size_t rela_offset = 0;
      if (elf_vaddr_to_image_offset(program, jmprel_vaddr, pltrel_size,
            &rela_offset) < 0)
        return -1;
      for (size_t offset = 0; offset < pltrel_size;
          offset += sizeof(Elf64_Rela)) {
        const Elf64_Rela *rela =
          (const Elf64_Rela *) (loaded_image + rela_offset + offset);
        const uint64_t symbol_index = ELF64_R_SYM(rela->r_info);
        const uint32_t reloc_type = ELF64_R_TYPE(rela->r_info);
        if (reloc_type == none_type)
          continue;
        if (symbol_index != 0 && reloc_type == tlsdesc_type) {
          uint64_t reloc_value = 0;
          size_t target = 0;
          if (tlsdesc_helper_pc == 0 ||
              elf_vaddr_to_image_offset(program, rela->r_offset, 16,
                &target) < 0 ||
              resolve_process_tls_reloc_symbol(program, loaded_image,
                symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
                gnu_hash_vaddr, symbol_index, &reloc_value) < 0)
            return -1;
          write_u64_le(loaded_image + target, tlsdesc_helper_pc);
          write_u64_le(loaded_image + target + 8,
            reloc_value + (uint64_t) rela->r_addend);
          continue;
        }
        if (symbol_index == 0 ||
            !symbolic_64_reloc_type_for_arch(program->arch, reloc_type)) {
          fprintf(stderr,
            "POLYEXEC_FAIL: unsupported plt rela relocation arch=%d type=%u symbol=%llu offset=0x%llx\n",
            program->arch, (unsigned) reloc_type,
            (unsigned long long) symbol_index,
            (unsigned long long) rela->r_offset);
          return -1;
        }
        uint64_t reloc_value = 0;
        uint8_t symbol_type = 0;
        int from_dependency = 0;
        if (resolve_same_image_reloc_symbol(program, loaded_image,
              symtab_vaddr, syment, hash_vaddr, gnu_hash_vaddr,
              symbol_index, &reloc_value, &symbol_type) < 0) {
          if (resolve_dependency_reloc_symbol(program, loaded_image,
                symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
                gnu_hash_vaddr, symbol_index, &reloc_value,
                &symbol_type, trampoline_code, prefix_size, return_pc,
                tls_get_addr_helper_pc, scratch) < 0)
            return -1;
          from_dependency = 1;
        }
        int reloc_value_is_absolute = from_dependency;
        if (!from_dependency && symbol_type == STT_GNU_IFUNC) {
          if (run_irelative_resolver(program, loaded_image, trampoline_code,
                prefix_size, return_pc, scratch, reloc_value,
                &reloc_value) < 0)
            return -1;
          reloc_value_is_absolute = 1;
        }
        size_t target = 0;
        if (elf_vaddr_to_image_offset(program, rela->r_offset, 8, &target) < 0)
          return -1;
        write_u64_le(loaded_image + target,
          (reloc_value_is_absolute ? reloc_value : load_bias + reloc_value) +
            (uint64_t) rela->r_addend);
      }
    }
    else {
      if (pltrel_size % sizeof(Elf64_Rel))
        return -1;
      size_t rel_offset = 0;
      if (elf_vaddr_to_image_offset(program, jmprel_vaddr, pltrel_size,
            &rel_offset) < 0)
        return -1;
      for (size_t offset = 0; offset < pltrel_size;
          offset += sizeof(Elf64_Rel)) {
        const Elf64_Rel *rel =
          (const Elf64_Rel *) (loaded_image + rel_offset + offset);
        const uint64_t symbol_index = ELF64_R_SYM(rel->r_info);
        const uint32_t reloc_type = ELF64_R_TYPE(rel->r_info);
        if (reloc_type == none_type)
          continue;
        if (symbol_index != 0 && reloc_type == tlsdesc_type) {
          uint64_t symbol_value = 0;
          size_t target = 0;
          if (tlsdesc_helper_pc == 0 ||
              elf_vaddr_to_image_offset(program, rel->r_offset, 16,
                &target) < 0 ||
              resolve_process_tls_reloc_symbol(program, loaded_image,
                symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
                gnu_hash_vaddr, symbol_index, &symbol_value) < 0)
            return -1;
          write_u64_le(loaded_image + target, tlsdesc_helper_pc);
          write_u64_le(loaded_image + target + 8,
            read_u64_le(loaded_image + target + 8) + symbol_value);
          continue;
        }
        if (symbol_index == 0 ||
            !symbolic_64_reloc_type_for_arch(program->arch, reloc_type)) {
          fprintf(stderr,
            "POLYEXEC_FAIL: unsupported plt rel relocation arch=%d type=%u symbol=%llu offset=0x%llx\n",
            program->arch, (unsigned) reloc_type,
            (unsigned long long) symbol_index,
            (unsigned long long) rel->r_offset);
          return -1;
        }
        size_t target = 0;
        if (elf_vaddr_to_image_offset(program, rel->r_offset, 8, &target) < 0)
          return -1;
        uint64_t symbol_value = 0;
        uint8_t symbol_type = 0;
        int from_dependency = 0;
        if (resolve_same_image_reloc_symbol(program, loaded_image,
              symtab_vaddr, syment, hash_vaddr, gnu_hash_vaddr,
              symbol_index, &symbol_value, &symbol_type) < 0) {
          if (resolve_dependency_reloc_symbol(program, loaded_image,
                symtab_vaddr, strtab_vaddr, strsz, syment, hash_vaddr,
                gnu_hash_vaddr, symbol_index, &symbol_value,
                &symbol_type, trampoline_code, prefix_size, return_pc,
                tls_get_addr_helper_pc, scratch) < 0)
            return -1;
          from_dependency = 1;
        }
        int symbol_value_is_absolute = from_dependency;
        if (!from_dependency && symbol_type == STT_GNU_IFUNC) {
          if (run_irelative_resolver(program, loaded_image, trampoline_code,
                prefix_size, return_pc, scratch, symbol_value,
                &symbol_value) < 0)
            return -1;
          symbol_value_is_absolute = 1;
        }
        write_u64_le(loaded_image + target,
          (symbol_value_is_absolute ? symbol_value : load_bias + symbol_value) +
            read_u64_le(loaded_image + target));
      }
    }
  }
  return 0;
}

static int call_process_initializer(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint64_t target_pc, uint8_t *scratch,
    const char *kind) {
  if (emit_poly_trampoline(program, trampoline_code, prefix_size, return_pc,
        target_pc, 0) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported %s lifecycle branch: %s\n",
      kind, program->path);
    return -1;
  }
  (void) run_poly_entry(trampoline_code, scratch);
  poly_mode_x86();
  (void) loaded_image;
  return 0;
}

static int run_process_initializer_array(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint8_t *scratch, uint64_t array_vaddr,
    uint64_t array_size, const char *kind) {
  if (array_size == 0)
    return 0;
  if (array_vaddr == 0 || array_size % sizeof(uint64_t) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: bad %s initializer array: %s\n", kind,
      program->path);
    return -1;
  }
  size_t array_offset = 0;
  if (elf_vaddr_to_image_offset(program, array_vaddr, array_size,
        &array_offset) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: %s initializer array escaped image: %s\n",
      kind, program->path);
    return -1;
  }
  const size_t init_count = (size_t) (array_size / sizeof(uint64_t));
  for (size_t n = 0; n < init_count; n++) {
    const uint64_t init_target =
      read_u64_le(loaded_image + array_offset + n * sizeof(uint64_t));
    if (init_target != 0 &&
        call_process_initializer(program, loaded_image, trampoline_code,
          prefix_size, return_pc, init_target, scratch, kind) < 0)
      return -1;
  }
  return 0;
}

static int run_process_preinitializers(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint8_t *scratch) {
  return run_process_initializer_array(program, loaded_image, trampoline_code,
    prefix_size, return_pc, scratch, program->preinit_array_vaddr,
    program->preinit_array_size, "root preinit");
}

static int run_process_initializers(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint8_t *scratch, const char *kind_prefix) {
  const uint64_t load_bias =
    (uint64_t) (uintptr_t) loaded_image - program->base_vaddr;
  if (program->init_vaddr != 0) {
    if (call_process_initializer(program, loaded_image, trampoline_code,
          prefix_size, return_pc, load_bias + program->init_vaddr, scratch,
          kind_prefix) < 0)
      return -1;
  }

  if (program->init_array_size == 0)
    return 0;
  return run_process_initializer_array(program, loaded_image, trampoline_code,
    prefix_size, return_pc, scratch, program->init_array_vaddr,
    program->init_array_size, kind_prefix);
}

static int run_process_finalizer_array(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint8_t *scratch, uint64_t array_vaddr,
    uint64_t array_size, const char *kind) {
  if (array_size == 0)
    return 0;
  if (array_vaddr == 0 || array_size % sizeof(uint64_t) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: bad %s finalizer array: %s\n", kind,
      program->path);
    return -1;
  }
  size_t array_offset = 0;
  if (elf_vaddr_to_image_offset(program, array_vaddr, array_size,
        &array_offset) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: %s finalizer array escaped image: %s\n",
      kind, program->path);
    return -1;
  }
  const size_t fini_count = (size_t) (array_size / sizeof(uint64_t));
  for (size_t n = fini_count; n > 0; n--) {
    const uint64_t fini_target =
      read_u64_le(loaded_image + array_offset + (n - 1) * sizeof(uint64_t));
    if (fini_target != 0 &&
        call_process_initializer(program, loaded_image, trampoline_code,
          prefix_size, return_pc, fini_target, scratch, kind) < 0)
      return -1;
  }
  return 0;
}

static int run_process_finalizers(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint8_t *scratch, const char *kind_prefix) {
  const uint64_t load_bias =
    (uint64_t) (uintptr_t) loaded_image - program->base_vaddr;
  if (run_process_finalizer_array(program, loaded_image, trampoline_code,
        prefix_size, return_pc, scratch, program->fini_array_vaddr,
        program->fini_array_size, kind_prefix) < 0)
    return -1;
  if (program->fini_vaddr != 0) {
    if (call_process_initializer(program, loaded_image, trampoline_code,
          prefix_size, return_pc, load_bias + program->fini_vaddr, scratch,
          kind_prefix) < 0)
      return -1;
  }
  return 0;
}

static int run_process_dependency_finalizers(struct poly_program *program,
    uint8_t *trampoline_code, size_t prefix_size, uint64_t return_pc,
    uint8_t *scratch) {
  for (size_t d = program->dep_count; d > 0; d--) {
    struct poly_process_dependency *dep = &program->deps[d - 1];
    if (dep->shared_from)
      continue;
    if (run_process_finalizers(dep->program, dep->loaded_image,
          trampoline_code, prefix_size, return_pc, scratch,
          "dependency") < 0)
      return -1;
    if (run_process_dependency_finalizers(dep->program, trampoline_code,
          prefix_size, return_pc, scratch) < 0)
      return -1;
  }
  return 0;
}

static int run_process_exit_finalizers(void) {
  struct poly_process_exit_finalizer_context *ctx =
    &poly_process_exit_finalizers;
  if (!ctx->active || !ctx->run_finalizers || ctx->completed || ctx->running)
    return 0;
  ctx->running = 1;
  int status = 0;
  if (run_process_finalizers(ctx->program, ctx->loaded_image,
        ctx->trampoline_code, ctx->prefix_size, ctx->return_pc, ctx->scratch,
        "root") < 0 ||
      run_process_dependency_finalizers(ctx->program, ctx->trampoline_code,
        ctx->prefix_size, ctx->return_pc, ctx->scratch) < 0)
    status = -1;
  ctx->completed = status == 0;
  ctx->running = 0;
  return status;
}

static int detect_arch(uint16_t machine, struct poly_program *program) {
  if (machine == EM_AARCH64) {
    program->arch = POLY_ARCH_AARCH64;
    program->arch_name = "aarch64";
    return 0;
  }
  if (machine == EM_RISCV) {
    program->arch = POLY_ARCH_RISCV;
    program->arch_name = "riscv";
    return 0;
  }
  if (machine == EM_X86_64) {
    program->arch = POLY_ARCH_X86;
    program->arch_name = "x86_64";
    return 0;
  }
  return -1;
}

static int resolve_dynamic_symbol(const struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count, const char *symbol_name,
    uint64_t *symbol_vaddr) {
  uint64_t symtab_vaddr = 0;
  uint64_t strtab_vaddr = 0;
  uint64_t strsz = 0;
  uint64_t syment = sizeof(Elf64_Sym);
  uint64_t hash_vaddr = 0;
  uint64_t gnu_hash_vaddr = 0;

  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_SYMTAB: symtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRTAB: strtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRSZ: strsz = dyn[n].d_un.d_val; break;
      case DT_SYMENT: syment = dyn[n].d_un.d_val; break;
      case DT_HASH: hash_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_GNU_HASH: gnu_hash_vaddr = dyn[n].d_un.d_ptr; break;
      default: break;
    }
  }

  if (!symtab_vaddr || !strtab_vaddr || !strsz ||
      syment < sizeof(Elf64_Sym))
    return -1;

  size_t symbol_count = 0;
  if (hash_vaddr) {
    size_t hash_offset = 0;
    if (elf_vaddr_to_image_offset(program, hash_vaddr, 8, &hash_offset) < 0)
      return -1;
    uint32_t nchain = 0;
    memcpy(&nchain, program->code_bytes + hash_offset + 4, sizeof(nchain));
    symbol_count = nchain;
  }
  else if (gnu_hash_vaddr) {
    size_t symtab_offset = 0;
    if (elf_vaddr_to_image_offset(program, symtab_vaddr, syment,
          &symtab_offset) < 0)
      return -1;

    size_t hash_offset = 0;
    if (elf_vaddr_to_image_offset(program, gnu_hash_vaddr, 16,
          &hash_offset) < 0)
      return -1;
    const uint32_t *hash = (const uint32_t *) (program->code_bytes + hash_offset);
    const uint32_t nbuckets = hash[0];
    const uint32_t symoffset = hash[1];
    const uint32_t bloom_size = hash[2];
    if (nbuckets == 0 || bloom_size == 0)
      return -1;

    const uint64_t buckets_offset = (uint64_t) hash_offset + 16 +
      (uint64_t) bloom_size * sizeof(uint64_t);
    const uint64_t buckets_size = (uint64_t) nbuckets * sizeof(uint32_t);
    if (buckets_offset > program->code_size ||
        buckets_size > program->code_size - buckets_offset)
      return -1;

    const uint64_t chains_offset = buckets_offset + buckets_size;
    if (chains_offset > program->code_size ||
        symtab_offset > program->code_size ||
        (program->code_size - symtab_offset) / syment < symoffset)
      return -1;

    const size_t max_symbols =
      (program->code_size - symtab_offset) / syment;
    const uint32_t *buckets =
      (const uint32_t *) (program->code_bytes + buckets_offset);
    symbol_count = symoffset;

    for (uint32_t n = 0; n < nbuckets; n++) {
      uint32_t index = buckets[n];
      if (index == 0)
        continue;
      if (index < symoffset || index >= max_symbols)
        return -1;

      while (1) {
        const uint64_t chain_offset = chains_offset +
          (uint64_t) (index - symoffset) * sizeof(uint32_t);
        if (chain_offset > program->code_size ||
            sizeof(uint32_t) > program->code_size - chain_offset)
          return -1;

        const uint32_t chain =
          *(const uint32_t *) (program->code_bytes + chain_offset);
        if ((size_t) index + 1 > symbol_count)
          symbol_count = (size_t) index + 1;
        if (chain & 1)
          break;
        index++;
        if (index >= max_symbols)
          return -1;
      }
    }
  }
  else {
    return -1;
  }

  if (symbol_count == 0 || symbol_count > 4096)
    return -1;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz, &strtab_offset) < 0)
    return -1;

  size_t symtab_offset = 0;
  const uint64_t symtab_size = (uint64_t) symbol_count * syment;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, symtab_size, &symtab_offset) < 0)
    return -1;

  for (uint32_t n = 0; n < symbol_count; n++) {
    const Elf64_Sym *sym = (const Elf64_Sym *) (program->code_bytes +
      symtab_offset + (uint64_t) n * syment);
    if (sym->st_name >= strsz)
      continue;
    const char *name = (const char *) (program->code_bytes + strtab_offset + sym->st_name);
    if (strcmp(name, symbol_name) == 0) {
      *symbol_vaddr = sym->st_value;
      return 0;
    }
  }
  return -1;
}

static int resolve_section_symbol(const unsigned char *data, size_t size,
    const Elf64_Ehdr *ehdr, const char *symbol_name, uint64_t *symbol_vaddr) {
  if (!ehdr->e_shoff || !ehdr->e_shnum ||
      ehdr->e_shentsize < sizeof(Elf64_Shdr) ||
      ehdr->e_shoff > size ||
      (uint64_t) ehdr->e_shnum * ehdr->e_shentsize > size - ehdr->e_shoff)
    return -1;

  const Elf64_Shdr *sections = (const Elf64_Shdr *) (data + ehdr->e_shoff);
  for (uint16_t n = 0; n < ehdr->e_shnum; n++) {
    const Elf64_Shdr *sym_shdr = &sections[n];
    if (sym_shdr->sh_type != SHT_DYNSYM && sym_shdr->sh_type != SHT_SYMTAB)
      continue;
    if (sym_shdr->sh_entsize < sizeof(Elf64_Sym) ||
        sym_shdr->sh_offset > size ||
        sym_shdr->sh_size > size - sym_shdr->sh_offset ||
        sym_shdr->sh_link >= ehdr->e_shnum)
      continue;

    const Elf64_Shdr *str_shdr = &sections[sym_shdr->sh_link];
    if (str_shdr->sh_offset > size || str_shdr->sh_size > size - str_shdr->sh_offset)
      continue;

    const size_t symbol_count = (size_t) (sym_shdr->sh_size / sym_shdr->sh_entsize);
    const char *strings = (const char *) (data + str_shdr->sh_offset);
    for (size_t index = 0; index < symbol_count; index++) {
      const Elf64_Sym *sym = (const Elf64_Sym *)
        (data + sym_shdr->sh_offset + (uint64_t) index * sym_shdr->sh_entsize);
      if (sym->st_name >= str_shdr->sh_size)
        continue;
      if (strcmp(strings + sym->st_name, symbol_name) == 0) {
        *symbol_vaddr = sym->st_value;
        return 0;
      }
    }
  }
  return -1;
}

static int load_elf_program(const char *path, const char *symbol_name,
    struct poly_program *program) {
  memset(program, 0, sizeof(*program));
  program->path = path;

  unsigned char *data = NULL;
  size_t size = 0;
  if (read_file(path, &data, &size) < 0)
    return -1;

  if (size < sizeof(Elf64_Ehdr)) {
    fprintf(stderr, "POLYEXEC_FAIL: ELF too small: %s\n", path);
    free(data);
    return -1;
  }

  const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *) data;
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
      ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
      (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) ||
      detect_arch(ehdr->e_machine, program) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported ELF header: %s\n", path);
    free(data);
    return -1;
  }
  program->is_et_exec = ehdr->e_type == ET_EXEC;

  if (ehdr->e_phentsize < sizeof(Elf64_Phdr) ||
      ehdr->e_phoff > size ||
      (uint64_t) ehdr->e_phnum * ehdr->e_phentsize > size - ehdr->e_phoff) {
    fprintf(stderr, "POLYEXEC_FAIL: bad ELF program header table: %s\n", path);
    free(data);
    return -1;
  }
  const uint64_t phdr_table_size =
    (uint64_t) ehdr->e_phnum * ehdr->e_phentsize;

  uint64_t base_vaddr = UINT64_MAX;
  uint64_t limit_vaddr = 0;
  uint64_t dynamic_vaddr = 0;
  uint64_t dynamic_size = 0;
  uint64_t phdr_vaddr = 0;
  int found_load = 0;
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type == PT_PHDR) {
      phdr_vaddr = phdr->p_vaddr;
      continue;
    }
    if (phdr->p_type == PT_DYNAMIC) {
      dynamic_vaddr = phdr->p_vaddr;
      dynamic_size = phdr->p_filesz;
      continue;
    }
    if (phdr->p_type == PT_INTERP) {
      if (phdr->p_filesz == 0 ||
          phdr->p_filesz >= sizeof(program->interp_path) ||
          phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
          memchr(data + phdr->p_offset, '\0', (size_t) phdr->p_filesz) == NULL) {
        fprintf(stderr, "POLYEXEC_FAIL: bad ELF interpreter: %s\n", path);
        free(data);
        return -1;
      }
      memcpy(program->interp_path, data + phdr->p_offset,
        (size_t) phdr->p_filesz);
      program->interp_path[sizeof(program->interp_path) - 1] = '\0';
      program->has_interp = 1;
      continue;
    }
    if (phdr->p_type == PT_GNU_RELRO) {
      program->relro_vaddr = phdr->p_vaddr;
      program->relro_size = phdr->p_memsz;
      continue;
    }
    if (phdr->p_type == PT_NOTE) {
      if (parse_process_bridge_specs_notes(program, data, size,
            (size_t) phdr->p_offset, (size_t) phdr->p_filesz) < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: bad Poly ABI note: %s\n", path);
        free(data);
        return -1;
      }
      continue;
    }
    if (phdr->p_type == PT_TLS) {
      if (phdr->p_filesz > phdr->p_memsz ||
          phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
          phdr->p_memsz > MAX_PROCESS_TLS_BYTES ||
          phdr->p_align > MAX_PROCESS_TLS_BYTES) {
        fprintf(stderr, "POLYEXEC_FAIL: bad ELF TLS segment: %s\n", path);
        free(data);
        return -1;
      }
      program->tls_vaddr = phdr->p_vaddr;
      program->tls_filesz = phdr->p_filesz;
      program->tls_memsz = phdr->p_memsz;
      program->tls_align = phdr->p_align;
      continue;
    }
    if (phdr->p_type != PT_LOAD)
      continue;

    if (phdr->p_filesz > phdr->p_memsz ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
        phdr->p_vaddr > UINT64_MAX - phdr->p_memsz ||
        phdr->p_vaddr > UINT64_MAX - phdr->p_filesz) {
      fprintf(stderr, "POLYEXEC_FAIL: bad ELF load segment: %s\n", path);
      free(data);
      return -1;
    }

    if (!phdr_vaddr && ehdr->e_phoff >= phdr->p_offset) {
      const uint64_t phdr_offset_in_segment = ehdr->e_phoff - phdr->p_offset;
      if (phdr_offset_in_segment <= phdr->p_filesz &&
          phdr_table_size <= phdr->p_filesz - phdr_offset_in_segment)
        phdr_vaddr = phdr->p_vaddr + phdr_offset_in_segment;
    }
    const uint64_t segment_base = align_down_u64(phdr->p_vaddr, 0x1000);
    const uint64_t segment_limit = phdr->p_vaddr + phdr->p_memsz;
    if (record_load_segment(program, phdr) < 0) {
      free(data);
      return -1;
    }
    if (segment_base < base_vaddr)
      base_vaddr = segment_base;
    if (segment_limit > limit_vaddr)
      limit_vaddr = segment_limit;
    found_load = 1;
  }

  if (!found_load || limit_vaddr <= base_vaddr) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported ELF load image: %s\n", path);
    free(data);
    return -1;
  }
  if (limit_vaddr - base_vaddr > MAX_PROGRAM_BYTES) {
    fprintf(stderr,
      "POLYEXEC_FAIL: ELF load image is too large: %s span=0x%llx max=0x%llx\n",
      path, (unsigned long long) (limit_vaddr - base_vaddr),
      (unsigned long long) MAX_PROGRAM_BYTES);
    free(data);
    return -1;
  }

  uint64_t image_size = limit_vaddr - base_vaddr;
  const uint64_t instruction_align = program->arch == POLY_ARCH_AARCH64 ? 4 : 2;
  if ((image_size % instruction_align) != 0)
    image_size += instruction_align - (image_size % instruction_align);
  if (image_size == 0 || image_size > MAX_PROGRAM_BYTES) {
    fprintf(stderr,
      "POLYEXEC_FAIL: ELF loaded image is too large: %s span=0x%llx max=0x%llx\n",
      path, (unsigned long long) image_size,
      (unsigned long long) MAX_PROGRAM_BYTES);
    free(data);
    return -1;
  }

  program->base_vaddr = base_vaddr;
  program->phent = ehdr->e_phentsize;
  program->phnum = ehdr->e_phnum;
  program->phdr_vaddr = phdr_vaddr;
  program->code_size = (size_t) image_size;
  program->code_bytes = calloc(1, program->code_size);
  if (!program->code_bytes) {
    fprintf(stderr, "POLYEXEC_FAIL: out of memory loading %s\n", path);
    free(data);
    return -1;
  }

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD || phdr->p_filesz == 0)
      continue;

    const uint64_t dest_offset = phdr->p_vaddr - base_vaddr;
    if (dest_offset > program->code_size ||
        phdr->p_filesz > program->code_size - dest_offset) {
      fprintf(stderr, "POLYEXEC_FAIL: bad ELF load copy range: %s\n", path);
      free(program->code_bytes);
      program->code_bytes = NULL;
      program->code_size = 0;
      free(data);
      return -1;
    }
    memcpy(program->code_bytes + dest_offset, data + phdr->p_offset, (size_t) phdr->p_filesz);
  }

  uint64_t entry_vaddr = ehdr->e_entry;
  if (symbol_name && symbol_name[0] != '\0') {
    int resolved = -1;
    if (dynamic_vaddr && dynamic_size && dynamic_size % sizeof(Elf64_Dyn) == 0) {
      size_t dynamic_offset = 0;
      if (elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size, &dynamic_offset) == 0) {
        resolved = resolve_dynamic_symbol(program,
          (const Elf64_Dyn *) (program->code_bytes + dynamic_offset),
          (size_t) (dynamic_size / sizeof(Elf64_Dyn)), symbol_name, &entry_vaddr);
      }
    }
    if (resolved < 0)
      resolved = resolve_section_symbol(data, size, ehdr, symbol_name, &entry_vaddr);
    if (resolved < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: symbol not found: %s#%s\n", path, symbol_name);
      free(program->code_bytes);
      program->code_bytes = NULL;
      program->code_size = 0;
      free(data);
      return -1;
    }
  }

  int entry_in_exec = 0;
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type == PT_LOAD && (phdr->p_flags & PF_X) &&
        entry_vaddr >= phdr->p_vaddr &&
        entry_vaddr < phdr->p_vaddr + phdr->p_filesz) {
      entry_in_exec = 1;
      break;
    }
  }
  if (ehdr->e_type == ET_DYN &&
      (!symbol_name || symbol_name[0] == '\0') && entry_vaddr == 0) {
    entry_in_exec = 1;
    entry_vaddr = base_vaddr;
  }
  if (!entry_in_exec || entry_vaddr < base_vaddr || entry_vaddr >= limit_vaddr) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported ELF entry image: %s\n", path);
    free(program->code_bytes);
    program->code_bytes = NULL;
    program->code_size = 0;
    free(data);
    return -1;
  }
  program->entry_offset = (size_t) (entry_vaddr - base_vaddr);

  if (dynamic_vaddr && dynamic_size) {
    if (dynamic_size % sizeof(Elf64_Dyn) != 0 ||
        elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size,
          &program->dynamic_offset) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: bad dynamic segment: %s\n", path);
      free(program->code_bytes);
      program->code_bytes = NULL;
      program->code_size = 0;
      free(data);
      return -1;
    }
    program->dynamic_size = (size_t) dynamic_size;
    const Elf64_Dyn *dyn =
      (const Elf64_Dyn *) (program->code_bytes + program->dynamic_offset);
    const size_t dyn_count = program->dynamic_size / sizeof(Elf64_Dyn);
    uint64_t strtab_vaddr = 0, strsz = 0, soname_offset = UINT64_MAX;
    for (size_t n = 0; n < dyn_count; n++) {
      switch (dyn[n].d_tag) {
        case DT_STRTAB:
          strtab_vaddr = dyn[n].d_un.d_ptr;
          break;
        case DT_STRSZ:
          strsz = dyn[n].d_un.d_val;
          break;
        case DT_SONAME:
          soname_offset = dyn[n].d_un.d_val;
          break;
        case DT_INIT:
          program->init_vaddr = dyn[n].d_un.d_ptr;
          break;
        case DT_PREINIT_ARRAY:
          program->preinit_array_vaddr = dyn[n].d_un.d_ptr;
          break;
        case DT_PREINIT_ARRAYSZ:
          program->preinit_array_size = dyn[n].d_un.d_val;
          break;
        case DT_INIT_ARRAY:
          program->init_array_vaddr = dyn[n].d_un.d_ptr;
          break;
        case DT_INIT_ARRAYSZ:
          program->init_array_size = dyn[n].d_un.d_val;
          break;
        case DT_FINI:
          program->fini_vaddr = dyn[n].d_un.d_ptr;
          break;
        case DT_FINI_ARRAY:
          program->fini_array_vaddr = dyn[n].d_un.d_ptr;
          break;
        case DT_FINI_ARRAYSZ:
          program->fini_array_size = dyn[n].d_un.d_val;
          break;
        default:
          break;
      }
    }
    if (soname_offset != UINT64_MAX) {
      size_t strtab_offset = 0;
      if (!strtab_vaddr || !strsz || soname_offset >= strsz ||
          elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
            &strtab_offset) < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: bad SONAME dynamic table: %s\n",
          path);
        free(program->code_bytes);
        program->code_bytes = NULL;
        program->code_size = 0;
        free(data);
        return -1;
      }
      const char *strings =
        (const char *) (program->code_bytes + strtab_offset);
      const void *end = memchr(strings + soname_offset, '\0',
        (size_t) (strsz - soname_offset));
      if (!end) {
        fprintf(stderr, "POLYEXEC_FAIL: bad SONAME string: %s\n", path);
        free(program->code_bytes);
        program->code_bytes = NULL;
        program->code_size = 0;
        free(data);
        return -1;
      }
      const size_t soname_len =
        (size_t) ((const char *) end - (strings + soname_offset));
      if (soname_len >= sizeof(program->soname)) {
        fprintf(stderr, "POLYEXEC_FAIL: SONAME too long: %s\n", path);
        free(program->code_bytes);
        program->code_bytes = NULL;
        program->code_size = 0;
        free(data);
        return -1;
      }
      memcpy(program->soname, strings + soname_offset, soname_len);
      program->soname[soname_len] = '\0';
    }
    if (program->preinit_array_size != 0 &&
        (program->preinit_array_vaddr == 0 ||
         program->preinit_array_size % sizeof(uint64_t) != 0)) {
      fprintf(stderr, "POLYEXEC_FAIL: bad PREINIT_ARRAY dynamic table: %s\n",
        path);
      free(program->code_bytes);
      program->code_bytes = NULL;
      program->code_size = 0;
      free(data);
      return -1;
    }
    if (program->init_array_size != 0 &&
        (program->init_array_vaddr == 0 ||
         program->init_array_size % sizeof(uint64_t) != 0)) {
      fprintf(stderr, "POLYEXEC_FAIL: bad INIT_ARRAY dynamic table: %s\n",
        path);
      free(program->code_bytes);
      program->code_bytes = NULL;
      program->code_size = 0;
      free(data);
      return -1;
    }
  }

  free(data);
  return 0;
}

static void free_program(struct poly_program *program);

static int build_needed_path(const char *owner_path, const char *needed,
    char *out, size_t out_size) {
  if (!needed || needed[0] == '\0')
    return -1;
  if (needed[0] == '/') {
    if (strlen(needed) >= out_size)
      return -1;
    strcpy(out, needed);
    return 0;
  }

  const char *slash = strrchr(owner_path, '/');
  if (!slash) {
    if (strlen(needed) >= out_size)
      return -1;
    strcpy(out, needed);
    return 0;
  }

  const size_t dir_len = (size_t) (slash + 1 - owner_path);
  const size_t needed_len = strlen(needed);
  if (dir_len + needed_len >= out_size)
    return -1;
  memcpy(out, owner_path, dir_len);
  memcpy(out + dir_len, needed, needed_len + 1);
  return 0;
}

static int append_path_bytes(char *out, size_t *out_len, size_t out_size,
    const char *bytes, size_t bytes_len) {
  if (*out_len + bytes_len + 1 > out_size)
    return -1;
  memcpy(out + *out_len, bytes, bytes_len);
  *out_len += bytes_len;
  out[*out_len] = '\0';
  return 0;
}

static int append_origin_dir(const char *owner_path, char *out,
    size_t *out_len, size_t out_size) {
  const char *slash = strrchr(owner_path, '/');
  const size_t dir_len = slash ? (size_t) (slash - owner_path) : 0;
  return append_path_bytes(out, out_len, out_size, owner_path, dir_len);
}

static int expand_runpath_entry(const char *owner_path,
    const char *platform_name, const char *entry, size_t entry_len,
    char *out, size_t out_size) {
  size_t out_len = 0;
  if (out_size == 0)
    return -1;
  out[0] = '\0';
  for (size_t n = 0; n < entry_len;) {
    if (entry_len - n >= 9 && memcmp(entry + n, "${ORIGIN}", 9) == 0) {
      if (append_origin_dir(owner_path, out, &out_len, out_size) < 0)
        return -1;
      n += 9;
      continue;
    }
    if (entry_len - n >= 7 && memcmp(entry + n, "$ORIGIN", 7) == 0) {
      if (append_origin_dir(owner_path, out, &out_len, out_size) < 0)
        return -1;
      n += 7;
      continue;
    }
    if (entry_len - n >= 6 && memcmp(entry + n, "${LIB}", 6) == 0) {
      if (append_path_bytes(out, &out_len, out_size, "lib", 3) < 0)
        return -1;
      n += 6;
      continue;
    }
    if (entry_len - n >= 4 && memcmp(entry + n, "$LIB", 4) == 0) {
      if (append_path_bytes(out, &out_len, out_size, "lib", 3) < 0)
        return -1;
      n += 4;
      continue;
    }
    if (platform_name &&
        entry_len - n >= 11 &&
        memcmp(entry + n, "${PLATFORM}", 11) == 0) {
      if (append_path_bytes(out, &out_len, out_size, platform_name,
            strlen(platform_name)) < 0)
        return -1;
      n += 11;
      continue;
    }
    if (platform_name &&
        entry_len - n >= 9 && memcmp(entry + n, "$PLATFORM", 9) == 0) {
      if (append_path_bytes(out, &out_len, out_size, platform_name,
            strlen(platform_name)) < 0)
        return -1;
      n += 9;
      continue;
    }
    if (append_path_bytes(out, &out_len, out_size, entry + n, 1) < 0)
      return -1;
    n++;
  }
  return out_len == 0 ? -1 : 0;
}

static int build_runpath_entry_needed_path(const char *entry, size_t entry_len,
    const char *needed, char *out, size_t out_size) {
  const size_t needed_len = strlen(needed);
  const int entry_has_slash = entry_len != 0 && entry[entry_len - 1] == '/';
  const size_t sep_len = entry_has_slash ? 0 : 1;
  if (entry_len + sep_len + needed_len + 1 > out_size)
    return -1;
  memcpy(out, entry, entry_len);
  if (sep_len != 0)
    out[entry_len] = '/';
  memcpy(out + entry_len + sep_len, needed, needed_len + 1);
  return 0;
}

static int build_runpath_needed_path(const char *owner_path,
    const char *platform_name, const char *runpath, size_t runpath_len,
    const char *needed, char *out, size_t out_size) {
  if (!runpath || runpath_len == 0 || needed[0] == '/')
    return -1;

  size_t start = 0;
  while (start < runpath_len) {
    size_t end = start;
    while (end < runpath_len && runpath[end] != ':')
      end++;
    const char *entry = runpath + start;
    const size_t entry_len = end - start;
    start = end + 1;
    if (entry_len == 0)
      continue;

    char expanded[MAX_DEP_PATH];
    if (expand_runpath_entry(owner_path, platform_name, entry, entry_len,
          expanded, sizeof(expanded)) == 0 &&
        build_runpath_entry_needed_path(expanded, strlen(expanded), needed,
          out, out_size) == 0 &&
        access(out, R_OK) == 0)
      return 0;
  }
  return -1;
}

static const char *process_library_path(void) {
  const char *library_path = getenv("POLY_LD_LIBRARY_PATH");
  if (library_path && library_path[0] != '\0')
    return library_path;
  return getenv("LD_LIBRARY_PATH");
}

static int find_process_needed_path(const char *owner_path, const char *needed,
    const char *platform_name, const char *runpath, size_t runpath_len,
    char *out, size_t out_size) {
  char expanded_needed[MAX_DEP_PATH];
  if (expand_runpath_entry(owner_path, platform_name, needed, strlen(needed),
        expanded_needed, sizeof(expanded_needed)) < 0)
    return -1;
  needed = expanded_needed;

  const char *library_path = process_library_path();
  const size_t library_path_len = library_path ? strlen(library_path) : 0;
  if (needed[0] != '/' && library_path_len != 0 &&
      build_runpath_needed_path(owner_path, platform_name, library_path,
        library_path_len, needed, out, out_size) == 0)
    return 0;
  if (needed[0] != '/' &&
      build_runpath_needed_path(owner_path, platform_name, runpath,
        runpath_len, needed, out, out_size) == 0)
    return 0;
  if (build_needed_path(owner_path, needed, out, out_size) < 0)
    return -1;
  if (needed[0] != '/' && access(out, R_OK) != 0)
    return -1;
  return 0;
}

static const char *process_platform_name_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return "aarch64";
  if (arch == POLY_ARCH_RISCV)
    return "riscv";
  if (arch == POLY_ARCH_X86)
    return "x86_64";
  return NULL;
}

static int preload_separator(char c) {
  return c == ':' || c == ' ' || c == '\t' || c == '\n';
}

static int process_dependency_path_already_loaded(
    const struct poly_program *program, const char *path) {
  for (size_t d = 0; d < program->dep_count; d++) {
    if (strcmp(program->deps[d].path, path) == 0)
      return 1;
  }
  return 0;
}

static struct poly_process_dependency *canonical_process_dependency(
    struct poly_process_dependency *dep) {
  while (dep && dep->shared_from)
    dep = dep->shared_from;
  return dep;
}

static struct poly_process_dependency *find_process_dependency_soname(
    struct poly_program *program, int arch, const char *soname) {
  if (!program || !soname || soname[0] == '\0')
    return NULL;

  for (size_t d = 0; d < program->dep_count; d++) {
    struct poly_process_dependency *dep = &program->deps[d];
    struct poly_process_dependency *canonical =
      canonical_process_dependency(dep);
    if (canonical && canonical->program && canonical->program->arch == arch &&
        canonical->program->soname[0] != '\0' &&
        strcmp(canonical->program->soname, soname) == 0)
      return canonical;
    if (canonical && canonical->program) {
      struct poly_process_dependency *nested =
        find_process_dependency_soname(canonical->program, arch, soname);
      if (nested)
        return nested;
    }
  }

  return NULL;
}

static int build_process_preload_path(const struct poly_program *program,
    const char *token, size_t token_len, char *out, size_t out_size) {
  if (token_len == 0 || token_len >= MAX_DEP_PATH)
    return -1;

  char raw[MAX_DEP_PATH];
  memcpy(raw, token, token_len);
  raw[token_len] = '\0';

  char expanded[MAX_DEP_PATH];
  if (expand_runpath_entry(program->path,
        process_platform_name_for_arch(program->arch), raw, token_len,
        expanded, sizeof(expanded)) < 0)
    return -1;

  if (expanded[0] == '/') {
    if (strlen(expanded) >= out_size || access(expanded, R_OK) != 0)
      return -1;
    strcpy(out, expanded);
    return 0;
  }

  const char *library_path = process_library_path();
  if (library_path && library_path[0] != '\0' &&
      build_runpath_needed_path(program->path,
        process_platform_name_for_arch(program->arch), library_path,
        strlen(library_path), expanded, out, out_size) == 0)
    return 0;

  if (build_needed_path(program->path, expanded, out, out_size) == 0 &&
      access(out, R_OK) == 0)
    return 0;

  return -1;
}

static int load_process_dependencies_at_depth(struct poly_program *program,
    struct poly_program *root_program, size_t depth) {
  if (!program->dynamic_size)
    return 0;
  if (depth >= MAX_PROCESS_DEP_DEPTH) {
    fprintf(stderr, "POLYEXEC_FAIL: DT_NEEDED dependency depth exceeded: %s\n",
      program->path);
    return -1;
  }

  const Elf64_Dyn *dyn =
    (const Elf64_Dyn *) (program->code_bytes + program->dynamic_offset);
  const size_t dyn_count = program->dynamic_size / sizeof(Elf64_Dyn);
  uint64_t strtab_vaddr = 0, strsz = 0;
  uint64_t runpath_offset = UINT64_MAX, rpath_offset = UINT64_MAX;
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_STRTAB: strtab_vaddr = dyn[n].d_un.d_ptr; break;
      case DT_STRSZ: strsz = dyn[n].d_un.d_val; break;
      case DT_RUNPATH: runpath_offset = dyn[n].d_un.d_val; break;
      case DT_RPATH: rpath_offset = dyn[n].d_un.d_val; break;
      default: break;
    }
  }
  if (!strtab_vaddr || !strsz)
    return 0;

  size_t strtab_offset = 0;
  if (elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;
  const char *strings = (const char *) (program->code_bytes + strtab_offset);
  const char *runpath = NULL;
  size_t runpath_len = 0;
  const uint64_t search_path_offset =
    runpath_offset != UINT64_MAX ? runpath_offset : rpath_offset;
  const char *search_path_tag =
    runpath_offset != UINT64_MAX ? "DT_RUNPATH" : "DT_RPATH";
  if (search_path_offset != UINT64_MAX) {
    if (search_path_offset >= strsz) {
      fprintf(stderr, "POLYEXEC_FAIL: bad %s string: %s\n", search_path_tag,
        program->path);
      return -1;
    }
    const void *end = memchr(strings + search_path_offset, '\0',
      (size_t) (strsz - search_path_offset));
    if (!end) {
      fprintf(stderr, "POLYEXEC_FAIL: bad %s string: %s\n", search_path_tag,
        program->path);
      return -1;
    }
    runpath = strings + search_path_offset;
    runpath_len = (size_t) ((const char *) end - runpath);
  }

  for (size_t n = 0; n < dyn_count; n++) {
    if (dyn[n].d_tag != DT_NEEDED)
      continue;
    const uint64_t needed_offset = dyn[n].d_un.d_val;
    if (needed_offset >= strsz ||
        memchr(strings + needed_offset, '\0',
          (size_t) (strsz - needed_offset)) == NULL) {
      fprintf(stderr, "POLYEXEC_FAIL: bad DT_NEEDED string: %s\n",
        program->path);
      return -1;
    }
    if (program->dep_count >= MAX_PROCESS_DEPS) {
      fprintf(stderr, "POLYEXEC_FAIL: too many DT_NEEDED dependencies: %s\n",
        program->path);
      return -1;
    }

    const char *needed = strings + needed_offset;
    struct poly_process_dependency *dep = &program->deps[program->dep_count];
    if (strchr(needed, '/') == NULL) {
      struct poly_process_dependency *shared =
        find_process_dependency_soname(root_program, program->arch, needed);
      if (shared) {
        if (strlen(shared->path) >= sizeof(dep->path))
          return -1;
        strcpy(dep->path, shared->path);
        dep->program = shared->program;
        dep->shared_from = shared;
        printf("POLYEXEC_DEP_SONAME_REUSE: arch=%s soname=%s requested_by=%s path=%s\n",
          program->arch_name, needed, program->path, shared->path);
        program->dep_count++;
        continue;
      }
    }
    if (find_process_needed_path(program->path, needed,
          process_platform_name_for_arch(program->arch), runpath, runpath_len,
          dep->path, sizeof(dep->path)) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: bad DT_NEEDED path: %s: %s\n",
        program->path, needed);
      return -1;
    }

    dep->program = calloc(1, sizeof(*dep->program));
    if (!dep->program) {
      fprintf(stderr, "POLYEXEC_FAIL: out of memory loading dependency: %s\n",
        dep->path);
      return -1;
    }
    if (load_elf_program(dep->path, "", dep->program) < 0) {
      free(dep->program);
      dep->program = NULL;
      return -1;
    }
    program->dep_count++;
    if (load_process_dependencies_at_depth(dep->program, root_program,
          depth + 1) < 0)
      return -1;
  }

  return 0;
}

static int load_process_dependencies(struct poly_program *program) {
  const char *preload = getenv("POLY_LD_PRELOAD");
  if (!preload || preload[0] == '\0')
    preload = getenv("LD_PRELOAD");
  if (preload && preload[0] != '\0') {
    size_t offset = 0;
    while (preload[offset] != '\0') {
      while (preload[offset] != '\0' && preload_separator(preload[offset]))
        offset++;
      const size_t start = offset;
      while (preload[offset] != '\0' && !preload_separator(preload[offset]))
        offset++;
      const size_t token_len = offset - start;
      if (token_len == 0)
        continue;

      char path[MAX_DEP_PATH];
      if (build_process_preload_path(program, preload + start, token_len,
            path, sizeof(path)) < 0) {
        fprintf(stderr, "POLYEXEC_FAIL: bad preload dependency path: %.*s\n",
          (int) token_len, preload + start);
        return -1;
      }
      if (process_dependency_path_already_loaded(program, path))
        continue;
      if (program->dep_count >= MAX_PROCESS_DEPS) {
        fprintf(stderr, "POLYEXEC_FAIL: too many preload dependencies: %s\n",
          program->path);
        return -1;
      }
      struct poly_process_dependency *dep = &program->deps[program->dep_count];
      if (strlen(path) >= sizeof(dep->path))
        return -1;
      strcpy(dep->path, path);
      dep->program = calloc(1, sizeof(*dep->program));
      if (!dep->program) {
        fprintf(stderr, "POLYEXEC_FAIL: out of memory loading dependency: %s\n",
          dep->path);
        return -1;
      }
      if (load_elf_program(dep->path, "", dep->program) < 0) {
        free(dep->program);
        dep->program = NULL;
        return -1;
      }
      program->dep_count++;
      if (load_process_dependencies_at_depth(dep->program, program, 1) < 0)
        return -1;
    }
  }
  return load_process_dependencies_at_depth(program, program, 0);
}

static void unmap_process_dependencies(struct poly_program *program) {
  for (size_t d = 0; d < program->dep_count; d++) {
    struct poly_process_dependency *dep = &program->deps[d];
    if (dep->shared_from) {
      dep->mapping = NULL;
      dep->mapping_size = 0;
      dep->loaded_image = NULL;
      continue;
    }
    if (dep->program)
      unmap_process_dependencies(dep->program);
    if (dep->mapping && dep->mapping_size)
      munmap(dep->mapping, dep->mapping_size);
    dep->mapping = NULL;
    dep->mapping_size = 0;
    dep->loaded_image = NULL;
  }
}

static void set_process_dependency_root_scope(struct poly_program *program,
    const struct poly_program *root_program, uint8_t *root_loaded_image) {
  program->scope_root_program = root_program;
  program->scope_root_loaded_image = root_loaded_image;
  for (size_t d = 0; d < program->dep_count; d++) {
    struct poly_process_dependency *dep = &program->deps[d];
    if (dep->program && !dep->shared_from)
      set_process_dependency_root_scope(dep->program, root_program,
        root_loaded_image);
  }
}

static int process_tree_has_arch_tls(const struct poly_program *program,
    int arch) {
  if (program->arch == arch && program->tls_memsz != 0)
    return 1;
  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_process_dependency *dep = &program->deps[d];
    if (dep->program && process_tree_has_arch_tls(dep->program, arch))
      return 1;
  }
  return 0;
}

static int reserve_process_tls_tree(struct poly_program *program,
    size_t *total_size) {
  if (reserve_process_tls_range(total_size, program->tls_memsz,
        program->tls_align, &program->tls_offset) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported process TLS layout: %s\n",
      program->path);
    return -1;
  }
  for (size_t d = 0; d < program->dep_count; d++) {
    if (program->deps[d].program && !program->deps[d].shared_from &&
        reserve_process_tls_tree(program->deps[d].program, total_size) < 0)
      return -1;
  }
  program->tls_total_size = *total_size;
  return 0;
}

static int copy_process_tls_image(const struct poly_program *program,
    const uint8_t *loaded_image, uint8_t *tls, size_t tls_size) {
  if (program->tls_memsz == 0)
    return 0;
  size_t tls_image_offset = 0;
  if (elf_vaddr_to_image_offset(program, program->tls_vaddr,
        program->tls_filesz, &tls_image_offset) < 0 ||
      program->tls_offset > tls_size ||
      program->tls_filesz > tls_size - program->tls_offset) {
    fprintf(stderr, "POLYEXEC_FAIL: process TLS image escaped image: %s\n",
      program->path);
    return -1;
  }
  memcpy(tls + program->tls_offset, loaded_image + tls_image_offset,
    (size_t) program->tls_filesz);
  return 0;
}

static int copy_process_dependency_tls_images(const struct poly_program *program,
    uint8_t *tls, size_t tls_size) {
  for (size_t d = 0; d < program->dep_count; d++) {
    const struct poly_process_dependency *dep = &program->deps[d];
    if (!dep->program || !dep->loaded_image)
      continue;
    if (dep->shared_from)
      continue;
    if (copy_process_dependency_tls_images(dep->program, tls, tls_size) < 0 ||
        copy_process_tls_image(dep->program, dep->loaded_image, tls,
          tls_size) < 0)
      return -1;
  }
  return 0;
}

static int map_process_dependencies(struct poly_program *program,
    uint8_t *trampoline_code, size_t prefix_size, uint64_t return_pc,
    const uint64_t tlsdesc_helpers[POLY_ARCH_COUNT],
    const uint64_t tls_get_addr_helpers[POLY_ARCH_COUNT],
    uint8_t *scratch) {
  for (size_t d = 0; d < program->dep_count; d++) {
    struct poly_process_dependency *dep = &program->deps[d];
    if (dep->shared_from) {
      struct poly_process_dependency *shared =
        canonical_process_dependency(dep->shared_from);
      if (!shared || !shared->loaded_image) {
        fprintf(stderr, "POLYEXEC_FAIL: shared dependency not mapped: %s\n",
          dep->path);
        return -1;
      }
      dep->mapping = shared->mapping;
      dep->mapping_size = shared->mapping_size;
      dep->loaded_image = shared->loaded_image;
      continue;
    }
    if (map_process_dependencies(dep->program, trampoline_code, prefix_size,
          return_pc, tlsdesc_helpers, tls_get_addr_helpers, scratch) < 0)
      return -1;
    const uint64_t mapping_size_u64 =
      align_up_u64((uint64_t) dep->program->code_size, 0x1000);
    if (mapping_size_u64 == 0 || mapping_size_u64 > SIZE_MAX) {
      fprintf(stderr, "POLYEXEC_FAIL: dependency mapping is too large: %s\n",
        dep->path);
      return -1;
    }
    dep->mapping_size = (size_t) mapping_size_u64;
    dep->mapping = mmap(NULL, dep->mapping_size,
      PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (dep->mapping == MAP_FAILED) {
      fprintf(stderr, "POLYEXEC_FAIL: dependency mmap failed: %s: %s\n",
        dep->path, strerror(errno));
      dep->mapping = NULL;
      return -1;
    }
    dep->loaded_image = dep->mapping;
    memcpy(dep->loaded_image, dep->program->code_bytes,
      dep->program->code_size);
    const uint64_t dep_load_bias =
      (uint64_t) (uintptr_t) dep->loaded_image - dep->program->base_vaddr;
    printf("POLYEXEC_DEP_LOAD: arch=%s at_base=0x%llx path=%s needed_by=%s\n",
      dep->program->arch_name, (unsigned long long) dep_load_bias,
      dep->path, program->path);

    if (apply_relative_relocations(dep->program, dep->loaded_image,
          trampoline_code, prefix_size, return_pc,
          process_helper_for_arch(tlsdesc_helpers, dep->program->arch),
          process_helper_for_arch(tls_get_addr_helpers, dep->program->arch),
          scratch) < 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unsupported dependency relocations: %s\n",
        dep->path);
      return -1;
    }
    if (protect_load_segments(dep->program, dep->loaded_image) < 0)
      return -1;
    if (protect_image_range(dep->program, dep->loaded_image,
          dep->program->relro_vaddr, dep->program->relro_size, PROT_READ,
          "dependency PT_GNU_RELRO") < 0)
      return -1;
    if (run_process_initializers(dep->program, dep->loaded_image,
          trampoline_code, prefix_size, return_pc, scratch,
          "dependency") < 0)
      return -1;
  }
  return 0;
}

static int emit_and_run(const struct poly_program *program, uint64_t *result,
    int prepare_scratch) {
  const size_t prefix_size = poly_trampoline_prefix_size(program->arch);
  if (prefix_size == 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported trampoline arch: %s\n",
      program->path);
    return -1;
  }
  const size_t load_base_offset = 4096;
  const size_t code_offset = load_base_offset - prefix_size;
  const size_t escape_return_size = program->arch == POLY_ARCH_X86 ? 1 : 5;
  const uint64_t image_mapping_size_u64 =
    align_up_u64((uint64_t) program->code_size + escape_return_size, 0x1000);
  if (image_mapping_size_u64 > SIZE_MAX - load_base_offset - 4096) {
    fprintf(stderr, "POLYEXEC_FAIL: ELF image mapping is too large: %s\n",
      program->path);
    return -1;
  }
  const size_t image_mapping_size = (size_t) image_mapping_size_u64;
  const size_t return_page_offset = load_base_offset + image_mapping_size;
  const size_t mapping_size = return_page_offset + 4096;
  uint8_t *mapping = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: mmap failed: %s\n", strerror(errno));
    return -1;
  }

  uint8_t *code = mapping + code_offset;
  const uint64_t return_pc = (uint64_t) (uintptr_t)
    (mapping + return_page_offset);
  const uint64_t entry_pc = (uint64_t) (uintptr_t) (mapping + load_base_offset + program->entry_offset);
  const uint32_t escape = program->arch == POLY_ARCH_AARCH64 ?
    POLY_AARCH64_CTRL_X86_ESCAPE : POLY_RISCV_CTRL_X86_ESCAPE;
  size_t offset = load_base_offset;
  emit_bytes(mapping, &offset, program->code_bytes, program->code_size);
  if (program->arch != POLY_ARCH_X86)
    emit_u32(mapping, &offset, escape);
  mapping[offset++] = 0xc3;
  offset = return_page_offset;
  if (program->arch != POLY_ARCH_X86)
    emit_u32(mapping, &offset, escape);
  mapping[offset++] = 0xc3;
  if (align_up_size(offset, 4, &offset) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }
  uint64_t tlsdesc_helper_pc = 0;
  if (emit_process_tlsdesc_helper_for_arch(program->arch, mapping, &offset,
        &tlsdesc_helper_pc) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }
  uint64_t tls_get_addr_helper_pc = 0;
  if (emit_process_tls_get_addr_helper_for_arch(program->arch, mapping,
        &offset,
        &tls_get_addr_helper_pc) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }

  size_t scratch_size = 4096;
  uint8_t *scratch = mmap(NULL, scratch_size, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (scratch == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: scratch mmap failed: %s\n",
      strerror(errno));
    munmap(mapping, mapping_size);
    return -1;
  }
  if (apply_relative_relocations(program, mapping + load_base_offset, code,
        prefix_size, return_pc, tlsdesc_helper_pc, tls_get_addr_helper_pc,
        scratch) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported dynamic relocations: %s\n",
      program->path);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (emit_poly_trampoline(program, code, prefix_size, return_pc, entry_pc, 0) < 0) {
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (protect_load_segments(program, mapping + load_base_offset) < 0) {
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (protect_image_range(program, mapping + load_base_offset,
        program->relro_vaddr, program->relro_size, PROT_READ,
        "PT_GNU_RELRO") < 0) {
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }

  if (prepare_scratch &&
      prepare_program_scratch(program->path, (char *) scratch,
        scratch_size) < 0) {
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (poly_thread_atomic_counter != NULL) {
    if (scratch_size < 4 * sizeof(uint64_t)) {
      fprintf(stderr, "POLYEXEC_FAIL: atomic scratch page is too small\n");
      munmap(scratch, scratch_size);
      munmap(mapping, mapping_size);
      return -1;
    }
    uint64_t *scratch_words = (uint64_t *) (void *) scratch;
    scratch_words[0] = (uint64_t) (uintptr_t) poly_thread_atomic_counter;
    scratch_words[1] = poly_thread_atomic_iterations;
    scratch_words[2] = poly_thread_atomic_index;
    scratch_words[3] = poly_thread_atomic_count;
  }
  char cwd[4096];
  int have_cwd = getcwd(cwd, sizeof(cwd)) != NULL;
  *result = run_poly_entry(code, scratch);
  if (have_cwd && chdir(cwd) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to restore cwd after %s: %s\n",
      program->path, strerror(errno));
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  poly_mode_x86();
  munmap(scratch, scratch_size);
  munmap(mapping, mapping_size);
  return 0;
}

static int run_poly_page_fault_selftest(void) {
  static const uint32_t code_words[] = {
    0xaa1f03e1U, // mov x1,xzr: use a real zero base, not SP.
    0xf9400020U, // ldr x0,[x1]: deliberate load from address 0.
    0xd65f03c0U  // ret: reaching this means the fault path failed.
  };
  struct poly_program program;
  memset(&program, 0, sizeof(program));
  program.path = "<polyexec-pagefault-selftest>";
  program.arch_name = "aarch64";
  program.arch = POLY_ARCH_AARCH64;
  program.code_bytes = (uint8_t *) (uintptr_t) code_words;
  program.code_size = sizeof(code_words);

  uint64_t result = 0;
  puts("POLYEXEC_SELFTEST_PAGEFAULT: start");
  if (emit_and_run(&program, &result, 0) < 0)
    return -1;
  fprintf(stderr,
    "POLYEXEC_FAIL: page-fault self-test returned without SIGSEGV result=%llu\n",
    (unsigned long long) result);
  return -1;
}

static int emit_and_run_process(struct poly_program *program,
    const struct poly_request *request, int extra_argc, char **extra_argv,
    uint64_t *result, int use_trap_vector) {
  reset_process_brk_arena();
  process_cross_report_path = program->path;
  process_cross_state_key_stub_count = 0;
  process_cross_aarch64_to_riscv_stub_count = 0;
  process_cross_riscv_to_aarch64_stub_count = 0;
  process_cross_aarch64_to_x86_stub_count = 0;
  process_cross_riscv_to_x86_stub_count = 0;
  process_cross_x86_to_aarch64_stub_count = 0;
  process_cross_x86_to_riscv_stub_count = 0;
  process_cross_signature_slot_stub_count = 0;
  process_cross_register_signature_stub_count = 0;
  process_cross_stack_bridge_stub_count = 0;
  process_cross_compact_shuffle_stub_count = 0;
  process_cross_x86_wrapper_stub_count = 0;
  process_cross_state_key_stub_reported = 0;

  const size_t prefix_size = poly_trampoline_prefix_size(program->arch);
  if (prefix_size == 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported process trampoline arch: %s\n",
      program->path);
    return -1;
  }
  const size_t load_base_offset = 4096;
  const size_t code_offset = load_base_offset - prefix_size;
  const size_t escape_return_size = program->arch == POLY_ARCH_X86 ? 1 : 5;
  const size_t fixed_control_size = 64 * 1024;
  const int fixed_main_image = program->is_et_exec;
  const uint64_t image_mapping_size_u64 =
    align_up_u64((uint64_t) program->code_size +
      (fixed_main_image ? 0 : escape_return_size), 0x1000);
  if (image_mapping_size_u64 == 0 ||
      image_mapping_size_u64 > SIZE_MAX ||
      (!fixed_main_image &&
       image_mapping_size_u64 > SIZE_MAX - load_base_offset - 4096) ||
      (fixed_main_image &&
       image_mapping_size_u64 > SIZE_MAX - fixed_control_size)) {
    fprintf(stderr, "POLYEXEC_FAIL: ELF image mapping is too large: %s\n",
      program->path);
    return -1;
  }
  const size_t image_mapping_size = (size_t) image_mapping_size_u64;
  const size_t image_offset = fixed_main_image ? 0 : load_base_offset;
  const size_t control_offset =
    fixed_main_image ? image_mapping_size : code_offset;
  const size_t lifecycle_return_page_offset = fixed_main_image ?
    image_mapping_size + 4096 : load_base_offset + image_mapping_size;
  const size_t process_return_page_offset =
    lifecycle_return_page_offset + 4096;
  const size_t mapping_size = fixed_main_image ?
    image_mapping_size + fixed_control_size : process_return_page_offset + 4096;
  if (fixed_main_image &&
      (program->base_vaddr > UINTPTR_MAX ||
       (uint64_t) mapping_size > UINTPTR_MAX - program->base_vaddr)) {
    fprintf(stderr, "POLYEXEC_FAIL: fixed ET_EXEC image address is invalid: %s\n",
      program->path);
    return -1;
  }

  void *desired_mapping = fixed_main_image ?
    (void *) (uintptr_t) program->base_vaddr : NULL;
  const int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS |
    (fixed_main_image ? MAP_FIXED_NOREPLACE : 0);
  uint8_t *mapping = mmap(desired_mapping, mapping_size,
    PROT_READ | PROT_WRITE | PROT_EXEC, mmap_flags, -1, 0);
  if (mapping == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: %s mmap failed: %s: %s\n",
      fixed_main_image ? "fixed ET_EXEC" : "process",
      program->path, strerror(errno));
    return -1;
  }
  if (fixed_main_image && mapping != (uint8_t *) desired_mapping) {
    fprintf(stderr, "POLYEXEC_FAIL: fixed ET_EXEC mmap used wrong address: %s\n",
      program->path);
    munmap(mapping, mapping_size);
    return -1;
  }

  uint8_t *loaded_image = mapping + image_offset;
  uint8_t *code = mapping + control_offset;
  const uint64_t main_load_bias =
    (uint64_t) (uintptr_t) loaded_image - program->base_vaddr;
  const uint64_t lifecycle_return_pc = (uint64_t) (uintptr_t)
    (mapping + lifecycle_return_page_offset);
  const uint64_t process_return_pc = (uint64_t) (uintptr_t)
    (mapping + process_return_page_offset);
  const uint64_t entry_pc = (uint64_t) (uintptr_t)
    (loaded_image + program->entry_offset);
  const uint32_t escape = program->arch == POLY_ARCH_AARCH64 ?
    POLY_AARCH64_CTRL_X86_ESCAPE : POLY_RISCV_CTRL_X86_ESCAPE;
  size_t offset = image_offset;
  emit_bytes(mapping, &offset, program->code_bytes, program->code_size);
  if (!fixed_main_image) {
    if (program->arch != POLY_ARCH_X86)
      emit_u32(mapping, &offset, escape);
    mapping[offset++] = 0xc3;
  }
  offset = lifecycle_return_page_offset;
  if (program->arch != POLY_ARCH_X86)
    emit_u32(mapping, &offset, escape);
  mapping[offset++] = 0xc3;
  offset = process_return_page_offset;
  if (program->arch != POLY_ARCH_X86)
    emit_u32(mapping, &offset, escape);
  emit_x86_exit_group_from_eax(mapping, &offset);
  if (align_up_size(offset, 4, &offset) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }
  uint64_t tlsdesc_helpers[POLY_ARCH_COUNT] = {0};
  uint64_t tls_get_addr_helpers[POLY_ARCH_COUNT] = {0};
  if (emit_process_tls_helpers_for_arches(mapping, &offset, tlsdesc_helpers,
        tls_get_addr_helpers) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }

  size_t scratch_size = 4096;
  uint8_t *scratch = mmap(NULL, scratch_size, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (scratch == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: scratch mmap failed: %s\n",
      strerror(errno));
    munmap(mapping, mapping_size);
    return -1;
  }
  set_process_dependency_root_scope(program, program, loaded_image);
  size_t process_tls_size = 0;
  if (reserve_process_tls_tree(program, &process_tls_size) < 0) {
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  // Direct foreign->x86 PCALL installs the process TLS base as x86 FSBASE only
  // for the callee window, then restores the runtime FSBASE on return.
  process_runtime_needs_x86_tls_wrapper = 0;
  if (map_process_dependencies(program, code, prefix_size, lifecycle_return_pc,
        tlsdesc_helpers, tls_get_addr_helpers, scratch) < 0) {
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (apply_relative_relocations(program, loaded_image, code,
        prefix_size, lifecycle_return_pc,
        process_helper_for_arch(tlsdesc_helpers, program->arch),
        process_helper_for_arch(tls_get_addr_helpers, program->arch),
        scratch) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported dynamic relocations: %s\n",
      program->path);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (emit_poly_trampoline(program, code, prefix_size, lifecycle_return_pc,
        entry_pc, 0) < 0) {
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  uint8_t *process_tls = NULL;
  if (process_tls_size != 0) {
    process_tls = mmap(NULL, process_tls_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (process_tls == MAP_FAILED) {
      fprintf(stderr, "POLYEXEC_FAIL: process TLS mmap failed: %s\n",
        strerror(errno));
      unmap_process_dependencies(program);
      munmap(scratch, scratch_size);
      munmap(mapping, mapping_size);
      return -1;
    }
    if (copy_process_tls_image(program, loaded_image,
          process_tls, process_tls_size) < 0 ||
        copy_process_dependency_tls_images(program, process_tls,
          process_tls_size) < 0) {
      munmap(process_tls, process_tls_size);
      unmap_process_dependencies(program);
      munmap(scratch, scratch_size);
      munmap(mapping, mapping_size);
      return -1;
    }
  }
  if (protect_load_segments(program, loaded_image) < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  const int static_et_exec = program->is_et_exec && program->dynamic_size == 0;
  if (!static_et_exec &&
      protect_image_range(program, loaded_image,
        program->relro_vaddr, program->relro_size, PROT_READ,
        "PT_GNU_RELRO") < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (run_process_preinitializers(program, loaded_image, code,
        prefix_size, lifecycle_return_pc, scratch) < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (run_process_initializers(program, loaded_image, code,
        prefix_size, lifecycle_return_pc, scratch, "root") < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (emit_poly_trampoline(program, code, prefix_size, process_return_pc,
        entry_pc, 1) < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }

  if (prepare_program_scratch(program->path, (char *) scratch,
        scratch_size) < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }

  uint8_t *process_stack = NULL;
  size_t process_stack_size = 0;
  uint64_t initial_sp = 0;
  uint64_t at_sysinfo_ehdr = 0;
  if (program->arch == POLY_ARCH_AARCH64 &&
      map_process_aarch64_vdso(&at_sysinfo_ehdr) < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (build_process_stack(program, request, loaded_image, 0, at_sysinfo_ehdr,
        extra_argc, extra_argv, &process_stack, &process_stack_size,
        &initial_sp) < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }

  poly_process_exit_finalizers =
    (struct poly_process_exit_finalizer_context) {
      .program = program,
      .loaded_image = loaded_image,
      .trampoline_code = code,
      .prefix_size = prefix_size,
      .return_pc = lifecycle_return_pc,
      .scratch = scratch,
      .active = 1,
      .run_finalizers = 1,
    };

  (void) result;
  process_runtime_x86_tls_base =
    process_tls != NULL &&
    (program->arch == POLY_ARCH_X86 ||
     process_tree_has_arch_tls(program, POLY_ARCH_X86)) ?
    (uint64_t) (uintptr_t) process_tls : 0;
  if (process_runtime_x86_tls_base != 0 &&
      poly_abi_signature_set_with_flags(process_native_signature_slot,
        POLY_ABI_SIGNATURE_KIND_NATIVE_REGS,
        POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE) != 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native TLS signature slot setup failed slot=%u\n",
      process_native_signature_slot);
    munmap(process_tls, process_tls_size);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  process_runtime_host_fs_base = get_x86_fs_base();
  const uint64_t startup_x86_tls_base =
    program->arch == POLY_ARCH_X86 ? process_runtime_x86_tls_base : 0;
  const uint64_t process_state_key = polyexec_use_explicit_state_key ?
    (uint64_t) (uintptr_t) &poly_state_key_anchor : 0;
  printf("POLYEXEC_MAIN_LOAD: arch=%s at_base=0x%llx entry=0x%llx path=%s\n",
    program->arch_name, (unsigned long long) main_load_bias,
    (unsigned long long) entry_pc, program->path);
  poly_restore_passthrough_stdout();
  run_poly_process_entry(code, initial_sp, (uint64_t) (uintptr_t) process_tls,
    startup_x86_tls_base, process_state_key, use_trap_vector, program->arch);
}

static int resolve_process_interpreter_path(const struct poly_program *program,
    char *out, size_t out_size) {
  if (!program->has_interp || program->interp_path[0] == '\0')
    return -1;
  if (strlen(program->interp_path) < out_size &&
      access(program->interp_path, R_OK) == 0) {
    strcpy(out, program->interp_path);
    return 0;
  }

  const char *interp_name = strrchr(program->interp_path, '/');
  interp_name = interp_name ? interp_name + 1 : program->interp_path;
  const char *arch_dir = program->arch == POLY_ARCH_AARCH64 ? "aarch64" :
    program->arch == POLY_ARCH_RISCV ? "riscv64" : "x86_64";
  const char *slash = strrchr(program->path, '/');
  const size_t dir_len = slash ? (size_t) (slash - program->path) : 0;
  if (dir_len + strlen("/processdeps/") + strlen(arch_dir) + 1 +
      strlen(interp_name) + 1 > out_size)
    return -1;
  memcpy(out, program->path, dir_len);
  out[dir_len] = '\0';
  strcat(out, "/processdeps/");
  strcat(out, arch_dir);
  strcat(out, "/");
  strcat(out, interp_name);
  if (access(out, R_OK) == 0)
    return 0;
  return -1;
}

static int map_process_exec_image(const struct poly_program *program,
    int include_control, uint8_t **mapping_out, size_t *mapping_size_out,
    uint8_t **loaded_image_out, uint8_t **code_out,
    uint64_t *return_pc_out) {
  const size_t prefix_size = poly_trampoline_prefix_size(program->arch);
  const size_t load_base_offset = 4096;
  const uint64_t image_mapping_size_u64 =
    align_up_u64((uint64_t) program->code_size, 0x1000);
  if (image_mapping_size_u64 == 0 || image_mapping_size_u64 > SIZE_MAX ||
      (!program->is_et_exec &&
       image_mapping_size_u64 > SIZE_MAX - load_base_offset)) {
    fprintf(stderr, "POLYEXEC_FAIL: interpreter image mapping is too large: %s\n",
      program->path);
    return -1;
  }
  const size_t image_mapping_size = (size_t) image_mapping_size_u64;
  const size_t image_offset = program->is_et_exec ? 0 : load_base_offset;
  size_t mapping_size = image_offset + image_mapping_size;
  size_t control_offset = 0;
  size_t return_page_offset = 0;
  if (include_control) {
    if (prefix_size == 0) {
      fprintf(stderr, "POLYEXEC_FAIL: unsupported interpreter trampoline arch: %s\n",
        program->path);
      return -1;
    }
    control_offset = mapping_size;
    return_page_offset = control_offset + 4096;
    if (mapping_size > SIZE_MAX - 8192) {
      fprintf(stderr, "POLYEXEC_FAIL: interpreter control mapping is too large: %s\n",
        program->path);
      return -1;
    }
    mapping_size += 8192;
  }
  if (program->is_et_exec &&
      (program->base_vaddr > UINTPTR_MAX ||
       (uint64_t) mapping_size > UINTPTR_MAX - program->base_vaddr)) {
    fprintf(stderr, "POLYEXEC_FAIL: fixed interpreter address is invalid: %s\n",
      program->path);
    return -1;
  }

  void *desired_mapping = program->is_et_exec ?
    (void *) (uintptr_t) program->base_vaddr : NULL;
  const int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS |
    (program->is_et_exec ? MAP_FIXED_NOREPLACE : 0);
  uint8_t *mapping = mmap(desired_mapping, mapping_size,
    PROT_READ | PROT_WRITE | PROT_EXEC, mmap_flags, -1, 0);
  if (mapping == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: interpreter mmap failed: %s: %s\n",
      program->path, strerror(errno));
    return -1;
  }
  if (program->is_et_exec && mapping != (uint8_t *) desired_mapping) {
    fprintf(stderr, "POLYEXEC_FAIL: fixed interpreter mmap used wrong address: %s\n",
      program->path);
    munmap(mapping, mapping_size);
    return -1;
  }

  uint8_t *loaded_image = mapping + image_offset;
  memcpy(loaded_image, program->code_bytes, program->code_size);
  if (include_control) {
    const uint32_t escape = program->arch == POLY_ARCH_AARCH64 ?
      POLY_AARCH64_CTRL_X86_ESCAPE : POLY_RISCV_CTRL_X86_ESCAPE;
    size_t offset = return_page_offset;
    if (program->arch != POLY_ARCH_X86)
      emit_u32(mapping, &offset, escape);
    emit_x86_exit_group_from_eax(mapping, &offset);
    *code_out = mapping + control_offset;
    *return_pc_out = (uint64_t) (uintptr_t) (mapping + return_page_offset);
  } else {
    *code_out = NULL;
    *return_pc_out = 0;
  }
  if (protect_load_segments(program, loaded_image) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }

  *mapping_out = mapping;
  *mapping_size_out = mapping_size;
  *loaded_image_out = loaded_image;
  return 0;
}

static int emit_and_run_process_interpreter(struct poly_program *program,
    const struct poly_request *request, int extra_argc, char **extra_argv,
    uint64_t *result, int use_trap_vector) {
  reset_process_brk_arena();
  process_cross_report_path = program->path;

  char interp_path[MAX_DEP_PATH];
  if (resolve_process_interpreter_path(program, interp_path,
        sizeof(interp_path)) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unable to resolve ELF interpreter: %s: %s\n",
      program->path, program->interp_path);
    return -1;
  }

  struct poly_program interp_program;
  if (load_elf_program(interp_path, "", &interp_program) < 0)
    return -1;
  if (interp_program.arch != program->arch) {
    fprintf(stderr, "POLYEXEC_FAIL: interpreter arch mismatch: %s\n",
      interp_path);
    free_program(&interp_program);
    return -1;
  }

  uint8_t *main_mapping = NULL;
  size_t main_mapping_size = 0;
  uint8_t *loaded_main = NULL;
  uint8_t *unused_code = NULL;
  uint64_t unused_return_pc = 0;
  if (map_process_exec_image(program, 0, &main_mapping, &main_mapping_size,
        &loaded_main, &unused_code, &unused_return_pc) < 0) {
    free_program(&interp_program);
    return -1;
  }

  uint8_t *interp_mapping = NULL;
  size_t interp_mapping_size = 0;
  uint8_t *loaded_interp = NULL;
  uint8_t *code = NULL;
  uint64_t process_return_pc = 0;
  if (map_process_exec_image(&interp_program, 1, &interp_mapping,
        &interp_mapping_size, &loaded_interp, &code,
        &process_return_pc) < 0) {
    munmap(main_mapping, main_mapping_size);
    free_program(&interp_program);
    return -1;
  }

  const size_t prefix_size = poly_trampoline_prefix_size(program->arch);
  const uint64_t interp_entry_pc = (uint64_t) (uintptr_t)
    (loaded_interp + interp_program.entry_offset);
  const uint64_t main_load_bias =
    (uint64_t) (uintptr_t) loaded_main - program->base_vaddr;
  const uint64_t interp_load_bias =
    (uint64_t) (uintptr_t) loaded_interp - interp_program.base_vaddr;
  if (emit_poly_trampoline(&interp_program, code, prefix_size,
        process_return_pc, interp_entry_pc, 1) < 0) {
    munmap(interp_mapping, interp_mapping_size);
    munmap(main_mapping, main_mapping_size);
    free_program(&interp_program);
    return -1;
  }

  size_t process_tls_size = 0;
  uint8_t *process_tls = NULL;
  if (reserve_process_tls_tree(&interp_program, &process_tls_size) < 0) {
    munmap(interp_mapping, interp_mapping_size);
    munmap(main_mapping, main_mapping_size);
    free_program(&interp_program);
    return -1;
  }
  if (process_tls_size != 0) {
    process_tls = mmap(NULL, process_tls_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (process_tls == MAP_FAILED) {
      fprintf(stderr, "POLYEXEC_FAIL: interpreter TLS mmap failed: %s\n",
        strerror(errno));
      munmap(interp_mapping, interp_mapping_size);
      munmap(main_mapping, main_mapping_size);
      free_program(&interp_program);
      return -1;
    }
    if (copy_process_tls_image(&interp_program, loaded_interp, process_tls,
          process_tls_size) < 0) {
      munmap(process_tls, process_tls_size);
      munmap(interp_mapping, interp_mapping_size);
      munmap(main_mapping, main_mapping_size);
      free_program(&interp_program);
      return -1;
    }
  }

  uint8_t *process_stack = NULL;
  size_t process_stack_size = 0;
  uint64_t initial_sp = 0;
  uint64_t at_sysinfo_ehdr = 0;
  if (program->arch == POLY_ARCH_AARCH64 &&
      map_process_aarch64_vdso(&at_sysinfo_ehdr) < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    munmap(interp_mapping, interp_mapping_size);
    munmap(main_mapping, main_mapping_size);
    free_program(&interp_program);
    return -1;
  }
  if (build_process_stack(program, request, loaded_main, interp_load_bias,
        at_sysinfo_ehdr, extra_argc, extra_argv, &process_stack, &process_stack_size,
        &initial_sp) < 0) {
    if (process_tls)
      munmap(process_tls, process_tls_size);
    munmap(interp_mapping, interp_mapping_size);
    munmap(main_mapping, main_mapping_size);
    free_program(&interp_program);
    return -1;
  }

  printf("POLYEXEC_INTERP_LOAD: arch=%s interp=%s resolved=%s at_base=0x%llx path=%s\n",
    program->arch_name, program->interp_path, interp_path,
    (unsigned long long) interp_load_bias, program->path);
  printf("POLYEXEC_MAIN_LOAD: arch=%s at_base=0x%llx entry=0x%llx path=%s\n",
    program->arch_name, (unsigned long long) main_load_bias,
    (unsigned long long) (main_load_bias + program->base_vaddr +
      (uint64_t) program->entry_offset),
    program->path);

  poly_process_exit_finalizers =
    (struct poly_process_exit_finalizer_context) {
      .program = program,
      .loaded_image = loaded_main,
      .trampoline_code = code,
      .prefix_size = prefix_size,
      .return_pc = process_return_pc,
      .scratch = NULL,
      .active = 1,
      .run_finalizers = 0,
    };

  (void) result;
  process_runtime_x86_tls_base = 0;
  process_runtime_host_fs_base = get_x86_fs_base();
  const uint64_t process_state_key = polyexec_use_explicit_state_key ?
    (uint64_t) (uintptr_t) &poly_state_key_anchor : 0;
  poly_restore_passthrough_stdout();
  run_poly_process_entry(code, initial_sp, (uint64_t) (uintptr_t) process_tls,
    0, process_state_key, use_trap_vector, program->arch);
}

static int program_exits_process(const char *path) {
  return strstr(path, "-exit.") != NULL ||
    strstr(path, "-exit-group.") != NULL;
}

static int emit_and_run_exit_child(const struct poly_program *program,
    uint64_t *result, int use_trap_vector) {
  fflush(NULL);
  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: fork failed for %s: %s\n",
      program->path, strerror(errno));
    return -1;
  }

  if (pid == 0) {
    uint64_t child_result = 0;
    poly_restore_passthrough_stdout();
    if (polyexec_use_auto_spill && install_poly_auto_spill() < 0)
      _exit(125);
    if (use_trap_vector)
      install_poly_trap_vector();
    if (emit_and_run(program, &child_result, 1) < 0)
      _exit(125);
    report_poly_monitor_packets();
    report_poly_syscall_summary();
    fflush(NULL);
    _exit((int) (child_result & 0xff));
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: waitpid failed for %s: %s\n",
      program->path, strerror(errno));
    return -1;
  }

  if (polyexec_use_explicit_state_key) {
    const uint64_t key = (uint64_t) (uintptr_t) &poly_state_key_anchor;
    if (poly_state_key_set(key) != 0 || poly_state_key_get() != key) {
      fprintf(stderr,
        "POLYEXEC_FAIL: parent Poly state-key restore failed for %s got=0x%llx\n",
        program->path, (unsigned long long) poly_state_key_get());
      return -1;
    }
  }
  poly_mode_x86();
  if (use_trap_vector)
    install_poly_trap_vector();

  if (WIFEXITED(status)) {
    printf("POLYEXEC_PROCESS_CHILD_STATUS: path=%s raw=0x%x exit=%d\n",
      program->path, status, WEXITSTATUS(status));
    *result = (uint64_t) WEXITSTATUS(status);
    return 0;
  }
  if (WIFSIGNALED(status)) {
    printf("POLYEXEC_PROCESS_CHILD_STATUS: path=%s raw=0x%x signal=%d\n",
      program->path, status, WTERMSIG(status));
    *result = (uint64_t) (128 + WTERMSIG(status));
    return 0;
  }

  fprintf(stderr, "POLYEXEC_FAIL: unexpected child status for %s: 0x%x\n",
    program->path, status);
  return -1;
}

static int poly_process_uses_real_interpreter(const struct poly_program *program,
    const struct poly_request *request, int extra_argc, char **extra_argv) {
  if (!program->has_interp || request->symbol[0])
    return 0;
  const char *force_interpreter = getenv("POLY_PROCESS_REAL_INTERPRETER");
  if (force_interpreter != NULL && strcmp(force_interpreter, "1") == 0)
    return 1;
  const char *base = strrchr(program->path, '/');
  base = base ? base + 1 : program->path;
  if (strcmp(base, "aarch64-real-ls.elf") == 0 ||
      strcmp(base, "aarch64-real-python3.elf") == 0 ||
      strcmp(base, "aarch64-process-exception-real.elf") == 0 ||
      strcmp(base, "aarch64-process-thread-hash-real.elf") == 0 ||
      strcmp(base, "aarch64-process-setjmp-real.elf") == 0 ||
      strcmp(base, "aarch64-process-vdso-time-real.elf") == 0)
    return 1;
  if (strcmp(base, "riscv-real-python3.elf") == 0 ||
      strcmp(base, "riscv-process-exception-real.elf") == 0 ||
      strcmp(base, "riscv-process-setjmp-real.elf") == 0)
    return 1;
  if (extra_argc != 1 || extra_argv == NULL || extra_argv[0] == NULL ||
      strcmp(extra_argv[0], "dynamic-libc") != 0)
    return 0;
  return strcmp(base, "aarch64-process-dynamic-libc-real.elf") == 0 ||
    strcmp(base, "riscv-process-dynamic-libc-real.elf") == 0;
}

static int emit_and_run_process_child(struct poly_program *program,
    const struct poly_request *request, int extra_argc, char **extra_argv,
    uint64_t *result, int use_trap_vector) {
  fflush(NULL);
  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: fork failed for %s: %s\n",
      program->path, strerror(errno));
    return -1;
  }

  if (pid == 0) {
    uint64_t child_result = 0;
    if (polyexec_use_auto_spill && install_poly_auto_spill() < 0)
      _exit(125);
    if (use_trap_vector)
      install_poly_trap_vector();
    const int use_interpreter = poly_process_uses_real_interpreter(program,
      request, extra_argc, extra_argv);
    if ((use_interpreter ?
          emit_and_run_process_interpreter(program, request, extra_argc,
            extra_argv, &child_result, use_trap_vector) :
          emit_and_run_process(program, request, extra_argc, extra_argv,
            &child_result, use_trap_vector)) < 0)
      _exit(125);
    poly_resume_stdout_diagnostics();
    report_poly_monitor_packets();
    report_poly_syscall_summary();
    fflush(NULL);
    _exit((int) (child_result & 0xff));
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: waitpid failed for %s: %s\n",
      program->path, strerror(errno));
    return -1;
  }

  if (WIFEXITED(status)) {
    fprintf(stderr, "POLYEXEC_PROCESS_CHILD_STATUS: path=%s raw=0x%x exit=%d\n",
      program->path, status, WEXITSTATUS(status));
    fflush(stderr);
    *result = (uint64_t) WEXITSTATUS(status);
    return 0;
  }
  if (WIFSIGNALED(status)) {
    fprintf(stderr, "POLYEXEC_PROCESS_CHILD_STATUS: path=%s raw=0x%x signal=%d\n",
      program->path, status, WTERMSIG(status));
    fflush(stderr);
    *result = (uint64_t) (128 + WTERMSIG(status));
    return 0;
  }

  fprintf(stderr, "POLYEXEC_FAIL: unexpected child status for %s: 0x%x\n",
    program->path, status);
  return -1;
}

static void *poly_thread_run_worker(void *arg) {
  struct poly_thread_run_context *ctx =
    (struct poly_thread_run_context *) arg;
  ctx->status = -1;
  ctx->native_tid = (long) syscall(SYS_gettid);

  if (install_poly_thread_state_key() < 0)
    return NULL;
  if (polyexec_use_auto_spill && install_poly_auto_spill() < 0)
    return NULL;
  ctx->spill_buffer = (uint64_t) (uintptr_t) poly_auto_spill_active_state();
  if (ctx->use_trap_vector)
    install_poly_trap_vector();

  while (!*ctx->start_flag)
    sched_yield();

  struct poly_program program;
  if (load_elf_program(ctx->request.path, ctx->request.symbol, &program) < 0) {
    clear_poly_auto_spill();
    return NULL;
  }

  if (ctx->atomic_mode) {
    poly_thread_atomic_counter = ctx->atomic_counter;
    poly_thread_atomic_iterations = ctx->atomic_iterations;
    poly_thread_atomic_index = (uint64_t) ctx->index;
    poly_thread_atomic_count = (uint64_t) ctx->thread_count;
  }

  uint64_t result = 0;
  if (emit_and_run(&program, &result, 0) == 0) {
    ctx->result = result;
    if (!ctx->request.check_expected || result == ctx->request.expected)
      ctx->status = 0;
    else
      fprintf(stderr,
        "POLYEXEC_FAIL: thread=%lu %s expected %llu got %llu\n",
        (unsigned long) ctx->index, ctx->request.path,
        (unsigned long long) ctx->request.expected,
        (unsigned long long) result);
  }

  poly_thread_atomic_counter = NULL;
  poly_thread_atomic_iterations = 0;
  poly_thread_atomic_index = 0;
  poly_thread_atomic_count = 0;
  free_program(&program);
  poly_mode_x86();
  if (ctx->use_trap_vector)
    clear_poly_trap_vector();
  clear_poly_auto_spill();
  return NULL;
}

static unsigned long poly_affinity_mask(void) {
  unsigned long mask = 0;
  if (syscall(SYS_sched_getaffinity, 0, sizeof(mask), &mask) < 0 || mask == 0)
    return 1;
  return mask;
}

static unsigned int poly_count_affinity_cpus(unsigned long mask) {
  unsigned int count = 0;
  for (unsigned int bit = 0; bit < sizeof(mask) * 8; bit++) {
    if ((mask & (1UL << bit)) != 0)
      count++;
  }
  return count;
}

static unsigned long poly_affinity_cpu_bit(unsigned long mask,
    uint64_t ordinal) {
  unsigned int count = poly_count_affinity_cpus(mask);
  if (count == 0)
    return 1;
  unsigned int wanted = (unsigned int) (ordinal % count);
  for (unsigned int bit = 0; bit < sizeof(mask) * 8; bit++) {
    if ((mask & (1UL << bit)) == 0)
      continue;
    if (wanted == 0)
      return 1UL << bit;
    wanted--;
  }
  return mask & (~mask + 1);
}

static int poly_set_tid_affinity(long tid, unsigned long mask) {
  if (tid <= 0 || mask == 0)
    return -1;
  return (int) syscall(SYS_sched_setaffinity, tid, sizeof(mask), &mask);
}

static void *poly_atomic_affinity_migrator(void *arg) {
  struct poly_atomic_migrator_context *ctx =
    (struct poly_atomic_migrator_context *) arg;
  uint64_t round = 0;
  ctx->status = 0;

  while (!*ctx->done_flag) {
    for (size_t n = 0; n < ctx->thread_count; n++) {
      long tid = ctx->contexts[n].native_tid;
      if (tid <= 0)
        continue;
      unsigned long mask = poly_affinity_cpu_bit(ctx->allowed_mask,
        round + n);
      if (poly_set_tid_affinity(tid, mask) == 0)
        ctx->migrations++;
      else if (errno != ESRCH)
        ctx->status = -1;
    }
    if ((round & 3) == 3) {
      for (size_t n = 0; n < ctx->thread_count; n++) {
        long tid = ctx->contexts[n].native_tid;
        if (tid > 0 && poly_set_tid_affinity(tid, ctx->allowed_mask) < 0 &&
            errno != ESRCH)
          ctx->status = -1;
      }
    }
    round++;
    sched_yield();
  }

  for (size_t n = 0; n < ctx->thread_count; n++) {
    long tid = ctx->contexts[n].native_tid;
    if (tid > 0)
      (void) poly_set_tid_affinity(tid, ctx->allowed_mask);
  }
  return NULL;
}

static int run_poly_thread_stress(const struct poly_request *request,
    size_t thread_count, int use_trap_vector) {
  if (thread_count == 0 || thread_count > 64) {
    fprintf(stderr,
      "POLYEXEC_FAIL: --threads count must be between 1 and 64\n");
    return -1;
  }

  pthread_t *threads = calloc(thread_count, sizeof(*threads));
  struct poly_thread_run_context *contexts =
    calloc(thread_count, sizeof(*contexts));
  if (threads == NULL || contexts == NULL) {
    fprintf(stderr, "POLYEXEC_FAIL: pthread stress allocation failed\n");
    free(threads);
    free(contexts);
    return -1;
  }

  int failed = 0;
  volatile int start_flag = 0;
  size_t created = 0;
  for (size_t n = 0; n < thread_count; n++) {
    contexts[n].request = *request;
    contexts[n].start_flag = &start_flag;
    contexts[n].use_trap_vector = use_trap_vector;
    contexts[n].index = n;
    contexts[n].status = -1;
    if (pthread_create(&threads[n], NULL, poly_thread_run_worker,
          &contexts[n]) != 0) {
      fprintf(stderr, "POLYEXEC_FAIL: pthread_create failed index=%zu\n", n);
      failed = 1;
      break;
    }
    created++;
  }
  start_flag = 1;

  for (size_t n = 0; n < created; n++) {
    if (pthread_join(threads[n], NULL) != 0) {
      fprintf(stderr, "POLYEXEC_FAIL: pthread_join failed index=%zu\n", n);
      failed = 1;
      continue;
    }
    if (contexts[n].status != 0)
      failed = 1;
  }

  for (size_t n = 0; n < created; n++) {
    if (contexts[n].spill_buffer == 0) {
      failed = 1;
      continue;
    }
    for (size_t m = n + 1; m < created; m++) {
      if (contexts[n].spill_buffer == contexts[m].spill_buffer) {
        fprintf(stderr,
          "POLYEXEC_FAIL: duplicate auto-spill buffer threads=%zu,%zu buffer=0x%llx\n",
          n, m, (unsigned long long) contexts[n].spill_buffer);
        failed = 1;
      }
    }
  }

  for (size_t n = 0; n < created; n++) {
    printf("POLYEXEC_THREAD_RESULT: index=%zu value=%llu spill_buffer=0x%llx path=%s\n",
      n, (unsigned long long) contexts[n].result,
      (unsigned long long) contexts[n].spill_buffer, request->path);
  }

  free(threads);
  free(contexts);
  return failed ? -1 : 0;
}

static int run_poly_atomic_thread_stress(const struct poly_request *request,
    size_t thread_count, uint64_t iterations, int use_trap_vector) {
  if (thread_count == 0 || thread_count > 64) {
    fprintf(stderr,
      "POLYEXEC_FAIL: --atomic-threads count must be between 1 and 64\n");
    return -1;
  }
  if (iterations == 0) {
    fprintf(stderr,
      "POLYEXEC_FAIL: --atomic-threads iterations must be nonzero\n");
    return -1;
  }

  unsigned long allowed_mask = poly_affinity_mask();
  unsigned int cpu_count = poly_count_affinity_cpus(allowed_mask);
  if (cpu_count < 2) {
    fprintf(stderr,
      "POLYEXEC_FAIL: --atomic-threads requires at least 2 runnable CPUs, got %u\n",
      cpu_count);
    return -1;
  }

  volatile uint64_t *counter = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (counter == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: atomic counter mmap failed: %s\n",
      strerror(errno));
    return -1;
  }
  *counter = 0;

  pthread_t *threads = calloc(thread_count, sizeof(*threads));
  struct poly_thread_run_context *contexts =
    calloc(thread_count, sizeof(*contexts));
  if (threads == NULL || contexts == NULL) {
    fprintf(stderr, "POLYEXEC_FAIL: atomic pthread allocation failed\n");
    free(threads);
    free(contexts);
    munmap((void *) counter, 4096);
    return -1;
  }

  int failed = 0;
  volatile int start_flag = 0;
  volatile int done_flag = 0;
  size_t created = 0;
  for (size_t n = 0; n < thread_count; n++) {
    contexts[n].request = *request;
    contexts[n].start_flag = &start_flag;
    contexts[n].use_trap_vector = use_trap_vector;
    contexts[n].atomic_mode = 1;
    contexts[n].index = n;
    contexts[n].thread_count = thread_count;
    contexts[n].atomic_iterations = iterations;
    contexts[n].atomic_counter = counter;
    contexts[n].status = -1;
    if (pthread_create(&threads[n], NULL, poly_thread_run_worker,
          &contexts[n]) != 0) {
      fprintf(stderr, "POLYEXEC_FAIL: atomic pthread_create failed index=%zu\n",
        n);
      failed = 1;
      break;
    }
    created++;
  }

  for (;;) {
    size_t ready = 0;
    for (size_t n = 0; n < created; n++) {
      if (contexts[n].native_tid > 0)
        ready++;
    }
    if (ready == created)
      break;
    sched_yield();
  }

  struct poly_atomic_migrator_context migrator = {
    .contexts = contexts,
    .thread_count = created,
    .done_flag = &done_flag,
    .allowed_mask = allowed_mask,
    .cpu_count = cpu_count,
    .migrations = 0,
    .status = 0,
  };
  pthread_t migrator_thread;
  int have_migrator =
    pthread_create(&migrator_thread, NULL, poly_atomic_affinity_migrator,
      &migrator) == 0;
  if (!have_migrator) {
    fprintf(stderr, "POLYEXEC_FAIL: atomic affinity migrator create failed\n");
    failed = 1;
  }

  start_flag = 1;
  for (size_t n = 0; n < created; n++) {
    if (pthread_join(threads[n], NULL) != 0) {
      fprintf(stderr, "POLYEXEC_FAIL: atomic pthread_join failed index=%zu\n",
        n);
      failed = 1;
      continue;
    }
    if (contexts[n].status != 0)
      failed = 1;
  }
  done_flag = 1;
  if (have_migrator && pthread_join(migrator_thread, NULL) != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: atomic migrator join failed\n");
    failed = 1;
  }
  if (migrator.status != 0) {
    fprintf(stderr, "POLYEXEC_FAIL: atomic affinity churn failed\n");
    failed = 1;
  }

  const uint64_t expected_counter = (uint64_t) thread_count * iterations;
  const uint64_t actual_counter = *counter;
  if (actual_counter != expected_counter) {
    fprintf(stderr,
      "POLYEXEC_FAIL: atomic shared counter expected=%llu got=%llu path=%s\n",
      (unsigned long long) expected_counter,
      (unsigned long long) actual_counter, request->path);
    failed = 1;
  }
  if (migrator.migrations == 0) {
    fprintf(stderr, "POLYEXEC_FAIL: atomic affinity churn made no migrations\n");
    failed = 1;
  }

  for (size_t n = 0; n < created; n++) {
    printf("POLYEXEC_THREAD_RESULT: index=%zu value=%llu spill_buffer=0x%llx path=%s\n",
      n, (unsigned long long) contexts[n].result,
      (unsigned long long) contexts[n].spill_buffer, request->path);
  }

  if (!failed) {
    printf("POLYEXEC_AFFINITY_CHURN_OK: cpus=%u migrations=%llu threads=%zu path=%s\n",
      migrator.cpu_count, (unsigned long long) migrator.migrations,
      thread_count, request->path);
    printf("POLYEXEC_ATOMIC_THREADS_OK: threads=%zu iterations=%llu counter=%llu path=%s\n",
      thread_count, (unsigned long long) iterations,
      (unsigned long long) actual_counter, request->path);
  }

  free(threads);
  free(contexts);
  munmap((void *) counter, 4096);
  return failed ? -1 : 0;
}

static void free_program(struct poly_program *program) {
  unmap_process_dependencies(program);
  for (size_t d = 0; d < program->dep_count; d++) {
    if (program->deps[d].program) {
      if (!program->deps[d].shared_from) {
        free_program(program->deps[d].program);
        free(program->deps[d].program);
      }
      program->deps[d].program = NULL;
      program->deps[d].shared_from = NULL;
    }
  }
  program->dep_count = 0;
  free(program->code_bytes);
  program->code_bytes = NULL;
  program->code_size = 0;
}

static int apply_env_rlimit_nofile(void) {
  const char *value = getenv("POLYEXEC_RLIMIT_NOFILE");
  if (value == NULL || value[0] == '\0' || strcmp(value, "0") == 0)
    return 0;

  uint64_t parsed = 0;
  if (parse_u64(value, &parsed) < 0 || parsed == 0) {
    fprintf(stderr, "POLYEXEC_FAIL: invalid POLYEXEC_RLIMIT_NOFILE=%s\n",
      value);
    return -1;
  }

  struct rlimit limit;
  if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: getrlimit RLIMIT_NOFILE failed: %s\n",
      strerror(errno));
    return -1;
  }
  limit.rlim_cur = (rlim_t) parsed;
  if (limit.rlim_max < limit.rlim_cur)
    limit.rlim_max = limit.rlim_cur;
  if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: setrlimit RLIMIT_NOFILE=%s failed: %s\n",
      value, strerror(errno));
    return -1;
  }
  if (getrlimit(RLIMIT_NOFILE, &limit) == 0)
    printf("POLYEXEC_RLIMIT_NOFILE: soft=%llu hard=%llu\n",
      (unsigned long long) limit.rlim_cur,
      (unsigned long long) limit.rlim_max);
  return 0;
}

int main(int argc, char **argv) {
  if (getenv("POLYEXEC_REPORT_UID") != NULL) {
    printf("POLYEXEC_MONITOR_UID: uid=%ld euid=%ld gid=%ld egid=%ld\n",
      (long) getuid(), (long) geteuid(), (long) getgid(), (long) getegid());
  }

  if (argc < 2) {
    fprintf(stderr, "usage: %s foreign.elf[=expected]... | --process foreign.elf[=expected] [arg...] | --threads N foreign.elf[=expected] | --atomic-threads N ITERATIONS foreign.elf[=expected] | --selftest-pagefault\n",
      argv[0]);
    return 2;
  }

  poly_redirect_stdout_diagnostics();
  puts("POLYEXEC: start");
  if (apply_env_rlimit_nofile() < 0)
    return 2;
  if (prepare_syscall_fixture_file() < 0)
    return 1;
  const char *trap_vector_env = getenv("POLYEXEC_TRAP_VECTOR");
  const int use_trap_vector =
    trap_vector_env == NULL || strcmp(trap_vector_env, "0") != 0;
  const char *auto_spill_env = getenv("POLYEXEC_AUTO_SPILL");
  polyexec_use_auto_spill =
    auto_spill_env == NULL || strcmp(auto_spill_env, "0") != 0;
  const char *watchdog_env = getenv("POLYEXEC_WATCHDOG_SECONDS");
  if (watchdog_env != NULL && watchdog_env[0] != '\0' &&
      strcmp(watchdog_env, "0") != 0) {
    char *end = NULL;
    errno = 0;
    unsigned long seconds = strtoul(watchdog_env, &end, 10);
    if (errno != 0 || end == watchdog_env || *end != '\0' ||
        seconds > UINT32_MAX) {
      fprintf(stderr, "POLYEXEC_FAIL: invalid POLYEXEC_WATCHDOG_SECONDS=%s\n",
        watchdog_env);
      return 2;
    }
    polyexec_watchdog_seconds = (unsigned int) seconds;
  }
  if (read_poly_base_contract(use_trap_vector) < 0)
    return 1;
  if (install_poly_thread_state_key() < 0)
    return 1;
  if (poly_install_protected_runtime_signal_handlers() < 0)
    return 1;
  if (polyexec_use_auto_spill) {
    if (install_poly_auto_spill() < 0)
      return 1;
  } else if (install_poly_signal_diagnostics() < 0) {
    return 1;
  }
  if (use_trap_vector)
    install_poly_trap_vector();
  if (install_poly_watchdog() < 0)
    return 1;

  if (strcmp(argv[1], "--selftest-pagefault") == 0) {
    if (run_poly_page_fault_selftest() < 0)
      return 1;
    return 1;
  }

  if (strcmp(argv[1], "--atomic-threads") == 0) {
    if (argc != 5) {
      fprintf(stderr,
        "POLYEXEC_FAIL: --atomic-threads requires a count, iteration count, and a foreign ELF\n");
      return 2;
    }
    char *end = NULL;
    errno = 0;
    unsigned long count = strtoul(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' ||
        count == 0 || count > 64) {
      fprintf(stderr,
        "POLYEXEC_FAIL: --atomic-threads count must be between 1 and 64\n");
      return 2;
    }
    end = NULL;
    errno = 0;
    unsigned long long iterations = strtoull(argv[3], &end, 10);
    if (errno != 0 || end == argv[3] || *end != '\0' ||
        iterations == 0) {
      fprintf(stderr,
        "POLYEXEC_FAIL: --atomic-threads iterations must be nonzero\n");
      return 2;
    }

    struct poly_request request;
    if (parse_request(argv[4], &request) < 0)
      return 1;
    if (run_poly_atomic_thread_stress(&request, (size_t) count,
          (uint64_t) iterations, use_trap_vector) < 0)
      return 1;
    report_poly_monitor_packets();
    report_poly_syscall_summary();
    report_poly_auto_spill_status();
    clear_poly_trap_vector();
    clear_poly_auto_spill();
    puts("POLYEXEC_OK");
    return 0;
  }

  if (strcmp(argv[1], "--threads") == 0) {
    if (argc != 4) {
      fprintf(stderr,
        "POLYEXEC_FAIL: --threads requires a count and a foreign ELF\n");
      return 2;
    }
    char *end = NULL;
    errno = 0;
    unsigned long count = strtoul(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' ||
        count == 0 || count > 64) {
      fprintf(stderr,
        "POLYEXEC_FAIL: --threads count must be between 1 and 64\n");
      return 2;
    }

    struct poly_request request;
    if (parse_request(argv[3], &request) < 0)
      return 1;
    if (run_poly_thread_stress(&request, (size_t) count,
          use_trap_vector) < 0)
      return 1;
    report_poly_monitor_packets();
    report_poly_syscall_summary();
    report_poly_auto_spill_status();
    clear_poly_trap_vector();
    clear_poly_auto_spill();
    printf("POLYEXEC_THREADS_OK: threads=%lu path=%s\n",
      count, request.path);
    puts("POLYEXEC_OK");
    return 0;
  }

  if (strcmp(argv[1], "--process") == 0) {
    if (argc < 3) {
      fprintf(stderr, "POLYEXEC_FAIL: --process requires a foreign ELF\n");
      return 2;
    }
    struct poly_request request;
    if (parse_request(argv[2], &request) < 0)
      return 1;

    struct poly_program program;
    if (load_elf_program(request.path, request.symbol, &program) < 0)
      return 1;
    const int use_interpreter = poly_process_uses_real_interpreter(&program,
      &request, argc - 3, argv + 3);
    if (!use_interpreter && load_process_dependencies(&program) < 0) {
      free_program(&program);
      return 1;
    }

    printf("POLYEXEC_ELF: arch=%s bytes=%zu entry=%zu loads=%zu relro=%u process=1 interp=%s interp_mode=%u path=%s%s%s\n",
      program.arch_name, program.code_size, program.entry_offset,
      program.load_segment_count, program.relro_size != 0,
      program.has_interp ? program.interp_path : "-", use_interpreter,
      program.path, request.symbol[0] ? "#" : "", request.symbol);

    uint64_t result = 0;
    if (emit_and_run_process_child(&program, &request, argc - 3, argv + 3,
          &result, use_trap_vector) < 0) {
      free_program(&program);
      return 1;
    }

    printf("POLYEXEC_ROOT_PENTER: arch=%s generic=1 process=1 path=%s\n",
      program.arch_name, program.path);
    printf("POLYEXEC_RESULT: arch=%s value=%llu process=1 path=%s\n",
      program.arch_name, (unsigned long long) result, program.path);
    if (request.check_expected) {
      printf("POLYEXEC_EXPECT: arch=%s expected=%llu process=1 path=%s\n",
        program.arch_name, (unsigned long long) request.expected,
        program.path);
      if (result != request.expected) {
        fprintf(stderr, "POLYEXEC_FAIL: %s expected %llu got %llu\n",
          program.path, (unsigned long long) request.expected,
          (unsigned long long) result);
        free_program(&program);
        return 1;
      }
    }
    free_program(&program);
    report_poly_monitor_packets();
    report_poly_syscall_summary();
    report_poly_auto_spill_status();
    clear_poly_auto_spill();
    clear_poly_trap_vector();
    puts("POLYEXEC_OK");
    return request.check_expected ? 0 : (int) (result & 0xff);
  }

  for (int n = 1; n < argc; n++) {
    struct poly_request request;
    if (parse_request(argv[n], &request) < 0)
      return 1;

    struct poly_program program;
    if (load_elf_program(request.path, request.symbol, &program) < 0)
      return 1;

    printf("POLYEXEC_ELF: arch=%s bytes=%zu entry=%zu loads=%zu relro=%u path=%s%s%s\n",
      program.arch_name, program.code_size, program.entry_offset,
      program.load_segment_count, program.relro_size != 0,
      program.path, request.symbol[0] ? "#" : "", request.symbol);

    uint64_t result = 0;
    int run_status = program_exits_process(program.path) ?
      emit_and_run_exit_child(&program, &result, use_trap_vector) :
      emit_and_run(&program, &result, 1);
    if (run_status < 0) {
      free_program(&program);
      return 1;
    }

    printf("POLYEXEC_ROOT_PENTER: arch=%s generic=1 path=%s\n",
      program.arch_name, program.path);
    printf("POLYEXEC_RESULT: arch=%s value=%llu path=%s\n",
      program.arch_name, (unsigned long long) result, program.path);
    if (request.check_expected) {
      printf("POLYEXEC_EXPECT: arch=%s expected=%llu path=%s\n",
        program.arch_name, (unsigned long long) request.expected, program.path);
      if (result != request.expected) {
        fprintf(stderr, "POLYEXEC_FAIL: %s expected %llu got %llu\n",
          program.path, (unsigned long long) request.expected, (unsigned long long) result);
        free_program(&program);
        return 1;
      }
    }
    free_program(&program);
  }

  report_poly_monitor_packets();
  report_poly_syscall_summary();
  report_poly_auto_spill_status();
  clear_poly_trap_vector();
  clear_poly_auto_spill();
  puts("POLYEXEC_OK");
  return 0;
}
