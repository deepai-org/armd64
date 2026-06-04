#include <stdint.h>
#include <stdio.h>

#include "../runtime/bridge/poly_bridge_plan.h"
#include "../runtime/bridge/poly_process_bridge_kind.h"

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

static int check_legacy_fp64_plan(void) {
  struct poly_bridge_plan plan;
  if (poly_bridge_plan_from_legacy_kind(POLY_ABI_ARCH_X86,
        POLY_ABI_ARCH_AARCH64, POLY_PROCESS_BRIDGE_FP64, 5, 1, &plan) != 0)
    return 1;
  if (plan.op_count != 4 ||
      plan.signature_slot != 5 ||
      plan.needs_state_key != 1 ||
      plan.ops[0].kind != POLY_BRIDGE_MOVE_SET_STATE_KEY ||
      plan.ops[1].kind != POLY_BRIDGE_MOVE_SET_SIGNATURE_SLOT ||
      plan.ops[2].kind != POLY_BRIDGE_MOVE_PCALL ||
      plan.ops[3].kind != POLY_BRIDGE_MOVE_REG_TO_REG)
    return 1;
  return !poly_abi_loc_equal(&plan.ops[3].src,
      &(struct poly_abi_loc) {
        .kind = POLY_ABI_LOC_FPR,
        .reg = 0,
        .size = 8
      }) ||
    !poly_abi_loc_equal(&plan.ops[3].dst,
      &(struct poly_abi_loc) {
        .kind = POLY_ABI_LOC_FPR,
        .reg = 0,
        .size = 8
      });
}

static int check_legacy_stack_arg_plan(void) {
  struct poly_bridge_plan plan;
  if (poly_bridge_plan_from_legacy_kind(POLY_ABI_ARCH_X86,
        POLY_ABI_ARCH_AARCH64, POLY_PROCESS_BRIDGE_U64_STACK9,
        POLY_BRIDGE_PLAN_NO_SIGNATURE_SLOT, 0, &plan) != 0)
    return 1;
  if (plan.op_count != 11 ||
      plan.ops[6].kind != POLY_BRIDGE_MOVE_STACK_TO_REG ||
      plan.ops[7].kind != POLY_BRIDGE_MOVE_STACK_TO_REG ||
      plan.ops[8].kind != POLY_BRIDGE_MOVE_STACK_TO_STACK ||
      plan.ops[9].kind != POLY_BRIDGE_MOVE_PCALL ||
      plan.ops[10].kind != POLY_BRIDGE_MOVE_REG_TO_REG)
    return 1;
  if (!poly_abi_loc_equal(&plan.ops[6].src,
        &(struct poly_abi_loc) {
          .kind = POLY_ABI_LOC_STACK,
          .stack_offset = 0,
          .size = 8
        }) ||
      !poly_abi_loc_equal(&plan.ops[6].dst,
        &(struct poly_abi_loc) {
          .kind = POLY_ABI_LOC_GPR,
          .reg = 6,
          .size = 8
        }) ||
      !poly_abi_loc_equal(&plan.ops[7].src,
        &(struct poly_abi_loc) {
          .kind = POLY_ABI_LOC_STACK,
          .stack_offset = 8,
          .size = 8
        }) ||
      !poly_abi_loc_equal(&plan.ops[7].dst,
        &(struct poly_abi_loc) {
          .kind = POLY_ABI_LOC_GPR,
          .reg = 7,
          .size = 8
        }))
    return 1;
  return !poly_abi_loc_equal(&plan.ops[8].src,
      &(struct poly_abi_loc) {
        .kind = POLY_ABI_LOC_STACK,
        .stack_offset = 16,
        .size = 8
      }) ||
    !poly_abi_loc_equal(&plan.ops[8].dst,
      &(struct poly_abi_loc) {
        .kind = POLY_ABI_LOC_STACK,
        .stack_offset = 0,
        .size = 8
      });
}

int main(void) {
  if (expect_ok(check_layout_helpers(), "layout helper check") != 0)
    return 1;
  if (expect_ok(check_bridge_plan(), "bridge plan check") != 0)
    return 1;
  if (expect_fail(check_invalid_plan(), "invalid bridge plan check") != 0)
    return 1;
  if (expect_ok(check_legacy_fp64_plan(), "legacy fp64 plan check") != 0)
    return 1;
  if (expect_ok(check_legacy_stack_arg_plan(),
        "legacy stack arg plan check") != 0)
    return 1;
  return 0;
}
