#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define POLY_OP_TRAP_VECTOR_SET ".byte 0x0f,0x3a,0xfc,0x60\n"
#define POLY_OP_TRAP_VECTOR_MODE_SET ".byte 0x0f,0x3a,0xfc,0x63\n"
#define POLY_OP_TRAP_RETURN ".byte 0x0f,0x3a,0xfc,0x62\n"

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
  POLY_MODE_X86 = 0,
  POLY_MODE_RAW_AARCH64 = 3,
  POLY_MODE_RAW_RISCV = 4,
  POLY_TRAP_SYSCALL = 1,
  POLY_TRAP_BREAK = 2,
  POLY_TRAP_IMPORT = 3,
  LOOP_ITERS = 200,
  POLYBENCH_MIXED_MAX_SWITCH_DELTA = 4,
  POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA = 5,
  POLYBENCH_NESTED_CROSS_CALL_MAX_SWITCH_DELTA = 7,
  POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA = 5,
  POLYBENCH_DIRECT_X86_LIBCALL_MAX_SWITCH_DELTA = 7,
  POLYBENCH_DIRECT_X86_MEMOPS_MAX_SWITCH_DELTA = 11
};

typedef uint32_t polybench_vec128_u32 __attribute__((vector_size(16)));

union polybench_vec128_u32_bits {
  polybench_vec128_u32 v;
  uint32_t u[4];
};

static inline void poly_mode_x86(void) { asm volatile(".byte 0x0f,0x3a,0xfc,0x00" ::: "memory"); }
static inline void poly_switch_count_status(void) { asm volatile(".byte 0x0f,0x3a,0xfc,0x40" ::: "memory"); }
static inline void poly_foreign_insn_count_status(void) { asm volatile(".byte 0x0f,0x3a,0xfc,0x42" ::: "memory"); }

static inline void poly_trap_vector_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_SET :: "a"(value) : "memory");
}

static inline void poly_trap_vector_mode_set_value(uint64_t value) {
  asm volatile(POLY_OP_TRAP_VECTOR_MODE_SET :: "a"(value) : "memory");
}

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
}

static void emit_u16(uint8_t *code, size_t *offset, uint16_t value) {
  code[(*offset)++] = (uint8_t) (value & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 8) & 0xff);
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

static void store_u64(uint8_t *code, size_t offset, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    code[offset + n] = (uint8_t) (value >> (n * 8));
}

static void emit_u64(uint8_t *code, size_t *offset, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    code[(*offset)++] = (uint8_t) (value >> (n * 8));
}

static void emit_bytes(uint8_t *code, size_t *offset, const uint8_t *bytes, size_t size) {
  memcpy(code + *offset, bytes, size);
  *offset += size;
}

static void emit_aarch64_movabs(uint8_t *code, size_t *offset, uint32_t rd,
    uint64_t value) {
  emit_u32(code, offset, 0xd2800000U | (((uint32_t) value & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2a00000U | ((((uint32_t) (value >> 16)) & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2c00000U | ((((uint32_t) (value >> 32)) & 0xffffU) << 5) | rd);
  emit_u32(code, offset, 0xf2e00000U | ((((uint32_t) (value >> 48)) & 0xffffU) << 5) | rd);
}

static void emit_aarch64_direct_x86_pcall(uint8_t *code, size_t *offset,
    uint64_t target) {
  emit_aarch64_movabs(code, offset, 16, target);
  emit_u32(code, offset, 0xd2800011U); // movz x17,#0 (x86 frontend)
  const uint64_t return_pc =
    (uint64_t) (uintptr_t) (code + *offset + 16 + 4);
  emit_aarch64_movabs(code, offset, 18, return_pc);
  emit_u32(code, offset, 0xd5032c3fU); // PCALL_SIG_IMM slot 1 (x86 SysV regs)
}

static size_t emit_x86_movabs_r10(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xba;
  size_t imm_offset = *offset;
  emit_u64(code, offset, value);
  return imm_offset;
}

static size_t emit_x86_movabs_r11(uint8_t *code, size_t *offset,
    uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbb;
  size_t imm_offset = *offset;
  emit_u64(code, offset, value);
  return imm_offset;
}

static uint32_t aarch64_mov_reg(uint32_t rd, uint32_t rn) {
  return 0xaa0003e0U | ((rn & 0x1fU) << 16) | (rd & 0x1fU);
}

static uint32_t aarch64_fadd_d(uint32_t rd, uint32_t rn, uint32_t rm) {
  return 0x1e602800U | ((rm & 0x1fU) << 16) |
    ((rn & 0x1fU) << 5) | (rd & 0x1fU);
}

static uint32_t aarch64_ldr_x_sp(uint32_t rt, uint32_t offset) {
  return 0xf94003e0U | (((offset / 8U) & 0xfffU) << 10) | (rt & 0x1fU);
}

static uint32_t aarch64_str_x_sp(uint32_t rt, uint32_t offset) {
  return 0xf90003e0U | (((offset / 8U) & 0xfffU) << 10) | (rt & 0x1fU);
}

static uint32_t aarch64_ldr_d_sp(uint32_t rt, uint32_t offset) {
  return 0xfd4003e0U | (((offset / 8U) & 0xfffU) << 10) | (rt & 0x1fU);
}

static uint64_t fp64_to_bits(double value) {
  union {
    double d;
    uint64_t u;
  } fp;
  fp.d = value;
  return fp.u;
}

static int poly_is_raw_foreign_mode(uint64_t mode) {
  return mode == POLY_MODE_RAW_AARCH64 || mode == POLY_MODE_RAW_RISCV;
}

__attribute__((noinline, used))
uint64_t polybench_trap_vector_dispatch(uint64_t reason, uint64_t mode,
    uint64_t number, uint64_t pc, uint64_t selector, uint64_t arg0,
    uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4,
    uint64_t arg5) {
  (void) pc;
  (void) selector;
  (void) arg1;
  (void) arg2;
  (void) arg3;
  (void) arg4;
  (void) arg5;

  if (!poly_is_raw_foreign_mode(mode))
    return (uint64_t) -38;
  if (reason == POLY_TRAP_SYSCALL && number == 172)
    return 4242;
  if (reason == POLY_TRAP_BREAK)
    return 0x4c000000ULL | (mode << 8) | number;
  return (uint64_t) -38;
}

__attribute__((naked, noinline, used))
static void polybench_trap_vector_handler(void) {
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
    "pushq %r12\n"
    "pushq %r11\n"
    "pushq %r10\n"
    "pushq %r9\n"
    "pushq %r8\n"
    "movq %rdi, %r9\n"
    "movq %rsi, %r8\n"
    "movq %rcx, %r10\n"
    "movq %rdx, %rcx\n"
    "movq %r10, %rdx\n"
    "movq %rbx, %rsi\n"
    "movq %rax, %rdi\n"
    "call polybench_trap_vector_dispatch\n"
    "addq $40, %rsp\n"
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

static void install_polybench_trap_vector(void) {
  poly_trap_vector_mode_set_value(POLY_MODE_X86);
  poly_trap_vector_set_value((uint64_t) (void *) polybench_trap_vector_handler);
}

__attribute__((noinline, used))
static uint64_t polybench_x86_strlen(const char *text) {
  uint64_t length = 0;
  while (length < 4096 && text[length] != '\0')
    length++;
  return length;
}

__attribute__((noinline, used))
static uint64_t polybench_x86_strnlen(const char *text, uint64_t limit) {
  uint64_t length = 0;
  if (limit > 4096)
    limit = 4096;
  while (length < limit && text[length] != '\0')
    length++;
  return length;
}

__attribute__((noinline, used))
static uint64_t polybench_x86_memcpy(uint8_t *dest, const uint8_t *src,
    uint64_t size) {
  for (uint64_t n = 0; n < size; n++)
    dest[n] = src[n];
  return (uint64_t) (uintptr_t) dest;
}

__attribute__((noinline, used))
static uint64_t polybench_x86_memset(uint8_t *dest, uint64_t value,
    uint64_t size) {
  for (uint64_t n = 0; n < size; n++)
    dest[n] = (uint8_t) value;
  return (uint64_t) (uintptr_t) dest;
}

__attribute__((noinline, used))
static uint64_t polybench_x86_memcmp(const uint8_t *left,
    const uint8_t *right, uint64_t size) {
  for (uint64_t n = 0; n < size; n++) {
    if (left[n] != right[n])
      return (uint64_t) ((int64_t) left[n] - (int64_t) right[n]);
  }
  return 0;
}

__attribute__((noinline, noipa, used))
static uint64_t polybench_x86_sum6_direct(uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
  return a0 + a1 + a2 + a3 + a4 + a5;
}

__attribute__((noinline, noipa, used))
static double polybench_x86_fp64_sum6_direct(double a0, double a1,
    double a2, double a3, double a4, double a5) {
  return a0 + a1 + a2 + a3 + a4 + a5;
}

static uint32_t riscv_ld(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (0x3U << 12) | (rd << 7) | 0x03U;
}

static uint32_t riscv_addi(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (rd << 7) | 0x13U;
}

static void emit_riscv_direct_x86_pcall(uint8_t *code, size_t *offset) {
  emit_u32(code, offset, riscv_addi(6, 0, 0)); // t1 = x86 frontend
  const size_t auipc_return_pc = *offset;
  emit_u32(code, offset, 0x00000397U); // auipc t2,0
  const size_t addi_return_offset = *offset;
  emit_u32(code, offset, 0);
  emit_u32(code, offset, 0x2200700bU); // PCALL_SIG_IMM slot 1 (x86 SysV regs)
  store_u32(code, addi_return_offset, riscv_addi(7, 7,
    (int32_t) *offset - (int32_t) auipc_return_pc));
}

static uint32_t riscv_addw(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return ((rs2 & 0x1fU) << 20) | ((rs1 & 0x1fU) << 15) |
    ((rd & 0x1fU) << 7) | 0x3bU;
}

static uint32_t riscv_or(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return ((rs2 & 0x1fU) << 20) | ((rs1 & 0x1fU) << 15) |
    (0x6U << 12) | ((rd & 0x1fU) << 7) | 0x33U;
}

static uint32_t riscv_slli(uint32_t rd, uint32_t rs1, uint32_t shamt) {
  return ((shamt & 0x3fU) << 20) | ((rs1 & 0x1fU) << 15) |
    (0x1U << 12) | ((rd & 0x1fU) << 7) | 0x13U;
}

static uint32_t riscv_srli(uint32_t rd, uint32_t rs1, uint32_t shamt) {
  return ((shamt & 0x3fU) << 20) | ((rs1 & 0x1fU) << 15) |
    (0x5U << 12) | ((rd & 0x1fU) << 7) | 0x13U;
}

static uint32_t riscv_fadd_d(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (0x01U << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | (0x7U << 12) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fsgnj_d(uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return (0x11U << 25) | ((rs2 & 0x1fU) << 20) |
    ((rs1 & 0x1fU) << 15) | ((rd & 0x1fU) << 7) | 0x53U;
}

static uint32_t riscv_fmv_d_x(uint32_t rd, uint32_t rs1) {
  return (0x79U << 25) | ((rs1 & 0x1fU) << 15) |
    ((rd & 0x1fU) << 7) | 0x53U;
}

static uint64_t call_code_no_args(const uint8_t *code) {
  uint64_t rax;
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%rbp\n"
    "pushq %%r12\n"
    "pushq %%r13\n"
    "pushq %%r14\n"
    "pushq %%r15\n"
    "call *%1\n"
    "popq %%r15\n"
    "popq %%r14\n"
    "popq %%r13\n"
    "popq %%r12\n"
    "popq %%rbp\n"
    "popq %%rbx"
    : "=a"(rax)
    : "r"(code)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static uint64_t call_code_with_rax_arg(const uint8_t *code, const char *payload) {
  register uint64_t rax asm("rax") = (uint64_t) (uintptr_t) payload;
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%rbp\n"
    "pushq %%r12\n"
    "pushq %%r13\n"
    "pushq %%r14\n"
    "pushq %%r15\n"
    "call *%1\n"
    "popq %%r15\n"
    "popq %%r14\n"
    "popq %%r13\n"
    "popq %%r12\n"
    "popq %%rbp\n"
    "popq %%rbx"
    : "+a"(rax)
    : "r"(code)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static uint64_t call_code_with_poly3_args(const uint8_t *code,
    const void *arg0, const void *arg1, uint64_t arg2) {
  uint64_t rax = (uint64_t) (uintptr_t) arg0;
  uint64_t rdx = (uint64_t) (uintptr_t) arg1;
  uint64_t rcx = arg2;
  asm volatile(
    "pushq %%rbx\n"
    "pushq %%rbp\n"
    "pushq %%r12\n"
    "pushq %%r13\n"
    "pushq %%r14\n"
    "pushq %%r15\n"
    "call *%3\n"
    "popq %%r15\n"
    "popq %%r14\n"
    "popq %%r13\n"
    "popq %%r12\n"
    "popq %%rbp\n"
    "popq %%rbx"
    : "+a"(rax), "+d"(rdx), "+c"(rcx)
    : "r"(code)
    : "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static uint64_t call_code_vec128_u32(const uint8_t *code) {
  union polybench_vec128_u32_bits arg0 = { .u = { 1, 2, 3, 4 } };
  union polybench_vec128_u32_bits arg1 = { .u = { 10, 20, 30, 40 } };
  union polybench_vec128_u32_bits result;
  polybench_vec128_u32 (*entry)(polybench_vec128_u32,
    polybench_vec128_u32) =
    (polybench_vec128_u32 (*)(polybench_vec128_u32,
      polybench_vec128_u32)) code;

  result.v = entry(arg0.v, arg1.v);
  return ((uint64_t) (result.u[3] & 0xffffU) << 48) |
    ((uint64_t) (result.u[2] & 0xffffU) << 32) |
    ((uint64_t) (result.u[1] & 0xffffU) << 16) |
    (uint64_t) (result.u[0] & 0xffffU);
}

static uint64_t call_code_fp64_6(const uint8_t *code) {
  double (*entry)(double, double, double, double, double, double) =
    (double (*)(double, double, double, double, double, double)) code;
  return fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0));
}

static int run_loop_program(int arch, uint64_t *result, uint64_t *insn_delta) {
  const size_t code_size = 3 + 8 + 4 * 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: mmap failed: %s\n", strerror(errno));
    return -1;
  }

  code[0] = 0x90;
  code[1] = 0x90;
  code[2] = 0x90;

  size_t offset = 3;
  if (arch == POLY_ARCH_AARCH64) {
    const uint8_t raw_switch[] = { 0x0f, 0x3a, 0xfc, 0x01 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, 0xd2800000U | ((uint32_t) LOOP_ITERS << 5)); // movz x0,#LOOP_ITERS
    emit_u32(code, &offset, 0xd1000400U); // sub x0,x0,#1
    emit_u32(code, &offset, 0xb5ffffe0U); // cbnz x0, previous instruction
    emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape
  } else {
    const uint8_t raw_switch[] = { 0x0f, 0x3a, 0xfc, 0x02 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, ((uint32_t) LOOP_ITERS << 20) | 0x00000513U); // addi a0,zero,LOOP_ITERS
    emit_u32(code, &offset, 0xfff50513U); // addi a0,a0,-1
    emit_u32(code, &offset, 0xfe051ee3U); // bne a0,zero, previous instruction
    emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  }
  code[offset++] = 0xc3;

  poly_foreign_insn_count_status();
  uint64_t before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  uint64_t after = read_rax();
  *insn_delta = after - before;

  munmap(code, code_size);
  return 0;
}

__attribute__((noinline, noipa))
static uint64_t polybench_direct_x86_pcall_aarch64_sum6(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) polybench_x86_sum6_direct;
  asm volatile(
    ".byte 0x0f,0x3a,0xfc,0x01\n"
    ".long 0xaa0703f0\n" // mov x16,x7, x86 target from R10/P7
    ".long 0xd2800020\n" // movz x0,#1
    ".long 0xd2800041\n" // movz x1,#2
    ".long 0xd2800062\n" // movz x2,#3
    ".long 0xd2800083\n" // movz x3,#4
    ".long 0xd28000a4\n" // movz x4,#5
    ".long 0xd28000c5\n" // movz x5,#6
    ".long 0xd2800011\n" // movz x17,#0 (x86 frontend)
    ".long 0x10000052\n" // adr x18,return
    ".long 0xd5032f3f\n" // generic pcall frontend=x17 target=x16
    ".long 0xd5032e1f\n" // return: aarch64 polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

__attribute__((noinline, noipa))
static uint64_t polybench_direct_x86_pcall_riscv_sum6(void) {
  uint64_t result;
  register uint64_t target asm("r10") =
    (uint64_t) (uintptr_t) polybench_x86_sum6_direct;
  asm volatile(
    ".byte 0x0f,0x3a,0xfc,0x02\n"
    ".long 0x00088293\n" // addi t0,a7,0, x86 target from R10/P7
    ".long 0x00100513\n" // addi a0,zero,1
    ".long 0x00200593\n" // addi a1,zero,2
    ".long 0x00300613\n" // addi a2,zero,3
    ".long 0x00400693\n" // addi a3,zero,4
    ".long 0x00500713\n" // addi a4,zero,5
    ".long 0x00600793\n" // addi a5,zero,6
    ".long 0x00000313\n" // addi t1,zero,0 (x86 frontend)
    ".long 0x00000397\n" // auipc t2,0
    ".long 0x00c38393\n" // addi t2,t2,12 -> return
    ".long 0x1200700b\n" // generic pcall frontend=t1 target=t0 return=t2
    ".long 0x0000700b\n" // return: riscv polyctrl x86 escape
    : "=a"(result), "+r"(target)
    :
    : "rdx", "rcx", "rdi", "rsi", "r8", "r9", "r11", "r12", "r13",
      "r14", "memory");
  return result;
}

static int run_direct_x86_pcall_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = polybench_direct_x86_pcall_aarch64_sum6();
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;
  return 0;
}

static int run_direct_x86_pcall_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = polybench_direct_x86_pcall_riscv_sum6();
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;
  return 0;
}

static int run_direct_x86_fp64_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64 direct x86 FP mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 1));
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_fp64_sum6_direct);
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 5));
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape
  code[offset++] = 0xc3;

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result_bits = call_code_fp64_6(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_direct_x86_fp64_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv direct x86 FP mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 11));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 15));
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_fp64_sum6_direct);
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result_bits = call_code_fp64_6(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_mixed_program(uint64_t *result, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 3 + 8 + 5 * 4 + 2 * 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: mixed mmap failed: %s\n", strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, 0xd2800140U); // movz x0,#10
  emit_u32(code, &offset, 0x91001400U); // add x0,x0,#5
  emit_u32(code, &offset, 0x10000070U); // adr x16,riscv target
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_u32(code, &offset, 0xd5032f1fU); // generic poly switch

  emit_u32(code, &offset, 0x01b50513U); // addi a0,a0,27
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_compressed_mixed_program(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 2 + 8 + 5 * 4 + 2 + 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: compressed mixed mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, 0xd2800140U); // movz x0,#10
  emit_u32(code, &offset, 0x91001400U); // add x0,x0,#5
  emit_u32(code, &offset, 0x10000070U); // adr x16,riscv target
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_u32(code, &offset, 0xd5032f1fU); // generic poly switch

  emit_u16(code, &offset, 0x056dU); // c.addi a0,27
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_compressed_reverse_mixed_program(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 2 + 8 + 2 + 4 * 4 + 2 * 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: compressed reverse mixed mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u16(code, &offset, 0x451dU); // c.li a0,7
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t addi_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  emit_u32(code, &offset, 0x1000700bU); // generic poly switch
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91008c00U); // add x0,x0,#35
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;
  store_u32(code, addi_target_offset, riscv_addi(5, 5,
    (int32_t) aarch64_target_offset - (int32_t) auipc_target_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_reverse_mixed_program(uint64_t *result, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 3 + 8 + 5 * 4 + 2 * 4 + 1;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: reverse mixed mmap failed: %s\n", strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u32(code, &offset, 0x00700513U); // addi a0,zero,7
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t addi_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  emit_u32(code, &offset, 0x1000700bU); // generic poly switch
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91008c00U); // add x0,x0,#35
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;
  store_u32(code, addi_target_offset, riscv_addi(5, 5,
    (int32_t) aarch64_target_offset - (int32_t) auipc_target_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset =
    aarch64_body_offset + 4 + 16 + 4 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_u32(code, &offset, 0xd2800051U); // movz x17,#2 (RISC-V frontend)
  emit_aarch64_movabs(code, &offset, 18,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032c1fU); // PCALL_SIG_IMM slot 0
  emit_u32(code, &offset, 0x91000400U); // add x0,x0,#1
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01550513U); // addi a0,a0,21
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, riscv_addi(6, 0, 1)); // frontend AArch64
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000397U); // auipc x7,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x2000700bU); // PCALL_SIG_IMM slot 0
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00150513U); // addi a0,a0,1
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91005400U); // add x0,x0,#21
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(7, 7,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv FP call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double) = (double (*)(double)) code;
  *result_bits = fp64_to_bits(entry(2.0));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 FP call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double) = (double (*)(double)) code;
  *result_bits = fp64_to_bits(entry(2.0));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp8_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv FP8 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  for (uint32_t reg = 11; reg <= 17; reg++)
    emit_u32(code, &offset, riscv_fadd_d(10, 10, reg));
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double, double, double, double, double, double, double,
    double) = (double (*)(double, double, double, double, double, double,
      double, double)) code;
  *result_bits = fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp8_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 FP8 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  for (uint32_t reg = 1; reg <= 7; reg++)
    emit_u32(code, &offset, aarch64_fadd_d(0, 0, reg));
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double, double, double, double, double, double, double,
    double) = (double (*)(double, double, double, double, double, double,
      double, double)) code;
  *result_bits = fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp64_stack_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv FP64 stack call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 4 + 8 * 8 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 4 + 1;

  emit_u32(code, &offset, 0xd10103ffU); // sub sp,sp,#64
  for (uint32_t n = 0; n < 8; n++) {
    emit_u32(code, &offset, aarch64_ldr_x_sp(8, 72 + n * 8));
    emit_u32(code, &offset, aarch64_str_x_sp(8, n * 8));
  }
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032ebfU); // aarch64 polyctrl FP64-stack call, FP64-stack call RISC-V
  emit_u32(code, &offset, 0x910103ffU); // add sp,sp,#64
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  for (uint32_t reg = 11; reg <= 17; reg++)
    emit_u32(code, &offset, riscv_fadd_d(10, 10, reg));
  for (uint32_t n = 0; n < 8; n++) {
    emit_u32(code, &offset, riscv_fmv_d_x(15, 10 + n));
    emit_u32(code, &offset, riscv_fadd_d(10, 10, 15));
  }
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double, double, double, double, double, double, double,
    double, double, double, double, double, double, double, double, double) =
    (double (*)(double, double, double, double, double, double, double,
      double, double, double, double, double, double, double, double,
      double)) code;
  *result_bits = fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
    9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_fp64_stack_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 FP64 stack call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  for (uint32_t n = 0; n < 8; n++)
    emit_u32(code, &offset, riscv_ld(10 + n, 2, 8 + n * 8));
  emit_u32(code, &offset, riscv_addi(2, 2, -64));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0a00700bU); // riscv polyctrl FP64-stack call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, riscv_addi(2, 2, 64));
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  for (uint32_t reg = 1; reg <= 7; reg++)
    emit_u32(code, &offset, aarch64_fadd_d(0, 0, reg));
  for (uint32_t n = 0; n < 8; n++) {
    emit_u32(code, &offset, aarch64_ldr_d_sp(1, n * 8));
    emit_u32(code, &offset, aarch64_fadd_d(0, 0, 1));
  }
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double, double, double, double, double, double, double,
    double, double, double, double, double, double, double, double, double) =
    (double (*)(double, double, double, double, double, double, double,
      double, double, double, double, double, double, double, double,
      double)) code;
  *result_bits = fp64_to_bits(entry(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
    9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static void emit_riscv_vec128_u32_pair_add(uint8_t *code, size_t *offset,
    uint32_t rd_lo, uint32_t rd_hi, uint32_t left_lo, uint32_t left_hi,
    uint32_t right_lo, uint32_t right_hi) {
  emit_u32(code, offset, riscv_addw(5, left_lo, right_lo));
  emit_u32(code, offset, riscv_srli(6, left_lo, 32));
  emit_u32(code, offset, riscv_srli(7, right_lo, 32));
  emit_u32(code, offset, riscv_addw(6, 6, 7));
  emit_u32(code, offset, riscv_slli(6, 6, 32));
  emit_u32(code, offset, riscv_or(rd_lo, 5, 6));

  emit_u32(code, offset, riscv_addw(5, left_hi, right_hi));
  emit_u32(code, offset, riscv_srli(6, left_hi, 32));
  emit_u32(code, offset, riscv_srli(7, right_hi, 32));
  emit_u32(code, offset, riscv_addw(6, 6, 7));
  emit_u32(code, offset, riscv_slli(6, 6, 32));
  emit_u32(code, offset, riscv_or(rd_hi, 5, 6));
}

static int run_cross_call_vec128_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv vec128 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  const size_t r10_imm_offset = emit_x86_movabs_r10(code, &offset, 0);
  const size_t r11_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc, 0x21
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t pcall_return_offset = offset;
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032effU); // aarch64 polyctrl vec128 call, vec128 call RISC-V
  emit_u32(code, &offset, 0xd65f03c0U); // ret to x86 PCALL return cookie

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_riscv_vec128_u32_pair_add(code, &offset, 10, 11, 10, 11, 12, 13);
  emit_u32(code, &offset, 0x00008067U); // ret

  store_u64(code, r10_imm_offset,
    (uint64_t) (uintptr_t) (code + aarch64_body_offset));
  store_u64(code, r11_imm_offset,
    (uint64_t) (uintptr_t) (code + pcall_return_offset));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_vec128_u32(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_vec128_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 vec128 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  const size_t r10_imm_offset = emit_x86_movabs_r10(code, &offset, 0);
  const size_t r11_imm_offset = emit_x86_movabs_r11(code, &offset, 0);
  const uint8_t pcall[] = {
    0x0f, 0x3a, 0xfc, 0x22
  };
  emit_bytes(code, &offset, pcall, sizeof(pcall));
  const size_t pcall_return_offset = offset;
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t riscv_body_offset = offset;
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0e00700bU); // riscv polyctrl vec128 call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00008067U); // ret to x86 PCALL return cookie

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x4ea18400U); // add v0.4s,v0.4s,v1.4s
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u64(code, r10_imm_offset,
    (uint64_t) (uintptr_t) (code + riscv_body_offset));
  store_u64(code, r11_imm_offset,
    (uint64_t) (uintptr_t) (code + pcall_return_offset));
  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_vec128_u32(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_mixed_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv mixed call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 8 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_u32(code, &offset, 0xd28000e0U); // movz x0,#7
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0xd22575d3U); // fcvt.d.l fa1,a0
  emit_u32(code, &offset, 0x02b57553U); // fadd.d fa0,fa0,fa1
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double) = (double (*)(double)) code;
  *result_bits = fp64_to_bits(entry(2.5));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_mixed_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 mixed call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00700513U); // addi a0,zero,7
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x9e630001U); // ucvtf d1,x0
  emit_u32(code, &offset, 0x1e612800U); // fadd d0,d0,d1
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double) = (double (*)(double)) code;
  *result_bits = fp64_to_bits(entry(2.5));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_stack_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv stack call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 4 * 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 3 * 4 + 1;

  emit_u32(code, &offset, 0xd10043ffU); // sub sp,sp,#16
  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_u32(code, &offset, 0xd2800128U); // movz x8,#9
  emit_u32(code, &offset, 0xf90003e8U); // str x8,[sp]
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0x910043ffU); // add sp,sp,#16
  emit_u32(code, &offset, 0x91003400U); // add x0,x0,#13
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00013283U); // ld t0,0(sp)
  emit_u32(code, &offset, 0x00550533U); // add a0,a0,t0
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_stack_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 stack call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0xff010113U); // addi sp,sp,-16
  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  emit_u32(code, &offset, 0x00900393U); // addi t2,zero,9
  emit_u32(code, &offset, 0x00713023U); // sd t2,0(sp)
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x01010113U); // addi sp,sp,16
  emit_u32(code, &offset, 0x00d50513U); // addi a0,a0,13
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xf94003e8U); // ldr x8,[sp]
  emit_u32(code, &offset, 0x8b080000U); // add x0,x0,x8
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_saved_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv saved-reg call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 2 * 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 2 * 4 + 1;

  emit_u32(code, &offset, 0xd28000b3U); // movz x19,#5
  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0x8b130000U); // add x0,x0,x19
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01150513U); // addi a0,a0,17
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_saved_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 saved-reg call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00500413U); // addi s0,zero,5
  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00850533U); // add a0,a0,s0
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91004400U); // add x0,x0,#17
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_saved_fp_aarch64_to_riscv(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv saved-fp call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] =
    { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 2 * 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 2 * 4 + 1;

  emit_u32(code, &offset, 0x1e604008U); // fmov d8,d0
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 0));
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, aarch64_fadd_d(0, 0, 8));
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, riscv_fsgnj_d(8, 10, 10)); // fmv.d fs0,fa0
  emit_u32(code, &offset, riscv_fadd_d(10, 8, 10));
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double) = (double (*)(double)) code;
  *result_bits = fp64_to_bits(entry(3.0));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_saved_fp_riscv_to_aarch64(uint64_t *result_bits,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 saved-fp call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, riscv_fsgnj_d(8, 10, 10)); // fmv.d fs0,fa0
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 10));
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, riscv_fadd_d(10, 10, 8));
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x1e604008U); // fmov d8,d0
  emit_u32(code, &offset, aarch64_fadd_d(0, 8, 0));
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  double (*entry)(double) = (double (*)(double)) code;
  *result_bits = fp64_to_bits(entry(3.0));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_pair_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv pair call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 2 * 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0x8b010000U); // add x0,x0,x1
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  emit_u32(code, &offset, 0x01600593U); // addi a1,zero,22
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_pair_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 pair call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00b50533U); // add a0,a0,a1
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_u32(code, &offset, 0xd28002c1U); // movz x1,#22
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_compact_u32_f32_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv compact u32/f32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 0, 0x4010000000000003ULL);
  emit_u32(code, &offset, 0xd28000a1U); // movz x1,#5
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e7fU); // aarch64 polyctrl compact u32-f32 call, compact {u32,float} call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0xd015f7d3U); // fcvt.s.wu fa5,a1
  emit_u32(code, &offset, 0x00b5053bU); // addw a0,a0,a1
  emit_u32(code, &offset, 0x00a7f553U); // fadd.s fa0,fa5,fa0
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_compact_f32_u32_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 320;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv compact f32/u32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 0, 0x0000000340100000ULL);
  emit_u32(code, &offset, 0xd28000a1U); // movz x1,#5
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e9fU); // aarch64 polyctrl compact f32-u32 call, compact {float,u32} call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0xd015f7d3U); // fcvt.s.wu fa5,a1
  emit_u32(code, &offset, 0x00b5053bU); // addw a0,a0,a1
  emit_u32(code, &offset, 0x00a7f553U); // fadd.s fa0,fa5,fa0
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_compact_u32_f32_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 384;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 compact u32/f32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00300513U); // addi a0,zero,3
  emit_u32(code, &offset, 0x00500593U); // addi a1,zero,5
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0600700bU); // riscv polyctrl compact {u32,float}, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0xe0050653U); // fmv.x.w a2,fa0
  emit_u32(code, &offset, 0x02061613U); // slli a2,a2,32
  emit_u32(code, &offset, 0x00c56533U); // or a0,a0,a2
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0x91001400U); // add x0,x0,#5
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t (*entry)(float) = (uint64_t (*)(float)) code;
  *result = entry(2.25f);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_compact_f32_u32_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 384;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 compact f32/u32 mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x00300513U); // addi a0,zero,3
  emit_u32(code, &offset, 0x00500593U); // addi a1,zero,5
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0800700bU); // riscv polyctrl compact {float,u32}, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0xe0050653U); // fmv.x.w a2,fa0
  emit_u32(code, &offset, 0x02051513U); // slli a0,a0,32
  emit_u32(code, &offset, 0x00c56533U); // or a0,a0,a2
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_aarch64_movabs(code, &offset, 1, 0x0000000500000000ULL);
  emit_u32(code, &offset, 0x8b010000U); // add x0,x0,x1
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t (*entry)(float) = (uint64_t (*)(float)) code;
  *result = entry(2.25f);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_syscall_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv syscall call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x0ac00893U); // addi a7,zero,172
  emit_u32(code, &offset, 0x00000073U); // ecall
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_syscall_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 syscall call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xd2801588U); // movz x8,#172
  emit_u32(code, &offset, 0xd4000001U); // svc #0
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_break_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv break call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00100893U); // addi a7,zero,1
  emit_u32(code, &offset, 0x00100073U); // ebreak
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_break_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 break call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xd4200020U); // brk #1, neutral break trap
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv direct x86 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00008413U); // addi s0,ra,0; save cross-return cookie
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, 0x00040093U); // addi ra,s0,0; restore cross-return cookie
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_strlen);

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  static const char payload[] = "polyglot";
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_with_rax_arg(code, payload);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 direct x86 call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xaa1e03f3U); // mov x19,x30; save cross-return cookie
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_strlen);
  emit_u32(code, &offset, 0xaa1303feU); // mov x30,x19; restore cross-return cookie
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  static const char payload[] = "polyglot";
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_with_rax_arg(code, payload);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_memcmp_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv direct x86 memcmp mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00008413U); // addi s0,ra,0; save cross-return cookie
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, 0x00040093U); // addi ra,s0,0; restore cross-return cookie
  emit_u32(code, &offset, 0x02a50513U); // addi a0,a0,42
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_memcmp);

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));

  static const char left[] = "polyglot";
  static const char right[] = "polyglot";
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_with_poly3_args(code, left, right, sizeof(left));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_memcmp_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 direct x86 memcmp mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xaa1e03f3U); // mov x19,x30; save cross-return cookie
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_memcmp);
  emit_u32(code, &offset, 0xaa1303feU); // mov x30,x19; restore cross-return cookie
  emit_u32(code, &offset, 0x9100a800U); // add x0,x0,#42
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  static const char left[] = "polyglot";
  static const char right[] = "polyglot";
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_with_poly3_args(code, left, right, sizeof(left));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_memops_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv direct x86 memops mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, riscv_addi(8, 1, 0));   // s0=ra; save cross-return cookie
  emit_u32(code, &offset, riscv_addi(9, 11, 0));  // s1=a1; save source
  emit_u32(code, &offset, riscv_addi(18, 12, 0)); // s2=a2; save count
  emit_u32(code, &offset, riscv_addi(19, 10, 0)); // s3=a0; save destination
  emit_u32(code, &offset, riscv_addi(11, 0, 0x58)); // a1='X'
  const size_t auipc_memset_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_memset_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, riscv_addi(10, 19, 0)); // a0=s3
  emit_u32(code, &offset, riscv_addi(11, 9, 0));  // a1=s1
  emit_u32(code, &offset, riscv_addi(12, 18, 0)); // a2=s2
  const size_t auipc_memcpy_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_memcpy_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, riscv_addi(10, 9, 0)); // a0=s1
  emit_u32(code, &offset, riscv_addi(11, 0, 4)); // a1=4
  const size_t auipc_strnlen_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_strnlen_offset = offset;
  emit_u32(code, &offset, 0);
  emit_riscv_direct_x86_pcall(code, &offset);
  emit_u32(code, &offset, riscv_addi(10, 10, 38)); // a0=42
  emit_u32(code, &offset, riscv_addi(1, 8, 0)); // ra=s0
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t memset_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_memset);
  const size_t memcpy_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_memcpy);
  const size_t strnlen_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) polybench_x86_strnlen);

  store_u32(code, ld_memset_offset, riscv_ld(5, 5,
    (int32_t) memset_data_offset - (int32_t) auipc_memset_pc));
  store_u32(code, ld_memcpy_offset, riscv_ld(5, 5,
    (int32_t) memcpy_data_offset - (int32_t) auipc_memcpy_pc));
  store_u32(code, ld_strnlen_offset, riscv_ld(5, 5,
    (int32_t) strnlen_data_offset - (int32_t) auipc_strnlen_pc));

  static const char source[] = "polyglot";
  char dest[sizeof(source)];
  memset(dest, 0, sizeof(dest));
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t raw_result = call_code_with_poly3_args(code, dest,
    source, sizeof(source));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;
  *result = (raw_result == 42 &&
      memcmp(dest, source, sizeof(source)) == 0) ? 42 : 0;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_direct_x86_memops_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 direct x86 memops mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x3a, 0xfc, 0x02 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000700bU); // riscv polyctrl x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, aarch64_mov_reg(19, 30)); // save cross-return cookie
  emit_u32(code, &offset, aarch64_mov_reg(20, 1));  // save source
  emit_u32(code, &offset, aarch64_mov_reg(21, 2));  // save count
  emit_u32(code, &offset, aarch64_mov_reg(22, 0));  // save destination
  emit_u32(code, &offset, 0xd2800b01U); // movz x1,#'X'
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_memset);
  emit_u32(code, &offset, aarch64_mov_reg(0, 22));
  emit_u32(code, &offset, aarch64_mov_reg(1, 20));
  emit_u32(code, &offset, aarch64_mov_reg(2, 21));
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_memcpy);
  emit_u32(code, &offset, aarch64_mov_reg(0, 20));
  emit_u32(code, &offset, 0xd2800081U); // movz x1,#4
  emit_aarch64_direct_x86_pcall(code, &offset,
    (uint64_t) (uintptr_t) polybench_x86_strnlen);
  emit_u32(code, &offset, 0x91009800U); // add x0,x0,#38
  emit_u32(code, &offset, aarch64_mov_reg(30, 19)); // restore cross-return cookie
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_target_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  static const char source[] = "polyglot";
  char dest[sizeof(source)];
  memset(dest, 0, sizeof(dest));
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t raw_result = call_code_with_poly3_args(code, dest,
    source, sizeof(source));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;
  *result = (raw_result == 42 &&
      memcmp(dest, source, sizeof(source)) == 0) ? 42 : 0;

  munmap(code, code_size);
  return 0;
}

static int run_nested_cross_call(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 384;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: nested cross call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x3a, 0xfc, 0x01 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_outer_offset = offset;
  const size_t aarch64_outer_return_offset =
    aarch64_outer_offset + 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_outer_return_offset + 4 + 1;

  emit_u32(code, &offset, 0xd2800140U); // movz x0,#10
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_outer_return_offset));
  emit_u32(code, &offset, 0xd5032e5fU); // aarch64 polyctrl riscv call, call RISC-V
  emit_u32(code, &offset, 0xd5032e1fU); // aarch64 polyctrl x86 escape, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00b50513U); // addi a0,a0,11
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0400700bU); // riscv polyctrl call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00150513U); // addi a0,a0,1
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_inner_offset = offset;
  emit_u32(code, &offset, 0x91005000U); // add x0,x0,#20
  emit_u32(code, &offset, 0xd65f03c0U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t target_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + aarch64_inner_offset));
  const size_t return_data_offset = offset;
  emit_u64(code, &offset, (uint64_t) (uintptr_t) (code + riscv_return_offset));

  store_u32(code, ld_target_offset, riscv_ld(5, 5,
    (int32_t) target_data_offset - (int32_t) auipc_target_pc));
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  *result = call_code_no_args(code);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int check_loop(const char *name, int arch) {
  uint64_t result = 0;
  uint64_t delta = 0;
  if (run_loop_program(arch, &result, &delta) < 0)
    return -1;

  const uint64_t min_expected_delta = 1 + (uint64_t) LOOP_ITERS * 2 + 1;
  printf("POLYBENCH_RESULT: arch=%s result=%llu raw_insn_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) delta);

  if (result != 0) {
    fprintf(stderr, "POLYBENCH_FAIL: %s loop result expected 0 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (delta < min_expected_delta) {
    fprintf(stderr, "POLYBENCH_FAIL: %s raw instruction delta expected at least %llu got %llu\n",
      name, (unsigned long long) min_expected_delta, (unsigned long long) delta);
    return -1;
  }
  return 0;
}

static int check_switch_delta_max(const char *kind, const char *name,
    uint64_t switch_delta, uint64_t max_switch_delta) {
  if (switch_delta > max_switch_delta) {
    fprintf(stderr, "POLYBENCH_FAIL: %s %s switch delta expected at most %llu got %llu\n",
      kind, name, (unsigned long long) max_switch_delta,
      (unsigned long long) switch_delta);
    return -1;
  }
  return 0;
}

static int check_mixed_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_MIXED_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: mixed %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: mixed %s raw instruction delta expected at least 4 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 3) {
    fprintf(stderr, "POLYBENCH_FAIL: mixed %s switch delta expected at least 3 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("mixed", name, switch_delta,
        POLYBENCH_MIXED_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_mixed(void) {
  if (check_mixed_direction("aarch64-to-riscv", run_mixed_program) < 0)
    return -1;
  if (check_mixed_direction("aarch64-to-compressed-riscv",
        run_compressed_mixed_program) < 0)
    return -1;
  if (check_mixed_direction("riscv-to-aarch64", run_reverse_mixed_program) < 0)
    return -1;
  if (check_mixed_direction("riscv-compressed-to-aarch64",
        run_compressed_reverse_mixed_program) < 0)
    return -1;
  return 0;
}

static int check_cross_call_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *),
    uint64_t max_switch_delta) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call", name, switch_delta,
        max_switch_delta) < 0)
    return -1;
  return 0;
}

static int check_direct_x86_pcall_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_DIRECT_X86_PCALL_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 21) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 pcall %s result expected 21 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 pcall %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 pcall %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("direct x86 pcall", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_direct_x86_fp64_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_DIRECT_X86_FP64_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x403d000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 FP64 %s expected 0x403d000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 FP64 %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: direct x86 FP64 %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("direct x86 FP64", name, switch_delta,
        POLYBENCH_DIRECT_X86_PCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_fp_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_FP_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4030000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP %s result expected 0x4030000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call FP", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_fp8_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_FP8_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4042000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP8 %s result expected 0x4042000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 14) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP8 %s raw instruction delta expected at least 14 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP8 %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call FP8", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_fp64_stack_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_FP64_STACK_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4061000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP64 stack %s result expected 0x4061000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 30) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP64 stack %s raw instruction delta expected at least 30 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call FP64 stack %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call FP64 stack", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_vec128_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_VEC128_RESULT: direction=%s packed=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != UINT64_C(0x002c00210016000b)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call vec128 %s expected 0x002c00210016000b got 0x%016llx\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call vec128 %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call vec128 %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call vec128", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_mixed_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_MIXED_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x4038000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call mixed %s result expected 0x4038000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call mixed %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call mixed %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call mixed", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_stack_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_STACK_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call stack %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call stack %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call stack %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call stack", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_saved_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_SAVED_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call saved", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_saved_fp_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result_bits = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result_bits, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_SAVED_FP_RESULT: direction=%s bits=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result_bits, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result_bits != UINT64_C(0x402e000000000000)) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved-fp %s expected 0x402e000000000000 got 0x%016llx\n",
      name, (unsigned long long) result_bits);
    return -1;
  }
  if (insn_delta < 11) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved-fp %s raw instruction delta expected at least 11 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call saved-fp %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call saved-fp", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_pair_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_PAIR_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call pair %s result expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 9) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call pair %s raw instruction delta expected at least 9 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call pair %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call pair", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_compact_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *), uint64_t expected) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_COMPACT_RESULT: direction=%s packed=0x%016llx raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != expected) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call compact %s expected 0x%016llx got 0x%016llx\n",
      name, (unsigned long long) expected, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 9) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call compact %s raw instruction delta expected at least 9 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call compact %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call compact", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_syscall_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_SYSCALL_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 4242) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call syscall %s result expected 4242 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 9) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call syscall %s raw instruction delta expected at least 9 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call syscall %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call syscall", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_break_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *),
    uint64_t expected) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_BREAK_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != expected) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call break %s expected %llu got %llu\n",
      name, (unsigned long long) expected, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call break %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call break %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call break", name, switch_delta,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_string_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_STRING_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call string %s strlen expected 8 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call string %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call string %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call string", name, switch_delta,
        POLYBENCH_DIRECT_X86_LIBCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_direct_x86_memcmp_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_DIRECT_X86_MEMCMP_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memcmp %s expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memcmp %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memcmp %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call direct x86 memcmp", name,
        switch_delta, POLYBENCH_DIRECT_X86_LIBCALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_call_direct_x86_memops_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_DIRECT_X86_MEMOPS_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memops %s expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 20) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memops %s raw instruction delta expected at least 20 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call direct x86 memops %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  if (check_switch_delta_max("cross call direct x86 memops", name,
        switch_delta, POLYBENCH_DIRECT_X86_MEMOPS_MAX_SWITCH_DELTA) < 0)
    return -1;
  return 0;
}

static int check_cross_calls(void) {
  if (check_cross_call_direction("aarch64-calls-riscv",
        run_cross_call_aarch64_to_riscv,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  if (check_cross_call_direction("riscv-calls-aarch64",
        run_cross_call_riscv_to_aarch64,
        POLYBENCH_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  if (check_cross_call_direction("nested-aarch64-riscv-aarch64",
        run_nested_cross_call,
        POLYBENCH_NESTED_CROSS_CALL_MAX_SWITCH_DELTA) < 0)
    return -1;
  if (check_direct_x86_pcall_direction("aarch64-calls-x86-direct",
        run_direct_x86_pcall_aarch64) < 0)
    return -1;
  if (check_direct_x86_pcall_direction("riscv-calls-x86-direct",
        run_direct_x86_pcall_riscv) < 0)
    return -1;
  if (check_direct_x86_fp64_direction("aarch64-calls-x86-direct-fp64",
        run_direct_x86_fp64_aarch64) < 0)
    return -1;
  if (check_direct_x86_fp64_direction("riscv-calls-x86-direct-fp64",
        run_direct_x86_fp64_riscv) < 0)
    return -1;
  if (check_cross_call_fp_direction("aarch64-calls-riscv-fp",
        run_cross_call_fp_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp_direction("riscv-calls-aarch64-fp",
        run_cross_call_fp_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_fp8_direction("aarch64-calls-riscv-fp8",
        run_cross_call_fp8_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp8_direction("riscv-calls-aarch64-fp8",
        run_cross_call_fp8_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_fp64_stack_direction("aarch64-calls-riscv-fp64-stack",
        run_cross_call_fp64_stack_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp64_stack_direction("riscv-calls-aarch64-fp64-stack",
        run_cross_call_fp64_stack_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_vec128_direction("aarch64-calls-riscv-vec128",
        run_cross_call_vec128_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_vec128_direction("riscv-calls-aarch64-vec128",
        run_cross_call_vec128_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_mixed_direction("aarch64-calls-riscv-mixed",
        run_cross_call_mixed_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_mixed_direction("riscv-calls-aarch64-mixed",
        run_cross_call_mixed_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_stack_direction("aarch64-calls-riscv-stack",
        run_cross_call_stack_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_stack_direction("riscv-calls-aarch64-stack",
        run_cross_call_stack_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_saved_direction("aarch64-calls-riscv-saved",
        run_cross_call_saved_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_saved_direction("riscv-calls-aarch64-saved",
        run_cross_call_saved_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_saved_fp_direction("aarch64-calls-riscv-saved-fp",
        run_cross_call_saved_fp_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_saved_fp_direction("riscv-calls-aarch64-saved-fp",
        run_cross_call_saved_fp_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_pair_direction("aarch64-calls-riscv-pair",
        run_cross_call_pair_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_pair_direction("riscv-calls-aarch64-pair",
        run_cross_call_pair_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_compact_direction("aarch64-calls-riscv-compact-u32-f32",
        run_cross_call_compact_u32_f32_aarch64_to_riscv,
        UINT64_C(0x40e8000000000008)) < 0)
    return -1;
  if (check_cross_call_compact_direction("aarch64-calls-riscv-compact-f32-u32",
        run_cross_call_compact_f32_u32_aarch64_to_riscv,
        UINT64_C(0x0000000840e80000)) < 0)
    return -1;
  if (check_cross_call_compact_direction("riscv-calls-aarch64-compact-u32-f32",
        run_cross_call_compact_u32_f32_riscv_to_aarch64,
        UINT64_C(0x4010000000000008)) < 0)
    return -1;
  if (check_cross_call_compact_direction("riscv-calls-aarch64-compact-f32-u32",
        run_cross_call_compact_f32_u32_riscv_to_aarch64,
        UINT64_C(0x0000000840100000)) < 0)
    return -1;
  if (check_cross_call_syscall_direction("aarch64-calls-riscv-syscall",
        run_cross_call_syscall_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_syscall_direction("riscv-calls-aarch64-syscall",
        run_cross_call_syscall_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_break_direction("aarch64-calls-riscv-break",
        run_cross_call_break_aarch64_to_riscv, UINT64_C(0x4c000401)) < 0)
    return -1;
  if (check_cross_call_break_direction("riscv-calls-aarch64-break",
        run_cross_call_break_riscv_to_aarch64, UINT64_C(0x4c000301)) < 0)
    return -1;
  if (check_cross_call_string_direction("aarch64-calls-riscv-direct-x86",
        run_cross_call_direct_x86_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_string_direction("riscv-calls-aarch64-direct-x86",
        run_cross_call_direct_x86_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_direct_x86_memcmp_direction("aarch64-calls-riscv-direct-x86-memcmp",
        run_cross_call_direct_x86_memcmp_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_direct_x86_memcmp_direction("riscv-calls-aarch64-direct-x86-memcmp",
        run_cross_call_direct_x86_memcmp_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_direct_x86_memops_direction("aarch64-calls-riscv-direct-x86-memops",
        run_cross_call_direct_x86_memops_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_direct_x86_memops_direction("riscv-calls-aarch64-direct-x86-memops",
        run_cross_call_direct_x86_memops_riscv_to_aarch64) < 0)
    return -1;
  return 0;
}

int main(void) {
  puts("POLYBENCH: start");
  install_polybench_trap_vector();
  if (check_loop("aarch64", POLY_ARCH_AARCH64) < 0)
    return 1;
  if (check_loop("riscv", POLY_ARCH_RISCV) < 0)
    return 1;
  if (check_mixed() < 0)
    return 1;
  if (check_cross_calls() < 0)
    return 1;
  puts("POLYBENCH_OK");
  return 0;
}
