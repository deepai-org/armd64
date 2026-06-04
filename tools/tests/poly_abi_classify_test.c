#include <stdio.h>
#include <string.h>

#include "../runtime/abi/poly_abi_classify.h"

static void add_scalar_arg(struct poly_function_sig *sig,
    enum poly_abi_type_kind kind) {
  sig->args[sig->arg_count++] = poly_abi_scalar_type(kind);
}

static int expect_loc(const char *name, const struct poly_abi_loc *actual,
    struct poly_abi_loc expected) {
  if (!poly_abi_loc_equal(actual, &expected)) {
    fprintf(stderr,
      "%s loc mismatch kind=%d reg=%u stack=%u size=%u expected kind=%d reg=%u stack=%u size=%u\n",
      name, actual->kind, actual->reg, actual->stack_offset, actual->size,
      expected.kind, expected.reg, expected.stack_offset, expected.size);
    return 1;
  }
  return 0;
}

static int check_scalar_regs(void) {
  struct poly_function_sig sig = { 0 };
  add_scalar_arg(&sig, POLY_ABI_TYPE_I64);
  add_scalar_arg(&sig, POLY_ABI_TYPE_F64);
  sig.ret = poly_abi_scalar_type(POLY_ABI_TYPE_F64);

  struct poly_abi_layout layout;
  if (poly_classify_call_abi(POLY_ABI_ARCH_X86, &sig, &layout) != 0)
    return 1;
  if (layout.arg_count != 2 || layout.ret_count != 1)
    return 1;
  if (expect_loc("x86 arg0", &layout.args[0].locs[0],
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 0, 8)) != 0 ||
      expect_loc("x86 arg1", &layout.args[1].locs[0],
        poly_abi_loc_reg(POLY_ABI_LOC_FPR, 0, 8)) != 0 ||
      expect_loc("x86 ret", &layout.ret[0],
        poly_abi_loc_reg(POLY_ABI_LOC_FPR, 0, 8)) != 0)
    return 1;

  if (poly_classify_call_abi(POLY_ABI_ARCH_AARCH64, &sig, &layout) != 0)
    return 1;
  if (expect_loc("aarch64 arg0", &layout.args[0].locs[0],
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 0, 8)) != 0 ||
      expect_loc("aarch64 arg1", &layout.args[1].locs[0],
        poly_abi_loc_reg(POLY_ABI_LOC_FPR, 0, 8)) != 0 ||
      expect_loc("aarch64 ret", &layout.ret[0],
        poly_abi_loc_reg(POLY_ABI_LOC_FPR, 0, 8)) != 0)
    return 1;

  if (poly_classify_call_abi(POLY_ABI_ARCH_RISCV, &sig, &layout) != 0)
    return 1;
  return expect_loc("riscv ret", &layout.ret[0],
    poly_abi_loc_reg(POLY_ABI_LOC_FPR, 0, 8));
}

static int check_aarch64_hfa(void) {
  struct poly_function_sig sig = { 0 };
  sig.args[sig.arg_count++] = poly_abi_hfa_type(POLY_ABI_TYPE_F64, 3);
  sig.ret = poly_abi_hfa_type(POLY_ABI_TYPE_F64, 3);

  struct poly_abi_layout layout;
  if (poly_classify_call_abi(POLY_ABI_ARCH_AARCH64, &sig, &layout) != 0)
    return 1;
  if (layout.args[0].loc_count != 3 || layout.ret_count != 3)
    return 1;
  for (uint8_t n = 0; n < 3; n++) {
    char name[32];
    snprintf(name, sizeof(name), "aarch64 hfa part %u", n);
    if (expect_loc(name, &layout.args[0].locs[n],
          poly_abi_loc_reg(POLY_ABI_LOC_FPR, n, 8)) != 0 ||
        expect_loc(name, &layout.ret[n],
          poly_abi_loc_reg(POLY_ABI_LOC_FPR, n, 8)) != 0)
      return 1;
  }
  return 0;
}

static int check_stack_arg(void) {
  struct poly_function_sig sig = { 0 };
  for (uint8_t n = 0; n < 9; n++)
    add_scalar_arg(&sig, POLY_ABI_TYPE_I64);
  sig.ret = poly_abi_scalar_type(POLY_ABI_TYPE_I64);

  struct poly_abi_layout layout;
  if (poly_classify_call_abi(POLY_ABI_ARCH_X86, &sig, &layout) != 0)
    return 1;
  if (layout.args[8].loc_count != 1 || layout.stack_arg_size != 24)
    return 1;
  return expect_loc("x86 ninth i64", &layout.args[8].locs[0],
    poly_abi_loc_stack(16, 8));
}

static int check_sret(void) {
  struct poly_function_sig sig = { 0 };
  sig.ret.kind = POLY_ABI_TYPE_AGGREGATE;
  sig.ret.size = 32;
  sig.ret.align = 8;
  sig.flags = POLY_FUNCTION_SIG_FLAG_RET_BY_SRET;
  add_scalar_arg(&sig, POLY_ABI_TYPE_I64);

  struct poly_abi_layout layout;
  if (poly_classify_call_abi(POLY_ABI_ARCH_X86, &sig, &layout) != 0)
    return 1;
  if (!layout.uses_sret || layout.ret_count != 1)
    return 1;
  if (expect_loc("x86 sret", &layout.ret[0],
        poly_abi_loc_sret_ptr(0, 32)) != 0 ||
      expect_loc("x86 first user arg after sret", &layout.args[0].locs[0],
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 1, 8)) != 0)
    return 1;

  if (poly_classify_call_abi(POLY_ABI_ARCH_AARCH64, &sig, &layout) != 0)
    return 1;
  if (!layout.uses_sret)
    return 1;
  return expect_loc("aarch64 sret", &layout.ret[0],
    poly_abi_loc_sret_ptr(8, 32));
}

int main(void) {
  if (check_scalar_regs() != 0 ||
      check_aarch64_hfa() != 0 ||
      check_stack_arg() != 0 ||
      check_sret() != 0)
    return 1;
  printf("poly ABI classifiers OK\n");
  return 0;
}
