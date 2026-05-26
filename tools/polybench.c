#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
  LOOP_ITERS = 200,
  POLY_IMPORT_FUNC_STRLEN = 8,
  POLY_IMPORT_FUNC_MEMCPY = 9,
  POLY_IMPORT_FUNC_MEMSET = 10,
  POLY_IMPORT_FUNC_MEMCMP = 11
};

#define POLY_IMPORT_CALL_BASE 0xffffffffffffe000ULL
#define POLY_IMPORT_CALL_STRIDE 0x10ULL

static inline void poly_mode_x86(void) { asm volatile(".byte 0x0f,0x24,0x00,0x50,0x4f,0x4c,0x59,0x21" ::: "memory"); }
static inline void poly_switch_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x30" ::: "memory"); }
static inline void poly_foreign_insn_count_status(void) { asm volatile(".byte 0x4e,0x0f,0x0b,0x53,0x57,0x43,0x48,0x32" ::: "memory"); }

static inline uint64_t read_rax(void) {
  uint64_t value;
  asm volatile("" : "=a"(value));
  return value;
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

static uint32_t aarch64_mov_reg(uint32_t rd, uint32_t rn) {
  return 0xaa0003e0U | ((rn & 0x1fU) << 16) | (rd & 0x1fU);
}

static uint64_t fp64_to_bits(double value) {
  union {
    double d;
    uint64_t u;
  } fp;
  fp.d = value;
  return fp.u;
}

static uint32_t riscv_ld(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (0x3U << 12) | (rd << 7) | 0x03U;
}

static uint32_t riscv_addi(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (rd << 7) | 0x13U;
}

static uint64_t call_code_with_rax_arg(const uint8_t *code, const char *payload) {
  register uint64_t rax asm("rax") = (uint64_t) (uintptr_t) payload;
  asm volatile("call *%1"
    : "+a"(rax)
    : "r"(code)
    : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
  return rax;
}

static uint64_t call_code_with_poly3_args(const uint8_t *code,
    const void *arg0, const void *arg1, uint64_t arg2) {
  register uint64_t rax asm("rax") = (uint64_t) (uintptr_t) arg0;
  register uint64_t rdi asm("rdi") = (uint64_t) (uintptr_t) arg1;
  register uint64_t rsi asm("rsi") = arg2;
  asm volatile("call *%3"
    : "+a"(rax), "+D"(rdi), "+S"(rsi)
    : "r"(code)
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
  return rax;
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
    const uint8_t raw_switch[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, 0xd2800000U | ((uint32_t) LOOP_ITERS << 5)); // movz x0,#LOOP_ITERS
    emit_u32(code, &offset, 0xd1000400U); // sub x0,x0,#1
    emit_u32(code, &offset, 0xb5ffffe0U); // cbnz x0, previous instruction
    emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff
  } else {
    const uint8_t raw_switch[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, ((uint32_t) LOOP_ITERS << 20) | 0x00000513U); // addi a0,zero,LOOP_ITERS
    emit_u32(code, &offset, 0xfff50513U); // addi a0,a0,-1
    emit_u32(code, &offset, 0xfe051ee3U); // bne a0,zero, previous instruction
    emit_u32(code, &offset, 0x0000000bU); // custom-0 x86 escape
  }
  code[offset++] = 0xc3;

  poly_foreign_insn_count_status();
  uint64_t before = read_rax();
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
  poly_mode_x86();
  poly_foreign_insn_count_status();
  uint64_t after = read_rax();
  *insn_delta = after - before;

  munmap(code, code_size);
  return 0;
}

static int run_mixed_program(uint64_t *result, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 3 + 8 + 3 * 4 + 2 * 4 + 1;
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));
  emit_u32(code, &offset, 0xd2800140U); // movz x0,#10
  emit_u32(code, &offset, 0x91001400U); // add x0,x0,#5
  emit_u32(code, &offset, 0xd42fffc0U); // brk #0x7ffe, switch directly to RISC-V

  emit_u32(code, &offset, 0x01b50513U); // addi a0,a0,27
  emit_u32(code, &offset, 0x0000000bU); // custom-0 x86 escape
  code[offset++] = 0xc3;

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_reverse_mixed_program(uint64_t *result, uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 3 + 8 + 2 * 4 + 2 * 4 + 1;
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

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));
  emit_u32(code, &offset, 0x00700513U); // addi a0,zero,7
  emit_u32(code, &offset, 0x0000002bU); // custom-1, switch directly to AArch64
  emit_u32(code, &offset, 0x91008c00U); // add x0,x0,#35
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
  code[offset++] = 0xc3;

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_u32(code, &offset, 0xd2800280U); // movz x0,#20
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0x91000400U); // add x0,x0,#1
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01550513U); // addi a0,a0,21
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  emit_u32(code, &offset, 0x01400513U); // addi a0,zero,20
  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00150513U); // addi a0,a0,1
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
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
  store_u32(code, ld_return_offset, riscv_ld(6, 6,
    (int32_t) return_data_offset - (int32_t) auipc_return_pc));

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 4 + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 8 + 1;

  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
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

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
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
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
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
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0x1e602800U); // fadd d0,d0,d0
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
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

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
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
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x02a50553U); // fadd.d fa0,fa0,fa0
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
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
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0x910043ffU); // add sp,sp,#16
  emit_u32(code, &offset, 0x91003400U); // add x0,x0,#13
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
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
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
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
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x01010113U); // addi sp,sp,16
  emit_u32(code, &offset, 0x00d50513U); // addi a0,a0,13
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
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
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
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
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0x8b130000U); // add x0,x0,x19
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x01150513U); // addi a0,a0,17
  emit_u32(code, &offset, 0x00008067U); // ret

  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
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
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00850533U); // add a0,a0,s0
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
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
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 2 * 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0x8b010000U); // add x0,x0,x1
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
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
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x00b50533U); // add a0,a0,a1
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
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
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
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
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
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
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_libcall_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv libcall call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00100893U); // addi a7,zero,1
  emit_u32(code, &offset, 0x00100073U); // ebreak
  emit_u32(code, &offset, 0x00008067U); // ret

  static const char payload[] = "polyglot";
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t (*entry)(const char *) = (uint64_t (*)(const char *)) code;
  *result = entry(payload);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_libcall_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 256;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 libcall call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xd4200020U); // brk #1, strlen libcall
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
  uint64_t (*entry)(const char *) = (uint64_t (*)(const char *)) code;
  *result = entry(payload);
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_descriptor_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv descriptor call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00008413U); // addi s0,ra,0; save cross-return cookie
  const size_t auipc_descriptor_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_descriptor_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x000280e7U); // jalr ra,0(x5), descriptor strlen
  emit_u32(code, &offset, 0x00040093U); // addi ra,s0,0; restore cross-return cookie
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t descriptor_data_offset = offset;
  emit_u64(code, &offset,
    POLY_IMPORT_CALL_BASE + POLY_IMPORT_FUNC_STRLEN * POLY_IMPORT_CALL_STRIDE);

  store_u32(code, ld_descriptor_offset, riscv_ld(5, 5,
    (int32_t) descriptor_data_offset - (int32_t) auipc_descriptor_pc));

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

static int run_cross_call_descriptor_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 descriptor call mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xaa1e03f3U); // mov x19,x30; save cross-return cookie
  emit_aarch64_movabs(code, &offset, 17,
    POLY_IMPORT_CALL_BASE + POLY_IMPORT_FUNC_STRLEN * POLY_IMPORT_CALL_STRIDE);
  emit_u32(code, &offset, 0xd63f0220U); // blr x17, descriptor strlen
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

static int run_cross_call_descriptor_memcmp_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv descriptor memcmp mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
  code[offset++] = 0xc3;

  while (offset < riscv_target_offset)
    code[offset++] = 0x90;
  emit_u32(code, &offset, 0x00008413U); // addi s0,ra,0; save cross-return cookie
  const size_t auipc_descriptor_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_descriptor_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x000280e7U); // jalr ra,0(x5), descriptor memcmp
  emit_u32(code, &offset, 0x00040093U); // addi ra,s0,0; restore cross-return cookie
  emit_u32(code, &offset, 0x02a50513U); // addi a0,a0,42
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t descriptor_data_offset = offset;
  emit_u64(code, &offset,
    POLY_IMPORT_CALL_BASE + POLY_IMPORT_FUNC_MEMCMP * POLY_IMPORT_CALL_STRIDE);

  store_u32(code, ld_descriptor_offset, riscv_ld(5, 5,
    (int32_t) descriptor_data_offset - (int32_t) auipc_descriptor_pc));

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

static int run_cross_call_descriptor_memcmp_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 descriptor memcmp mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, 0xaa1e03f3U); // mov x19,x30; save cross-return cookie
  emit_aarch64_movabs(code, &offset, 17,
    POLY_IMPORT_CALL_BASE + POLY_IMPORT_FUNC_MEMCMP * POLY_IMPORT_CALL_STRIDE);
  emit_u32(code, &offset, 0xd63f0220U); // blr x17, descriptor memcmp
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

static int run_cross_call_descriptor_memops_aarch64_to_riscv(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: aarch64-to-riscv descriptor memops mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_aarch64, sizeof(raw_aarch64));

  const size_t aarch64_body_offset = offset;
  const size_t aarch64_return_offset = aarch64_body_offset + 16 + 16 + 4;
  const size_t riscv_target_offset = aarch64_return_offset + 4 + 1;

  emit_aarch64_movabs(code, &offset, 16,
    (uint64_t) (uintptr_t) (code + riscv_target_offset));
  emit_aarch64_movabs(code, &offset, 17,
    (uint64_t) (uintptr_t) (code + aarch64_return_offset));
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
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
  emit_u32(code, &offset, 0x000280e7U); // jalr ra,0(x5), descriptor memset
  emit_u32(code, &offset, riscv_addi(10, 19, 0)); // a0=s3
  emit_u32(code, &offset, riscv_addi(11, 9, 0));  // a1=s1
  emit_u32(code, &offset, riscv_addi(12, 18, 0)); // a2=s2
  const size_t auipc_memcpy_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_memcpy_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x000280e7U); // jalr ra,0(x5), descriptor memcpy
  emit_u32(code, &offset, riscv_addi(1, 8, 0)); // ra=s0
  emit_u32(code, &offset, 0x00008067U); // ret

  while ((offset & 7U) != 0)
    code[offset++] = 0;
  const size_t memset_data_offset = offset;
  emit_u64(code, &offset,
    POLY_IMPORT_CALL_BASE + POLY_IMPORT_FUNC_MEMSET * POLY_IMPORT_CALL_STRIDE);
  const size_t memcpy_data_offset = offset;
  emit_u64(code, &offset,
    POLY_IMPORT_CALL_BASE + POLY_IMPORT_FUNC_MEMCPY * POLY_IMPORT_CALL_STRIDE);

  store_u32(code, ld_memset_offset, riscv_ld(5, 5,
    (int32_t) memset_data_offset - (int32_t) auipc_memset_pc));
  store_u32(code, ld_memcpy_offset, riscv_ld(5, 5,
    (int32_t) memcpy_data_offset - (int32_t) auipc_memcpy_pc));

  static const char source[] = "polyglot";
  char dest[sizeof(source)];
  memset(dest, 0, sizeof(dest));
  poly_foreign_insn_count_status();
  uint64_t insns_before = read_rax();
  poly_switch_count_status();
  uint64_t switches_before = read_rax();
  uint64_t raw_result = call_code_with_poly3_args(code, dest, source, sizeof(source));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;
  *result = (raw_result == (uint64_t) (uintptr_t) dest &&
      memcmp(dest, source, sizeof(source)) == 0) ? 42 : 0;

  munmap(code, code_size);
  return 0;
}

static int run_cross_call_descriptor_memops_riscv_to_aarch64(uint64_t *result,
    uint64_t *insn_delta, uint64_t *switch_delta) {
  const size_t code_size = 512;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYBENCH_FAIL: riscv-to-aarch64 descriptor memops mmap failed: %s\n",
      strerror(errno));
    return -1;
  }

  size_t offset = 0;
  code[offset++] = 0x90;
  code[offset++] = 0x90;
  code[offset++] = 0x90;

  const uint8_t raw_riscv[] = { 0x0f, 0x24, 0x02, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
  emit_bytes(code, &offset, raw_riscv, sizeof(raw_riscv));

  const size_t auipc_target_pc = offset;
  emit_u32(code, &offset, 0x00000297U); // auipc x5,0
  const size_t ld_target_offset = offset;
  emit_u32(code, &offset, 0);
  const size_t auipc_return_pc = offset;
  emit_u32(code, &offset, 0x00000317U); // auipc x6,0
  const size_t ld_return_offset = offset;
  emit_u32(code, &offset, 0);
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
  const size_t riscv_return_offset = offset;
  emit_u32(code, &offset, 0x0000000bU); // custom-0, x86 escape
  code[offset++] = 0xc3;

  while ((offset & 3U) != 0)
    code[offset++] = 0x90;
  const size_t aarch64_target_offset = offset;
  emit_u32(code, &offset, aarch64_mov_reg(19, 30)); // save cross-return cookie
  emit_u32(code, &offset, aarch64_mov_reg(20, 1));  // save source
  emit_u32(code, &offset, aarch64_mov_reg(21, 2));  // save count
  emit_u32(code, &offset, aarch64_mov_reg(22, 0));  // save destination
  emit_u32(code, &offset, 0xd2800b01U); // movz x1,#'X'
  emit_aarch64_movabs(code, &offset, 17,
    POLY_IMPORT_CALL_BASE + POLY_IMPORT_FUNC_MEMSET * POLY_IMPORT_CALL_STRIDE);
  emit_u32(code, &offset, 0xd63f0220U); // blr x17, descriptor memset
  emit_u32(code, &offset, aarch64_mov_reg(0, 22));
  emit_u32(code, &offset, aarch64_mov_reg(1, 20));
  emit_u32(code, &offset, aarch64_mov_reg(2, 21));
  emit_aarch64_movabs(code, &offset, 17,
    POLY_IMPORT_CALL_BASE + POLY_IMPORT_FUNC_MEMCPY * POLY_IMPORT_CALL_STRIDE);
  emit_u32(code, &offset, 0xd63f0220U); // blr x17, descriptor memcpy
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
  uint64_t raw_result = call_code_with_poly3_args(code, dest, source, sizeof(source));
  poly_mode_x86();
  poly_foreign_insn_count_status();
  *insn_delta = read_rax() - insns_before;
  poly_switch_count_status();
  *switch_delta = read_rax() - switches_before;
  *result = (raw_result == (uint64_t) (uintptr_t) dest &&
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

  const uint8_t raw_aarch64[] = { 0x0f, 0x24, 0x01, 0x50, 0x4f, 0x4c, 0x59, 0x21 };
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
  emit_u32(code, &offset, 0xd42fffa0U); // brk #0x7ffd, call RISC-V
  emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff, x86 escape
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
  emit_u32(code, &offset, 0x0000005bU); // custom-2, call AArch64
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
  uint64_t (*entry)(void) = (uint64_t (*)(void)) code;
  *result = entry();
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
  return 0;
}

static int check_mixed(void) {
  if (check_mixed_direction("aarch64-to-riscv", run_mixed_program) < 0)
    return -1;
  if (check_mixed_direction("riscv-to-aarch64", run_reverse_mixed_program) < 0)
    return -1;
  return 0;
}

static int check_cross_call_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
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
  return 0;
}

static int check_cross_call_libcall_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_LIBCALL_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call libcall %s strlen expected 8 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 8) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call libcall %s raw instruction delta expected at least 8 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call libcall %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  return 0;
}

static int check_cross_call_descriptor_memcmp_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_DESCRIPTOR_MEMCMP_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call descriptor memcmp %s expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 10) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call descriptor memcmp %s raw instruction delta expected at least 10 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call descriptor memcmp %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  return 0;
}

static int check_cross_call_descriptor_memops_direction(const char *name,
    int (*runner)(uint64_t *, uint64_t *, uint64_t *)) {
  uint64_t result = 0;
  uint64_t insn_delta = 0;
  uint64_t switch_delta = 0;
  if (runner(&result, &insn_delta, &switch_delta) < 0)
    return -1;

  printf("POLYBENCH_CROSS_CALL_DESCRIPTOR_MEMOPS_RESULT: direction=%s result=%llu raw_insn_delta=%llu switch_delta=%llu\n",
    name, (unsigned long long) result, (unsigned long long) insn_delta,
    (unsigned long long) switch_delta);

  if (result != 42) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call descriptor memops %s expected 42 got %llu\n",
      name, (unsigned long long) result);
    return -1;
  }
  if (insn_delta < 15) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call descriptor memops %s raw instruction delta expected at least 15 got %llu\n",
      name, (unsigned long long) insn_delta);
    return -1;
  }
  if (switch_delta < 4) {
    fprintf(stderr, "POLYBENCH_FAIL: cross call descriptor memops %s switch delta expected at least 4 got %llu\n",
      name, (unsigned long long) switch_delta);
    return -1;
  }
  return 0;
}

static int check_cross_calls(void) {
  if (check_cross_call_direction("aarch64-calls-riscv",
        run_cross_call_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_direction("riscv-calls-aarch64",
        run_cross_call_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_direction("nested-aarch64-riscv-aarch64",
        run_nested_cross_call) < 0)
    return -1;
  if (check_cross_call_fp_direction("aarch64-calls-riscv-fp",
        run_cross_call_fp_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_fp_direction("riscv-calls-aarch64-fp",
        run_cross_call_fp_riscv_to_aarch64) < 0)
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
  if (check_cross_call_pair_direction("aarch64-calls-riscv-pair",
        run_cross_call_pair_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_pair_direction("riscv-calls-aarch64-pair",
        run_cross_call_pair_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_syscall_direction("aarch64-calls-riscv-syscall",
        run_cross_call_syscall_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_syscall_direction("riscv-calls-aarch64-syscall",
        run_cross_call_syscall_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_libcall_direction("aarch64-calls-riscv-libcall",
        run_cross_call_libcall_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_libcall_direction("riscv-calls-aarch64-libcall",
        run_cross_call_libcall_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_libcall_direction("aarch64-calls-riscv-descriptor",
        run_cross_call_descriptor_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_libcall_direction("riscv-calls-aarch64-descriptor",
        run_cross_call_descriptor_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_descriptor_memcmp_direction("aarch64-calls-riscv-descriptor-memcmp",
        run_cross_call_descriptor_memcmp_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_descriptor_memcmp_direction("riscv-calls-aarch64-descriptor-memcmp",
        run_cross_call_descriptor_memcmp_riscv_to_aarch64) < 0)
    return -1;
  if (check_cross_call_descriptor_memops_direction("aarch64-calls-riscv-descriptor-memops",
        run_cross_call_descriptor_memops_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_descriptor_memops_direction("riscv-calls-aarch64-descriptor-memops",
        run_cross_call_descriptor_memops_riscv_to_aarch64) < 0)
    return -1;
  return 0;
}

int main(void) {
  puts("POLYBENCH: start");
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
