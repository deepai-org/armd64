#include <errno.h>
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/vfs.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../include/polycpuid.h"

extern char **environ;

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x3a,0xfc,0x60\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x3a,0xfc,0x63\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x3a,0xfc,0x62\n"
#define POLY_OP_STATE_KEY_SET ".byte 0x0f,0x3a,0xfc,0x65\n"
#define POLY_OP_STATE_KEY_GET ".byte 0x0f,0x3a,0xfc,0x66\n"
#define POLY_OP_MONITOR_PACKET_SET ".byte 0x0f,0x3a,0xfc,0x6b\n"
#define POLY_OP_ABI_SIGNATURE_SET ".byte 0x0f,0x3a,0xfc,0x69\n"

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
  POLY_ARCH_AARCH64 = POLY_FRONTEND_AARCH64,
  POLY_ARCH_RISCV = POLY_FRONTEND_RISCV,
  POLY_X86_CONTROL_OPCODE_SIZE = 4,
  POLY_X86_PENTER_GENERIC_SIZE = 10,
  MAX_PROGRAM_BYTES = 1024 * 1024,
  MAX_LOAD_SEGMENTS = 16,
  MAX_PROCESS_DEPS = 8,
  MAX_PROCESS_DEP_DEPTH = 4,
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
static size_t process_cross_state_key_stub_count;
static int process_cross_state_key_stub_reported;

struct poly_request {
  char path[160];
  char symbol[80];
  uint64_t expected;
  int check_expected;
};

static uint32_t process_native_signature_slot = 3;
static __thread uint8_t poly_state_key_anchor;
static volatile uint64_t poly_monitor_packet[16] __attribute__((aligned(64)));
static volatile uint64_t poly_monitor_packet_count;
static volatile uint64_t poly_process_terminal_exit_code;

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

struct poly_process_exit_finalizer_context {
  struct poly_program *program;
  uint8_t *loaded_image;
  uint8_t *trampoline_code;
  size_t prefix_size;
  uint64_t return_pc;
  uint8_t *scratch;
  int active;
  int running;
  int completed;
};

static struct poly_process_exit_finalizer_context
  poly_process_exit_finalizers;

static int run_process_exit_finalizers(void);

static int run_irelative_resolver(const struct poly_program *program,
    uint8_t *loaded_image, uint8_t *trampoline_code, size_t prefix_size,
    uint64_t return_pc, uint8_t *scratch, uint64_t resolver_vaddr,
    uint64_t *resolved);
static int emit_process_cross_isa_call_stub(int caller_arch, int callee_arch,
    uint64_t target, uint64_t *stub_addr);

static inline void poly_mode_x86(void) { asm volatile(".byte 0x0f,0x3a,0xfc,0x00" ::: "memory"); }

static uint64_t poly_state_key_set(uint64_t value) {
  asm volatile(POLY_OP_STATE_KEY_SET : "+a"(value) :: "memory");
  return value;
}

static uint64_t poly_state_key_get(void) {
  uint64_t value;
  asm volatile(POLY_OP_STATE_KEY_GET : "=a"(value) :: "memory");
  return value;
}

static int install_poly_thread_state_key(void) {
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
  if (x86_controls.eax != expected_x86_controls.eax ||
      x86_controls.ebx != expected_x86_controls.ebx ||
      x86_controls.ecx != expected_x86_controls.ecx ||
      x86_controls.edx != expected_x86_controls.edx) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly x86 control manifest mismatch x86=(0x%x,0x%x,0x%x,0x%x)\n",
      x86_controls.eax, x86_controls.ebx, x86_controls.ecx,
      x86_controls.edx);
    return -1;
  }

  const struct poly_cpuid_regs signature =
    poly_read_cpuid(POLY_CPUID_BASE + 2, 7);
  const struct poly_cpuid_regs expected_signature =
    poly_cpuid_expected_escape_leaf7();
  const uint32_t native_slot = (signature.ecx >> 24) & 0xffU;
  const uint32_t native_kind = (signature.edx >> 24) & 0xffU;
  if (signature.eax != expected_signature.eax ||
      signature.ebx != expected_signature.ebx ||
      signature.ecx != expected_signature.ecx ||
      signature.edx != expected_signature.edx ||
      native_slot >= signature.ebx ||
      native_kind != POLY_ABI_SIGNATURE_KIND_NATIVE_REGS) {
    fprintf(stderr,
      "POLYEXEC_FAIL: poly native signature manifest mismatch sig=(0x%x,%u,0x%x,0x%x)\n",
      signature.eax, signature.ebx, signature.ecx, signature.edx);
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

static int poly_is_raw_foreign_mode(uint64_t mode) {
  return mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV;
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

static void write_u64_le(uint8_t *bytes, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    bytes[n] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static uint32_t relative_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_RELATIVE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_RELATIVE;
  return 0;
}

static int symbolic_64_reloc_type_for_arch(int arch, uint32_t type) {
  if (arch == POLY_ARCH_AARCH64)
    return type == R_AARCH64_ABS64 || type == R_AARCH64_GLOB_DAT ||
      type == R_AARCH64_JUMP_SLOT;
  if (arch == POLY_ARCH_RISCV)
    return type == R_RISCV_64 || type == R_RISCV_JUMP_SLOT;
  return 0;
}

static uint32_t irelative_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_IRELATIVE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_IRELATIVE;
  return UINT32_MAX;
}

static uint32_t tls_tprel_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_TLS_TPREL64;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_TLS_TPREL64;
  return UINT32_MAX;
}

static uint32_t none_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_NONE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_NONE;
  return UINT32_MAX;
}

static uint32_t copy_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_COPY;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_COPY;
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
      if (caller_arch != dep->program->arch &&
          (resolved_type == STT_FUNC || resolved_type == STT_NOTYPE ||
           resolved_type == STT_GNU_IFUNC) &&
          emit_process_cross_isa_call_stub(caller_arch, dep->program->arch,
            *symbol_value, symbol_value) < 0)
        return -1;
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
  if (program->arch != program->scope_root_program->arch &&
      (resolved_type == STT_FUNC || resolved_type == STT_NOTYPE ||
       resolved_type == STT_GNU_IFUNC) &&
      emit_process_cross_isa_call_stub(program->arch,
        program->scope_root_program->arch, *symbol_value, symbol_value) < 0)
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
  if ((program->arch == POLY_ARCH_AARCH64 ||
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
    default:
      return 0;
  }
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
    case 134: *x86_number = SYS_rt_sigaction; return 1;
    case 135: *x86_number = SYS_rt_sigprocmask; return 1;
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
    case 180: *x86_number = SYS_socket; return 1;
    case 181: *x86_number = SYS_socketpair; return 1;
    case 182: *x86_number = SYS_bind; return 1;
    case 183: *x86_number = SYS_listen; return 1;
    case 184: *x86_number = SYS_accept; return 1;
    case 185: *x86_number = SYS_connect; return 1;
    case 186: *x86_number = SYS_getsockname; return 1;
    case 187: *x86_number = SYS_getpeername; return 1;
    case 188: *x86_number = SYS_sendto; return 1;
    case 189: *x86_number = SYS_recvfrom; return 1;
    case 190: *x86_number = SYS_setsockopt; return 1;
    case 191: *x86_number = SYS_getsockopt; return 1;
    case 192: *x86_number = SYS_shutdown; return 1;
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
    case 444: *x86_number = SYS_landlock_create_ruleset; return 1;
    case 445: *x86_number = SYS_landlock_add_rule; return 1;
    case 446: *x86_number = SYS_landlock_restrict_self; return 1;
    case 448: *x86_number = SYS_process_mrelease; return 1;
    case 449: *x86_number = SYS_futex_waitv; return 1;
    case 450: *x86_number = SYS_set_mempolicy_home_node; return 1;
    default: return 0;
  }
}

static int read_poly_monitor_packet(struct poly_runtime_trap_packet *packet) {
  const uint64_t required_flags = POLY_TRAP_PACKET_REQUIRED_FLAGS;
  const uint64_t header = poly_monitor_packet[0];
  packet->reason = header & 0xffffffffULL;
  packet->mode = header >> 32;
  packet->number = poly_monitor_packet[1];
  packet->selector = poly_monitor_packet[2];
  packet->pc = poly_monitor_packet[3];
  packet->next_pc = poly_monitor_packet[4];
  packet->flags = poly_monitor_packet[5];
  packet->reserved[0] = poly_monitor_packet[6];
  packet->reserved[1] = poly_monitor_packet[7];
  for (size_t n = 0; n < 8; n++)
    packet->args[n] = poly_monitor_packet[8 + n];

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

__attribute__((noinline, used))
uint64_t poly_trap_vector_dispatch(void) {
  struct poly_runtime_trap_packet packet;

  if (read_poly_monitor_packet(&packet) < 0)
    return (uint64_t) -EIO;
  poly_monitor_packet_count++;

  if (!poly_is_raw_foreign_mode(packet.mode))
    return (uint64_t) -ENOSYS;

  if (packet.reason == POLY_TRAP_SYSCALL) {
    uint64_t structured_result = 0;
    if (poly_handle_structured_foreign_syscall(packet.number, packet.mode,
          packet.args[0], packet.args[1], packet.args[2], packet.args[3],
          packet.args[4], packet.args[5], &structured_result))
      return structured_result;

    long x86_number = -1;
    if (!poly_generic_linux_syscall_to_x86(packet.number, &x86_number))
      return (uint64_t) -ENOSYS;
    if (x86_number == SYS_exit || x86_number == SYS_exit_group) {
      poly_process_terminal_exit_code = packet.args[0];
      if (run_process_exit_finalizers() < 0)
        poly_process_terminal_exit_code = 125;
      poly_x86_exit_group_now(poly_process_terminal_exit_code);
    }
    uint64_t args[6];
    for (size_t n = 0; n < 6; n++)
      args[n] = packet.args[n];
    return (uint64_t) poly_x86_syscall6(x86_number, args[0], args[1],
      args[2], args[3], args[4], args[5]);
  }

  if (packet.reason == POLY_TRAP_BREAK) {
    return 0x4c000000ULL | (packet.mode << 8) | packet.number;
  }

  return (uint64_t) -ENOSYS;
}

__attribute__((naked, noinline, used))
static void poly_trap_vector_handler(void) {
  __asm__(
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
    "subq $56, %rsp\n"
    "call poly_trap_vector_dispatch\n"
    "addq $56, %rsp\n"
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
    POLY_OP_TRAP_RETURN
    "ud2\n");
}

static void install_poly_trap_vector(void) {
  memset((void *) poly_monitor_packet, 0, sizeof(poly_monitor_packet));
  poly_monitor_packet_count = 0;
  poly_monitor_packet_set_value((uint64_t) (uintptr_t) poly_monitor_packet);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value((uint64_t) (void *) poly_trap_vector_handler);
}

static void clear_poly_trap_vector(void) {
  poly_trap_vector_set_value(0);
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_monitor_packet_set_value(0);
}

static void report_poly_monitor_packets(void) {
  if (poly_monitor_packet_count != 0) {
    printf("POLYEXEC_MONITOR_PACKETS: count=%llu\n",
      (unsigned long long) poly_monitor_packet_count);
  }
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

static void emit_x86_penter_frontend(uint8_t *code, size_t *offset,
    uint32_t frontend) {
  code[(*offset)++] = 0x41; // mov r15d,frontend
  code[(*offset)++] = 0xbf;
  emit_u32(code, offset, frontend);
  code[(*offset)++] = 0x0f;
  code[(*offset)++] = 0x3a;
  code[(*offset)++] = 0xfc;
  code[(*offset)++] = POLY_X86_CTRL_PENTER_MODE;
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
    uint64_t initial_sp, uint64_t tls_base) {
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

static int build_process_stack(const struct poly_program *program,
    const struct poly_request *request, const uint8_t *loaded_image,
    int extra_argc, char **extra_argv, uint8_t **stack_out,
    size_t *stack_size_out, uint64_t *initial_sp_out) {
  const size_t stack_size = 128 * 1024;
  uint8_t *stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (stack == MAP_FAILED) {
    fprintf(stderr, "POLYEXEC_FAIL: process stack mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

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
  if (copy_stack_string(stack, &cursor, program->arch == POLY_ARCH_AARCH64 ?
        "aarch64" : "riscv64", &platform_ptr) < 0) {
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
    { AT_BASE, 0 },
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
    uint64_t target_pc) {
  size_t offset = 0;
  if (program->arch == POLY_ARCH_AARCH64) {
    emit_x86_penter_frontend(code, &offset, POLY_ARCH_AARCH64);
    emit_u32(code, &offset, aarch64_adr(30,
      (int64_t) return_pc - (int64_t) (uintptr_t) (code + offset)));
    emit_aarch64_movabs(code, &offset, 16, target_pc);
    emit_u32(code, &offset, 0xd61f0200U); // br x16
  }
  else {
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
  if (offset != prefix_size) {
    fprintf(stderr, "POLYEXEC_FAIL: internal trampoline size mismatch: %s\n",
      program->path);
    return -1;
  }
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
  return 0xd5032c1fU | ((slot & 0x7U) << 5);
}

static uint32_t riscv_pcall_sig_imm(uint32_t slot) {
  return 0x2000700bU | ((slot & 0x7U) << 25);
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
    if (code_size < 43)
      return -1;
    const uint64_t resolver_return_pc = (uint64_t) (uintptr_t) (code + 38);
    code[offset++] = 0x48; // movabs rax,target_pc -> AArch64 x0
    code[offset++] = 0xb8;
    emit_u64(code, &offset, target_pc);
    code[offset++] = 0x48; // movabs rdx,return_pc -> AArch64 x1
    code[offset++] = 0xba;
    emit_u64(code, &offset, resolver_return_pc);
    emit_x86_penter_frontend(code, &offset, POLY_ARCH_AARCH64);
    emit_u32(code, &offset, 0xaa0103feU); // mov x30,x1
    emit_u32(code, &offset, 0xd61f0000U); // br x0
    emit_u32(code, &offset, 0xd5032e1fU); // AArch64 polyctrl escape
    code[offset++] = 0xc3;
    return 0;
  }

  if (program->arch == POLY_ARCH_RISCV) {
    if (code_size < 43)
      return -1;
    const uint64_t resolver_return_pc = (uint64_t) (uintptr_t) (code + 38);
    code[offset++] = 0x48; // movabs rax,target_pc -> RISC-V a0
    code[offset++] = 0xb8;
    emit_u64(code, &offset, target_pc);
    code[offset++] = 0x48; // movabs rdx,return_pc -> RISC-V a1
    code[offset++] = 0xba;
    emit_u64(code, &offset, resolver_return_pc);
    emit_x86_penter_frontend(code, &offset, POLY_ARCH_RISCV);
    emit_u32(code, &offset, riscv_addi(1, 11, 0)); // mv ra,a1
    emit_u32(code, &offset, riscv_jalr(0, 10, 0)); // jr a0
    emit_u32(code, &offset, 0x0000700bU); // RISC-V polyctrl escape
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

static int emit_process_cross_isa_call_stub(int caller_arch, int callee_arch,
    uint64_t target, uint64_t *stub_addr) {
  if (caller_arch == callee_arch) {
    *stub_addr = target;
    return 0;
  }
  if (!((caller_arch == POLY_ARCH_AARCH64 && callee_arch == POLY_ARCH_RISCV) ||
        (caller_arch == POLY_ARCH_RISCV && callee_arch == POLY_ARCH_AARCH64)))
    return -1;
  if (ensure_process_cross_stub_arena() < 0 ||
      align_up_size(process_cross_stubs.offset, 8,
        &process_cross_stubs.offset) < 0)
    return -1;

  uint8_t *code = process_cross_stubs.mapping;
  const size_t start = process_cross_stubs.offset;
  const uint64_t start_addr = (uint64_t) (uintptr_t) (code + start);
  const uint64_t state_key =
    (uint64_t) (uintptr_t) &poly_state_key_anchor;

  if (caller_arch == POLY_ARCH_AARCH64) {
    if (process_cross_stubs.size - start < 128)
      return -1;
    size_t offset = start;
    const uint64_t return_addr = start_addr + 84;
    emit_u32(code, &offset, 0xd10083ffU); // sub sp, sp, #32
    emit_u32(code, &offset, 0xf94013f2U); // ldr x18, [sp, #32]
    emit_u32(code, &offset, 0xf90003f2U); // str x18, [sp]
    emit_u32(code, &offset, 0xf90007feU); // str x30, [sp, #8]
    emit_u32(code, &offset, 0xf9000be0U); // str x0, [sp, #16]
    emit_aarch64_movabs(code, &offset, 0, state_key);
    emit_u32(code, &offset, POLY_AARCH64_CTRL_STATE_KEY_SET);
    emit_u32(code, &offset, 0xf9400be0U); // ldr x0, [sp, #16]
    emit_aarch64_movabs(code, &offset, 16, target);
    emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
    emit_aarch64_movabs(code, &offset, 18, return_addr);
    emit_u32(code, &offset,
      aarch64_pcall_sig_imm(process_native_signature_slot));
    emit_u32(code, &offset, 0xf94007feU); // ldr x30, [sp, #8]
    emit_u32(code, &offset, 0x910083ffU); // add sp, sp, #32
    emit_u32(code, &offset, 0xd65f03c0U); // ret
    process_cross_stubs.offset = offset;
    process_cross_state_key_stub_count++;
    if (!process_cross_state_key_stub_reported) {
      printf("POLYEXEC_CROSS_STUB_STATE_KEY: explicit=1\n");
      fflush(NULL);
      process_cross_state_key_stub_reported = 1;
    }
    *stub_addr = start_addr;
    return 0;
  }

  if (process_cross_stubs.size - start < 128)
    return -1;
  size_t offset = start;
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0xfe010113U); // addi sp,sp,-32
  emit_u32(code, &offset, riscv_ld(29, 2, 32)); // ld t4,32(sp)
  emit_u32(code, &offset, riscv_sd(29, 2, 0)); // sd t4,0(sp)
  emit_u32(code, &offset, riscv_sd(1, 2, 8)); // sd ra,8(sp)
  emit_u32(code, &offset, riscv_sd(10, 2, 16)); // sd a0,16(sp)
  const size_t auipc_state_key_pc = offset;
  emit_u32(code, &offset, 0x00000517U); // auipc a0,0
  const size_t ld_state_key_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, POLY_RISCV_CTRL_STATE_KEY_SET);
  emit_u32(code, &offset, riscv_ld(10, 2, 16)); // ld a0,16(sp)
  emit_u32(code, &offset, riscv_pcall_sig_imm(process_native_signature_slot));
  const size_t return_pc = offset;
  emit_u32(code, &offset, 0x00813083U); // ld ra,8(sp)
  emit_u32(code, &offset, 0x02010113U); // addi sp,sp,32
  emit_u32(code, &offset, 0x00008067U); // ret
  if (align_up_size(offset, 8, &offset) < 0 ||
      process_cross_stubs.size - offset < 24)
    return -1;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, target);
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + return_pc));
  const size_t state_key_data_offset = offset;
  emit_u64(code, &offset, state_key);
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int16_t) ((int64_t) target_data_offset - (int64_t) auipc_target_pc)));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int16_t) ((int64_t) return_data_offset - (int64_t) auipc_return_pc)));
  store_u32(code, ld_state_key_offset, riscv_ld(10, 10,
    (int16_t) ((int64_t) state_key_data_offset -
      (int64_t) auipc_state_key_pc)));
  process_cross_stubs.offset = offset;
  process_cross_state_key_stub_count++;
  if (!process_cross_state_key_stub_reported) {
    printf("POLYEXEC_CROSS_STUB_STATE_KEY: explicit=1\n");
    fflush(NULL);
    process_cross_state_key_stub_reported = 1;
  }
  *stub_addr = start_addr;
  return 0;
}

static int emit_process_tlsdesc_helper(const struct poly_program *program,
    uint8_t *mapping, size_t *offset, uint64_t *helper_pc) {
  if (*offset > SIZE_MAX - 16)
    return -1;
  if (program->arch != POLY_ARCH_AARCH64) {
    *helper_pc = 0;
    return 0;
  }
  *helper_pc = (uint64_t) (uintptr_t) (mapping + *offset);
  emit_u32(mapping, offset, 0xf9400400U); // ldr x0, [x0, #8]
  emit_u32(mapping, offset, 0xd65f03c0U); // ret
  return 0;
}

static int emit_process_tls_get_addr_helper(const struct poly_program *program,
    uint8_t *mapping, size_t *offset, uint64_t *helper_pc) {
  if (*offset > SIZE_MAX - 16)
    return -1;
  if (program->arch != POLY_ARCH_AARCH64 &&
      program->arch != POLY_ARCH_RISCV) {
    *helper_pc = 0;
    return 0;
  }
  *helper_pc = (uint64_t) (uintptr_t) (mapping + *offset);
  if (program->arch == POLY_ARCH_AARCH64) {
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

  const size_t expected_prefix_size =
    program->arch == POLY_ARCH_AARCH64 ? 18 :
    program->arch == POLY_ARCH_RISCV ? 30 : 0;
  if (expected_prefix_size != 0 && prefix_size == expected_prefix_size) {
    if (emit_poly_trampoline(program, trampoline_code, prefix_size,
          return_pc, resolver_pc) < 0)
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
    program->arch == POLY_ARCH_RISCV ? R_RISCV_TLS_DTPMOD64 : UINT32_MAX;
  const uint32_t tls_dtprel_type = program->arch == POLY_ARCH_AARCH64 ?
    R_AARCH64_TLS_DTPREL64 :
    program->arch == POLY_ARCH_RISCV ? R_RISCV_TLS_DTPREL64 : UINT32_MAX;
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
            !symbolic_64_reloc_type_for_arch(program->arch, reloc_type))
          return -1;
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
            !symbolic_64_reloc_type_for_arch(program->arch, reloc_type))
          return -1;
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
        target_pc) < 0) {
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
  if (!ctx->active || ctx->completed || ctx->running)
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
    if (phdr->p_type == PT_GNU_RELRO) {
      program->relro_vaddr = phdr->p_vaddr;
      program->relro_size = phdr->p_memsz;
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

  if (!found_load || limit_vaddr <= base_vaddr ||
      limit_vaddr - base_vaddr > MAX_PROGRAM_BYTES) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported ELF load image: %s\n", path);
    free(data);
    return -1;
  }

  uint64_t image_size = limit_vaddr - base_vaddr;
  const uint64_t instruction_align = program->arch == POLY_ARCH_AARCH64 ? 4 : 2;
  if ((image_size % instruction_align) != 0)
    image_size += instruction_align - (image_size % instruction_align);
  if (image_size == 0 || image_size > MAX_PROGRAM_BYTES) {
    fprintf(stderr, "POLYEXEC_FAIL: ELF loaded image is too large: %s\n", path);
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
    uint64_t tlsdesc_helper_pc, uint64_t tls_get_addr_helper_pc,
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
          return_pc, tlsdesc_helper_pc, tls_get_addr_helper_pc, scratch) < 0)
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

    if (apply_relative_relocations(dep->program, dep->loaded_image,
          trampoline_code, prefix_size, return_pc, tlsdesc_helper_pc,
          tls_get_addr_helper_pc, scratch) < 0) {
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

static int emit_and_run(const struct poly_program *program, uint64_t *result) {
  const size_t return_setup_size = program->arch == POLY_ARCH_AARCH64 ? 4 : 8;
  const size_t branch_size = program->arch == POLY_ARCH_AARCH64 ? 20 : 12;
  const size_t raw_switch_size = POLY_X86_PENTER_GENERIC_SIZE;
  const size_t prefix_size = raw_switch_size + return_setup_size + branch_size;
  const size_t load_base_offset = 4096;
  const size_t branch_offset = load_base_offset - branch_size;
  const size_t code_offset = branch_offset - raw_switch_size - return_setup_size;
  const size_t escape_return_size = 5;
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
    0xd5032e1fU : 0x0000700bU;
  size_t offset = load_base_offset;
  emit_bytes(mapping, &offset, program->code_bytes, program->code_size);
  emit_u32(mapping, &offset, escape);
  mapping[offset++] = 0xc3;
  offset = return_page_offset;
  emit_u32(mapping, &offset, escape);
  mapping[offset++] = 0xc3;
  if (align_up_size(offset, 4, &offset) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }
  uint64_t tlsdesc_helper_pc = 0;
  if (emit_process_tlsdesc_helper(program, mapping, &offset,
        &tlsdesc_helper_pc) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }
  uint64_t tls_get_addr_helper_pc = 0;
  if (emit_process_tls_get_addr_helper(program, mapping, &offset,
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
  if (emit_poly_trampoline(program, code, prefix_size, return_pc, entry_pc) < 0) {
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

  if (prepare_program_scratch(program->path, (char *) scratch, scratch_size) < 0) {
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
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

static int emit_and_run_process(struct poly_program *program,
    const struct poly_request *request, int extra_argc, char **extra_argv,
    uint64_t *result) {
  const size_t return_setup_size = program->arch == POLY_ARCH_AARCH64 ? 4 : 8;
  const size_t branch_size = program->arch == POLY_ARCH_AARCH64 ? 20 : 12;
  const size_t raw_switch_size = POLY_X86_PENTER_GENERIC_SIZE;
  const size_t prefix_size = raw_switch_size + return_setup_size + branch_size;
  const size_t load_base_offset = 4096;
  const size_t branch_offset = load_base_offset - branch_size;
  const size_t code_offset = branch_offset - raw_switch_size - return_setup_size;
  const size_t escape_return_size = 5;
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
  const uint64_t lifecycle_return_pc = (uint64_t) (uintptr_t)
    (mapping + lifecycle_return_page_offset);
  const uint64_t process_return_pc = (uint64_t) (uintptr_t)
    (mapping + process_return_page_offset);
  const uint64_t entry_pc = (uint64_t) (uintptr_t)
    (loaded_image + program->entry_offset);
  const uint32_t escape = program->arch == POLY_ARCH_AARCH64 ?
    0xd5032e1fU : 0x0000700bU;
  size_t offset = image_offset;
  emit_bytes(mapping, &offset, program->code_bytes, program->code_size);
  if (!fixed_main_image) {
    emit_u32(mapping, &offset, escape);
    mapping[offset++] = 0xc3;
  }
  offset = lifecycle_return_page_offset;
  emit_u32(mapping, &offset, escape);
  mapping[offset++] = 0xc3;
  offset = process_return_page_offset;
  emit_u32(mapping, &offset, escape);
  emit_x86_exit_group_from_eax(mapping, &offset);
  if (align_up_size(offset, 4, &offset) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }
  uint64_t tlsdesc_helper_pc = 0;
  if (emit_process_tlsdesc_helper(program, mapping, &offset,
        &tlsdesc_helper_pc) < 0) {
    munmap(mapping, mapping_size);
    return -1;
  }
  uint64_t tls_get_addr_helper_pc = 0;
  if (emit_process_tls_get_addr_helper(program, mapping, &offset,
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
  set_process_dependency_root_scope(program, program, loaded_image);
  size_t process_tls_size = 0;
  if (reserve_process_tls_tree(program, &process_tls_size) < 0) {
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (map_process_dependencies(program, code, prefix_size, lifecycle_return_pc,
        tlsdesc_helper_pc, tls_get_addr_helper_pc, scratch) < 0) {
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (apply_relative_relocations(program, loaded_image, code,
        prefix_size, lifecycle_return_pc, tlsdesc_helper_pc,
        tls_get_addr_helper_pc, scratch) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: unsupported dynamic relocations: %s\n",
      program->path);
    unmap_process_dependencies(program);
    munmap(scratch, scratch_size);
    munmap(mapping, mapping_size);
    return -1;
  }
  if (emit_poly_trampoline(program, code, prefix_size, lifecycle_return_pc,
        entry_pc) < 0) {
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
        entry_pc) < 0) {
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
  if (build_process_stack(program, request, loaded_image,
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
    };

  (void) result;
  run_poly_process_entry(code, initial_sp,
    (uint64_t) (uintptr_t) process_tls);
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
    if (use_trap_vector)
      install_poly_trap_vector();
    if (emit_and_run(program, &child_result) < 0)
      _exit(125);
    _exit((int) (child_result & 0xff));
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: waitpid failed for %s: %s\n",
      program->path, strerror(errno));
    return -1;
  }

  if (WIFEXITED(status)) {
    *result = (uint64_t) WEXITSTATUS(status);
    return 0;
  }
  if (WIFSIGNALED(status)) {
    *result = (uint64_t) (128 + WTERMSIG(status));
    return 0;
  }

  fprintf(stderr, "POLYEXEC_FAIL: unexpected child status for %s: 0x%x\n",
    program->path, status);
  return -1;
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
    if (use_trap_vector)
      install_poly_trap_vector();
    if (emit_and_run_process(program, request, extra_argc, extra_argv,
          &child_result) < 0)
      _exit(125);
    _exit((int) (child_result & 0xff));
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fprintf(stderr, "POLYEXEC_FAIL: waitpid failed for %s: %s\n",
      program->path, strerror(errno));
    return -1;
  }

  if (WIFEXITED(status)) {
    *result = (uint64_t) WEXITSTATUS(status);
    return 0;
  }
  if (WIFSIGNALED(status)) {
    *result = (uint64_t) (128 + WTERMSIG(status));
    return 0;
  }

  fprintf(stderr, "POLYEXEC_FAIL: unexpected child status for %s: 0x%x\n",
    program->path, status);
  return -1;
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

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s foreign.elf[=expected]... | --process foreign.elf[=expected] [arg...]\n",
      argv[0]);
    return 2;
  }

  puts("POLYEXEC: start");
  if (prepare_syscall_fixture_file() < 0)
    return 1;
  const char *trap_vector_env = getenv("POLYEXEC_TRAP_VECTOR");
  const int use_trap_vector =
    trap_vector_env == NULL || strcmp(trap_vector_env, "0") != 0;
  if (read_poly_base_contract(use_trap_vector) < 0)
    return 1;
  if (install_poly_thread_state_key() < 0)
    return 1;
  if (use_trap_vector)
    install_poly_trap_vector();

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
    if (load_process_dependencies(&program) < 0) {
      free_program(&program);
      return 1;
    }

    printf("POLYEXEC_ELF: arch=%s bytes=%zu entry=%zu loads=%zu relro=%u process=1 path=%s%s%s\n",
      program.arch_name, program.code_size, program.entry_offset,
      program.load_segment_count, program.relro_size != 0,
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
    clear_poly_trap_vector();
    report_poly_monitor_packets();
    puts("POLYEXEC_OK");
    return 0;
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
      emit_and_run(&program, &result);
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

  clear_poly_trap_vector();
  report_poly_monitor_packets();
  puts("POLYEXEC_OK");
  return 0;
}
