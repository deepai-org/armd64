#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
  LOOP_ITERS = 200
};

static inline void poly_mode_x86(void) { asm volatile(".byte 0x64,0x0f,0x0b,0x58,0x4d,0x4f,0x44,0x45" ::: "memory"); }
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

static uint32_t riscv_ld(uint32_t rd, uint32_t rs1, int32_t imm) {
  return (((uint32_t) imm & 0xfffU) << 20) |
    (rs1 << 15) | (0x3U << 12) | (rd << 7) | 0x03U;
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
    const uint8_t raw_switch[] = { 0x65, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x36, 0x34 };
    memcpy(code + offset, raw_switch, sizeof(raw_switch));
    offset += sizeof(raw_switch);
    emit_u32(code, &offset, 0xd2800000U | ((uint32_t) LOOP_ITERS << 5)); // movz x0,#LOOP_ITERS
    emit_u32(code, &offset, 0xd1000400U); // sub x0,x0,#1
    emit_u32(code, &offset, 0xb5ffffe0U); // cbnz x0, previous instruction
    emit_u32(code, &offset, 0xd42fffe0U); // brk #0x7fff
  } else {
    const uint8_t raw_switch[] = { 0x66, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x52, 0x56 };
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

  const uint8_t raw_aarch64[] = { 0x65, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x36, 0x34 };
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

  const uint8_t raw_riscv[] = { 0x66, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x52, 0x56 };
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

  const uint8_t raw_aarch64[] = { 0x65, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x36, 0x34 };
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

  const uint8_t raw_riscv[] = { 0x66, 0x0f, 0x0b, 0x52, 0x41, 0x57, 0x52, 0x56 };
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

static int check_cross_calls(void) {
  if (check_cross_call_direction("aarch64-calls-riscv",
        run_cross_call_aarch64_to_riscv) < 0)
    return -1;
  if (check_cross_call_direction("riscv-calls-aarch64",
        run_cross_call_riscv_to_aarch64) < 0)
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
