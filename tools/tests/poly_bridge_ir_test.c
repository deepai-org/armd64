#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../runtime/abi/poly_abi_descriptor.h"
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

static int check_descriptor_fp64_plan(void) {
  struct poly_bridge_plan plan;
  if (poly_bridge_plan_from_descriptor_kind(POLY_ABI_ARCH_X86,
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

static int check_descriptor_stack_arg_plan(void) {
  struct poly_bridge_plan plan;
  if (poly_bridge_plan_from_descriptor_kind(POLY_ABI_ARCH_X86,
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

static char trace_code(enum poly_bridge_move_kind kind) {
  switch (kind) {
    case POLY_BRIDGE_MOVE_REG_TO_REG: return 'R';
    case POLY_BRIDGE_MOVE_REG_TO_STACK: return 'D';
    case POLY_BRIDGE_MOVE_STACK_TO_REG: return 'U';
    case POLY_BRIDGE_MOVE_STACK_TO_STACK: return 'S';
    case POLY_BRIDGE_MOVE_LOAD_IMM: return 'I';
    case POLY_BRIDGE_MOVE_SAVE_CALLER_REG: return 'V';
    case POLY_BRIDGE_MOVE_RESTORE_CALLER_REG: return 'W';
    case POLY_BRIDGE_MOVE_SET_STATE_KEY: return 'K';
    case POLY_BRIDGE_MOVE_SET_SIGNATURE_SLOT: return 'T';
    case POLY_BRIDGE_MOVE_PCALL: return 'P';
    default: return '?';
  }
}

static int expect_plan_trace(int bridge_kind, enum poly_abi_arch caller,
    enum poly_abi_arch callee, const char *expected) {
  struct poly_bridge_plan plan;
  char actual[POLY_BRIDGE_PLAN_MAX_OPS + 1];
  if (poly_bridge_plan_from_descriptor_kind(caller, callee, bridge_kind,
        POLY_BRIDGE_PLAN_NO_SIGNATURE_SLOT, 0, &plan) != 0) {
    fprintf(stderr, "bridge plan trace failed kind=%d caller=%d callee=%d\n",
      bridge_kind, caller, callee);
    return 1;
  }
  for (size_t n = 0; n < plan.op_count; n++)
    actual[n] = trace_code(plan.ops[n].kind);
  actual[plan.op_count] = '\0';
  if (strcmp(actual, expected) != 0) {
    fprintf(stderr,
      "bridge plan trace mismatch kind=%d caller=%d callee=%d expected=%s actual=%s\n",
      bridge_kind, caller, callee, expected, actual);
    return 1;
  }
  return 0;
}

static int check_representative_plan_traces(void) {
  if (expect_plan_trace(POLY_PROCESS_BRIDGE_U64_STACK9,
        POLY_ABI_ARCH_X86, POLY_ABI_ARCH_AARCH64, "RRRRRRUUSPR") != 0)
    return 1;
  if (expect_plan_trace(POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET,
        POLY_ABI_ARCH_AARCH64, POLY_ABI_ARCH_RISCV, "RRRPRRR") != 0)
    return 1;
  if (expect_plan_trace(POLY_PROCESS_BRIDGE_FPAIR64_RET,
        POLY_ABI_ARCH_X86, POLY_ABI_ARCH_AARCH64, "RRRPRR") != 0)
    return 1;
  return 0;
}

struct descriptor_plan_expect {
  uint8_t ok;
  uint8_t op_count;
};

static int check_descriptor_plan_matrix(void) {
  const enum poly_abi_arch archs[] = {
    POLY_ABI_ARCH_X86,
    POLY_ABI_ARCH_AARCH64,
    POLY_ABI_ARCH_RISCV
  };
  const struct descriptor_plan_expect expected[POLY_PROCESS_BRIDGE_KIND_COUNT][6] = {
    { { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 } },
    { { 1, 4 }, { 1, 4 }, { 1, 4 }, { 1, 4 }, { 1, 4 }, { 1, 4 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 4 }, { 0, 0 }, { 1, 4 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 4 }, { 0, 0 }, { 1, 4 } },
    { { 1, 11 }, { 1, 11 }, { 1, 11 }, { 1, 11 }, { 1, 11 }, { 1, 11 } },
    { { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 } },
    { { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 }, { 1, 2 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 7 }, { 0, 0 }, { 1, 7 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 9 }, { 0, 0 }, { 1, 9 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 4 }, { 0, 0 }, { 1, 4 } },
    { { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 5 }, { 0, 0 }, { 1, 5 } },
    { { 1, 6 }, { 1, 6 }, { 1, 6 }, { 1, 6 }, { 1, 6 }, { 1, 6 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 6 }, { 0, 0 }, { 1, 6 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 7 }, { 0, 0 }, { 1, 7 } },
    { { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 } },
    { { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 7 }, { 0, 0 }, { 1, 7 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 9 }, { 0, 0 }, { 1, 9 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 6 }, { 0, 0 }, { 1, 6 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 7 }, { 0, 0 }, { 1, 7 } }
  };

  for (int kind = 0; kind < POLY_PROCESS_BRIDGE_KIND_COUNT; kind++) {
    size_t pair = 0;
    for (size_t caller = 0; caller < sizeof(archs) / sizeof(archs[0]);
        caller++) {
      for (size_t callee = 0; callee < sizeof(archs) / sizeof(archs[0]);
          callee++) {
        if (caller == callee)
          continue;
        struct poly_bridge_plan plan;
        const int rc = poly_bridge_plan_from_descriptor_kind(archs[caller],
          archs[callee], kind, POLY_BRIDGE_PLAN_NO_SIGNATURE_SLOT, 0, &plan);
        const struct descriptor_plan_expect want = expected[kind][pair++];
        if (want.ok) {
          if (rc != 0 || plan.op_count != want.op_count) {
            fprintf(stderr,
              "descriptor plan matrix mismatch kind=%d name=%s caller=%d callee=%d expected_ok=1 expected_ops=%u rc=%d actual_ops=%zu\n",
              kind, poly_abi_descriptor_kind_name(kind), archs[caller],
              archs[callee], want.op_count, rc, rc == 0 ? plan.op_count : 0);
            return 1;
          }
        } else if (rc == 0) {
          fprintf(stderr,
            "descriptor plan matrix mismatch kind=%d name=%s caller=%d callee=%d expected_ok=0 actual_ops=%zu\n",
            kind, poly_abi_descriptor_kind_name(kind), archs[caller],
            archs[callee], plan.op_count);
          return 1;
        }
      }
    }
  }
  return 0;
}

int main(void) {
  if (expect_ok(check_layout_helpers(), "layout helper check") != 0)
    return 1;
  if (expect_ok(check_bridge_plan(), "bridge plan check") != 0)
    return 1;
  if (expect_fail(check_invalid_plan(), "invalid bridge plan check") != 0)
    return 1;
  if (expect_ok(check_descriptor_fp64_plan(), "descriptor fp64 plan check") != 0)
    return 1;
  if (expect_ok(check_descriptor_stack_arg_plan(),
        "descriptor stack arg plan check") != 0)
    return 1;
  if (expect_ok(check_representative_plan_traces(),
        "representative bridge plan trace check") != 0)
    return 1;
  if (expect_ok(check_descriptor_plan_matrix(),
        "ABI descriptor plan matrix check") != 0)
    return 1;
  return 0;
}
