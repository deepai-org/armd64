#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "../include/polycpuid.h"

#define POLY_OP_STATE_EXPORT ".byte 0x0f,0x24,0x67\n"

enum {
  POLYSIGNAL_LOOP_COUNT = 200000,
  POLYSIGNAL_SNAPSHOT_NONE = 0,
  POLYSIGNAL_SNAPSHOT_AARCH64_X20 = 1,
  POLYSIGNAL_SNAPSHOT_RISCV_X20 = 2,
  POLYSIGNAL_SNAPSHOT_AARCH64_D20 = 3,
  POLYSIGNAL_SNAPSHOT_RISCV_F20 = 4
};

static volatile sig_atomic_t signal_count;
static volatile sig_atomic_t signal_transition_count;
static volatile sig_atomic_t signal_transition_bad;
static volatile sig_atomic_t signal_snapshot_bad;
static volatile sig_atomic_t signal_expected_snapshot;
static volatile sig_atomic_t signal_expected_mode;
static volatile uint64_t signal_expected_snapshot_value;
static struct poly_xsave_state signal_snapshot __attribute__((aligned(64)));

static uint64_t double_to_bits(double value) {
  union {
    double d;
    uint64_t u;
  } bits;
  bits.d = value;
  return bits.u;
}

static inline uint64_t read_xmm0_u64(void) {
  uint64_t value;
  asm volatile("movq %%xmm0, %0" : "=r"(value));
  return value;
}

static inline void write_xmm0_u64(uint64_t value) {
  asm volatile("movq %0, %%xmm0" :: "r"(value) : "xmm0", "memory");
}

static inline void write_xmm1_u64(uint64_t value) {
  asm volatile("movq %0, %%xmm1" :: "r"(value) : "xmm1", "memory");
}

static inline void poly_state_export(struct poly_xsave_state *state) {
  asm volatile(POLY_OP_STATE_EXPORT :: "a"(state) : "memory");
}

static void emit_u32(uint8_t *code, size_t *offset, uint32_t value) {
  code[(*offset)++] = (uint8_t) (value & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 8) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 16) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 24) & 0xff);
}

static void emit_u64(uint8_t *code, size_t *offset, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    code[(*offset)++] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static void emit_bytes(uint8_t *code, size_t *offset, const uint8_t *bytes,
    size_t count) {
  for (size_t n = 0; n < count; n++)
    code[(*offset)++] = bytes[n];
}

static void store_u32(uint8_t *code, size_t offset, uint32_t value) {
  code[offset] = (uint8_t) (value & 0xff);
  code[offset + 1] = (uint8_t) ((value >> 8) & 0xff);
  code[offset + 2] = (uint8_t) ((value >> 16) & 0xff);
  code[offset + 3] = (uint8_t) ((value >> 24) & 0xff);
}

static void emit_aarch64_movabs(uint8_t *code, size_t *offset, unsigned rd,
    uint64_t value) {
  emit_u32(code, offset, 0xd2800000U | (((uint32_t) value & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2a00000U |
    ((((uint32_t) (value >> 16)) & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2c00000U |
    ((((uint32_t) (value >> 32)) & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2e00000U |
    ((((uint32_t) (value >> 48)) & 0xffffU) << 5) | rd);
}

static uint32_t riscv_ld(uint32_t rd, uint32_t rs1, int32_t imm) {
  uint32_t uimm = (uint32_t) imm & 0xfffU;
  return (uimm << 20) | ((rs1 & 0x1fU) << 15) | (3U << 12) |
    ((rd & 0x1fU) << 7) | 0x03U;
}

static int signal_snapshot_matches(void) {
  switch (signal_expected_snapshot) {
  case POLYSIGNAL_SNAPSHOT_NONE:
    return 1;
  case POLYSIGNAL_SNAPSHOT_AARCH64_X20:
    return signal_snapshot.aarch64_gpr[20] == signal_expected_snapshot_value;
  case POLYSIGNAL_SNAPSHOT_RISCV_X20:
    return signal_snapshot.riscv_gpr[20] == signal_expected_snapshot_value;
  case POLYSIGNAL_SNAPSHOT_AARCH64_D20:
    return signal_snapshot.aarch64_fp[20].lo == signal_expected_snapshot_value;
  case POLYSIGNAL_SNAPSHOT_RISCV_F20:
    return signal_snapshot.riscv_fp[20].lo == signal_expected_snapshot_value;
  default:
    return 0;
  }
}

static void handle_alarm(int signo) {
  (void) signo;
  signal_count++;
  poly_state_export(&signal_snapshot);

  uint16_t flags = signal_snapshot.transition.active.flags;
  uint32_t target_mode = signal_snapshot.transition.active.target_mode;
  if ((flags & POLY_TRANSITION_FLAG_INTERRUPTED_RAW) != 0 &&
      target_mode == (uint32_t) signal_expected_mode &&
      signal_snapshot_matches())
    signal_transition_count++;
  else {
    if ((flags & POLY_TRANSITION_FLAG_INTERRUPTED_RAW) != 0 &&
        target_mode == (uint32_t) signal_expected_mode)
      signal_snapshot_bad++;
    signal_transition_bad++;
  }
}

static int arm_alarm(void) {
  struct itimerval timer;
  memset(&timer, 0, sizeof(timer));
  timer.it_value.tv_usec = 1000;

  if (setitimer(ITIMER_REAL, &timer, 0) != 0) {
    fprintf(stderr, "POLYSIGNAL_FAIL: setitimer: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

static uint64_t pcall_aarch64_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x10\n"
    "1:\n"
    ".long 0xf1000421\n" // subs x1,x1,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x91000400\n" // add x0,x0,#1
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result), "+D"(seed), "+S"(loops)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_aarch64_hidden_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x10\n"
    "1:\n"
    ".long 0x91000014\n" // add x20,x0,#0
    ".long 0xf1000421\n" // subs x1,x1,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x91001e80\n" // add x0,x20,#7
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "=a"(result), "+D"(seed), "+S"(loops)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_riscv_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x11\n"
    "1:\n"
    ".long 0xfff58593\n" // addi a1,a1,-1
    ".long 0xfe059ee3\n" // bnez a1,-4
    ".long 0x00150513\n" // addi a0,a0,1
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(result), "+D"(seed), "+S"(loops)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_riscv_hidden_signal(uint64_t seed, uint64_t loops) {
  uint64_t result;
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x11\n"
    "1:\n"
    ".long 0x00050a13\n" // addi s4,a0,0
    ".long 0xfff58593\n" // addi a1,a1,-1
    ".long 0xfe059ee3\n" // bnez a1,-4
    ".long 0x007a0513\n" // addi a0,s4,7
    ".long 0x00008067\n" // ret
    "2:\n"
    : "=a"(result), "+D"(seed), "+S"(loops)
    :
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return result;
}

static uint64_t pcall_aarch64_hidden_fp_signal(uint64_t left_bits,
    uint64_t right_bits, uint64_t loops) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x10\n"
    "1:\n"
    ".long 0x1e604014\n" // fmov d20,d0
    ".long 0xf1000421\n" // subs x1,x1,#1
    ".long 0x54ffffe1\n" // b.ne -4
    ".long 0x1e612a80\n" // fadd d0,d20,d1
    ".long 0xd65f03c0\n" // ret x30
    "2:\n"
    : "+S"(loops)
    :
    : "rax", "rcx", "rdx", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_riscv_hidden_fp_signal(uint64_t left_bits,
    uint64_t right_bits, uint64_t loops) {
  write_xmm0_u64(left_bits);
  write_xmm1_u64(right_bits);
  asm volatile(
    "leaq 1f(%%rip), %%r10\n"
    "leaq 2f(%%rip), %%r11\n"
    ".byte 0x0f,0x24,0x11\n"
    "1:\n"
    ".long 0x22a50a53\n" // fsgnj.d f20,fa0,fa0
    ".long 0xfff58593\n" // addi a1,a1,-1
    ".long 0xfe059ee3\n" // bnez a1,-4
    ".long 0x02ba7553\n" // fadd.d fa0,f20,fa1
    ".long 0x00008067\n" // ret
    "2:\n"
    : "+S"(loops)
    :
    : "rax", "rcx", "rdx", "rdi", "r8", "r9", "r10", "r11",
      "xmm0", "xmm1", "memory");
  return read_xmm0_u64();
}

static uint64_t pcall_aarch64_to_riscv_hidden_signal(uint64_t seed,
    uint64_t loops) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYSIGNAL_FAIL: aarch64-to-riscv mmap: %s\n",
      strerror(errno));
    return UINT64_MAX;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = {
    0x0f, 0x24, 0x01
  };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_return_offset = offset + 16 + 16 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_aarch64_movabs(code, &offset, 0, seed);
  emit_aarch64_movabs(code, &offset, 1, loops);
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0x91000400U); // add x0,x0,#1
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00050a13U); // addi s4,a0,0
  emit_u32(code, &offset, 0xfff58593U); // addi a1,a1,-1
  emit_u32(code, &offset, 0xfe059ee3U); // bnez a1,-4
  emit_u32(code, &offset, 0x009a0513U); // addi a0,s4,9
  emit_u32(code, &offset, 0x00008067U); // ret

  uint64_t (*entry)(uint64_t, uint64_t) =
    (uint64_t (*)(uint64_t, uint64_t)) code;
  uint64_t result = entry(seed, loops);
  munmap(code, code_size);
  return result;
}

static uint64_t pcall_riscv_to_aarch64_hidden_signal(uint64_t seed,
    uint64_t loops) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYSIGNAL_FAIL: riscv-to-aarch64 mmap: %s\n",
      strerror(errno));
    return UINT64_MAX;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = {
    0x0f, 0x24, 0x02
  };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_seed_pc = offset;
  emit_u32(code, &offset, 0x00000517U); // auipc x10,0
  const size_t ld_seed_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_loops_pc = offset;
  emit_u32(code, &offset, 0x00000597U); // auipc x11,0
  const size_t ld_loops_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400000bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00150513U); // addi a0,a0,1
  emit_u32(code, &offset, 0x0000000bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91000014U); // add x20,x0,#0
  emit_u32(code, &offset, 0xf1000421U); // subs x1,x1,#1
  emit_u32(code, &offset, 0x54ffffe1U); // b.ne -4
  emit_u32(code, &offset, 0x91002280U); // add x0,x20,#8
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));
  const size_t seed_data_offset = offset;
  emit_u64(code, &offset, seed);
  const size_t loops_data_offset = offset;
  emit_u64(code, &offset, loops);

  store_u32(code, ld_seed_offset, riscv_ld(10, 10,
    (int32_t) seed_data_offset - (int32_t) auipc_seed_pc));
  store_u32(code, ld_loops_offset, riscv_ld(11, 11,
    (int32_t) loops_data_offset - (int32_t) auipc_loops_pc));
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  uint64_t (*entry)(uint64_t, uint64_t) =
    (uint64_t (*)(uint64_t, uint64_t)) code;
  uint64_t result = entry(seed, loops);
  munmap(code, code_size);
  return result;
}

static int check_arch(const char *name, uint64_t seed, uint64_t expected_delta,
  uint64_t (*pcall)(uint64_t, uint64_t))
{
  sig_atomic_t before = signal_count;
  sig_atomic_t transition_before = signal_transition_count;
  sig_atomic_t bad_before = signal_transition_bad;
  sig_atomic_t snapshot_bad_before = signal_snapshot_bad;

  if (arm_alarm() != 0)
    return -1;

  uint64_t result = pcall(seed, POLYSIGNAL_LOOP_COUNT);
  if (result != seed + expected_delta) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s got=%llu expected=%llu\n",
      name, (unsigned long long) result,
      (unsigned long long) (seed + expected_delta));
    return -1;
  }

  if (signal_count == before) {
    fprintf(stderr, "POLYSIGNAL_FAIL: arch=%s signal not delivered\n", name);
    return -1;
  }
  if (signal_transition_count == transition_before ||
      signal_transition_bad != bad_before) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s interrupted raw transition missing count=%d bad=%d snapshot_bad=%d\n",
      name, (int) (signal_transition_count - transition_before),
      (int) (signal_transition_bad - bad_before),
      (int) (signal_snapshot_bad - snapshot_bad_before));
    return -1;
  }

  return 0;
}

static int check_arch_fp(const char *name, uint64_t seed,
  uint64_t (*pcall)(uint64_t, uint64_t, uint64_t))
{
  sig_atomic_t before = signal_count;
  sig_atomic_t transition_before = signal_transition_count;
  sig_atomic_t bad_before = signal_transition_bad;
  sig_atomic_t snapshot_bad_before = signal_snapshot_bad;
  uint64_t seven_bits = double_to_bits(7.0);

  if (arm_alarm() != 0)
    return -1;

  uint64_t result = pcall(double_to_bits((double) seed), seven_bits,
    POLYSIGNAL_LOOP_COUNT);
  uint64_t expected = double_to_bits((double) seed + 7.0);
  if (result != expected) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s got=0x%llx expected=0x%llx\n",
      name, (unsigned long long) result, (unsigned long long) expected);
    return -1;
  }

  if (signal_count == before) {
    fprintf(stderr, "POLYSIGNAL_FAIL: arch=%s signal not delivered\n", name);
    return -1;
  }
  if (signal_transition_count == transition_before ||
      signal_transition_bad != bad_before) {
    fprintf(stderr,
      "POLYSIGNAL_FAIL: arch=%s interrupted raw transition missing count=%d bad=%d snapshot_bad=%d\n",
      name, (int) (signal_transition_count - transition_before),
      (int) (signal_transition_bad - bad_before),
      (int) (signal_snapshot_bad - snapshot_bad_before));
    return -1;
  }

  return 0;
}

int main(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_alarm;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGALRM, &sa, 0) != 0) {
    fprintf(stderr, "POLYSIGNAL_FAIL: sigaction: %s\n", strerror(errno));
    return 1;
  }

  printf("POLYSIGNAL_START\n");
  signal_expected_mode = POLY_MODE_RAW_AARCH64;
  signal_expected_snapshot = POLYSIGNAL_SNAPSHOT_NONE;
  if (check_arch("aarch64", 0x51000000ULL, 1, pcall_aarch64_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_RISCV;
  signal_expected_snapshot = POLYSIGNAL_SNAPSHOT_NONE;
  if (check_arch("riscv", 0x52000000ULL, 1, pcall_riscv_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_AARCH64;
  signal_expected_snapshot = POLYSIGNAL_SNAPSHOT_AARCH64_X20;
  signal_expected_snapshot_value = 0x53000000ULL;
  if (check_arch("aarch64-hidden", 0x53000000ULL, 7,
      pcall_aarch64_hidden_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_RISCV;
  signal_expected_snapshot = POLYSIGNAL_SNAPSHOT_RISCV_X20;
  signal_expected_snapshot_value = 0x54000000ULL;
  if (check_arch("riscv-hidden", 0x54000000ULL, 7,
      pcall_riscv_hidden_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_AARCH64;
  signal_expected_snapshot = POLYSIGNAL_SNAPSHOT_AARCH64_D20;
  signal_expected_snapshot_value = double_to_bits((double) 0x55000000ULL);
  if (check_arch_fp("aarch64-hidden-fp", 0x55000000ULL,
      pcall_aarch64_hidden_fp_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_RISCV;
  signal_expected_snapshot = POLYSIGNAL_SNAPSHOT_RISCV_F20;
  signal_expected_snapshot_value = double_to_bits((double) 0x56000000ULL);
  if (check_arch_fp("riscv-hidden-fp", 0x56000000ULL,
      pcall_riscv_hidden_fp_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_RISCV;
  signal_expected_snapshot = POLYSIGNAL_SNAPSHOT_RISCV_X20;
  signal_expected_snapshot_value = 0x57000000ULL;
  if (check_arch("aarch64-to-riscv-hidden", 0x57000000ULL, 10,
      pcall_aarch64_to_riscv_hidden_signal) != 0)
    return 1;
  signal_expected_mode = POLY_MODE_RAW_AARCH64;
  signal_expected_snapshot = POLYSIGNAL_SNAPSHOT_AARCH64_X20;
  signal_expected_snapshot_value = 0x58000000ULL;
  if (check_arch("riscv-to-aarch64-hidden", 0x58000000ULL, 9,
      pcall_riscv_to_aarch64_hidden_signal) != 0)
    return 1;

  printf("POLYSIGNAL_OK\n");
  return 0;
}
