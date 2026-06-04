#include <stdint.h>
#include <stdio.h>

#include "../runtime/bridge/poly_bridge_plan.h"

static int expect_ok(int value, const char *what) {
  if (value == 0)
    return 0;
  fprintf(stderr, "%s failed\n", what);
  return 1;
}

static int expect_fail(int value, const char *what) {
  if (value != 0)
    return 0;
  fprintf(stderr, "%s unexpectedly succeeded\n", what);
  return 1;
}

static int check_layout_helpers(void) {
  struct poly_abi_layout layout;
  poly_abi_layout_init(&layout, POLY_ABI_ARCH_AARCH64);
  if (poly_abi_layout_add_arg_loc(&layout, 0,
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 0, 8)) != 0)
    return 1;
  if (poly_abi_layout_add_arg_loc(&layout, 0,
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 1, 8)) != 0)
    return 1;
  if (poly_abi_layout_add_ret_loc(&layout,
        poly_abi_loc_reg(POLY_ABI_LOC_FPR, 0, 8)) != 0)
    return 1;
  if (layout.arch != POLY_ABI_ARCH_AARCH64 ||
      layout.arg_count != 1 ||
      layout.args[0].loc_count != 2 ||
      layout.ret_count != 1 ||
      layout.stack_align != 16)
    return 1;
  if (!poly_abi_loc_equal(&layout.args[0].locs[1],
        &(struct poly_abi_loc) {
          .kind = POLY_ABI_LOC_GPR,
          .reg = 1,
          .size = 8
        }))
    return 1;
  return 0;
}

static int check_bridge_plan(void) {
  struct poly_bridge_plan plan;
  const struct poly_abi_loc none = poly_abi_loc_none();
  poly_bridge_plan_init(&plan, POLY_ABI_ARCH_X86, POLY_ABI_ARCH_AARCH64);
  plan.signature_slot = 7;
  plan.needs_state_key = 1;

  if (poly_bridge_plan_add_op(&plan, POLY_BRIDGE_MOVE_SET_STATE_KEY,
        none, none, 0x1234) != 0)
    return 1;
  if (poly_bridge_plan_add_op(&plan, POLY_BRIDGE_MOVE_REG_TO_REG,
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 0, 8),
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 1, 8), 0) != 0)
    return 1;
  if (poly_bridge_plan_add_op(&plan, POLY_BRIDGE_MOVE_REG_TO_STACK,
        poly_abi_loc_reg(POLY_ABI_LOC_FPR, 0, 8),
        poly_abi_loc_stack(16, 8), 0) != 0)
    return 1;
  if (poly_bridge_plan_add_op(&plan, POLY_BRIDGE_MOVE_SET_SIGNATURE_SLOT,
        none, none, plan.signature_slot) != 0)
    return 1;
  if (poly_bridge_plan_add_op(&plan, POLY_BRIDGE_MOVE_PCALL,
        none, none, 0) != 0)
    return 1;

  if (plan.op_count != 5 ||
      plan.caller != POLY_ABI_ARCH_X86 ||
      plan.callee != POLY_ABI_ARCH_AARCH64)
    return 1;
  return poly_bridge_plan_validate(&plan);
}

static int check_invalid_plan(void) {
  struct poly_bridge_plan plan;
  poly_bridge_plan_init(&plan, POLY_ABI_ARCH_X86, POLY_ABI_ARCH_RISCV);
  if (poly_bridge_plan_add_op(&plan, POLY_BRIDGE_MOVE_STACK_TO_REG,
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 0, 8),
        poly_abi_loc_reg(POLY_ABI_LOC_GPR, 1, 8), 0) != 0)
    return 1;
  return poly_bridge_plan_validate(&plan);
}

int main(void) {
  if (expect_ok(check_layout_helpers(), "layout helper check") != 0)
    return 1;
  if (expect_ok(check_bridge_plan(), "bridge plan check") != 0)
    return 1;
  if (expect_fail(check_invalid_plan(), "invalid bridge plan check") != 0)
    return 1;
  return 0;
}
