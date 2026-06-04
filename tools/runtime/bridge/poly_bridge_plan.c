#include "poly_bridge_plan.h"

static int valid_arch(enum poly_abi_arch arch) {
  return arch == POLY_ABI_ARCH_X86 ||
    arch == POLY_ABI_ARCH_AARCH64 ||
    arch == POLY_ABI_ARCH_RISCV;
}

static int valid_loc(const struct poly_abi_loc *loc) {
  switch (loc->kind) {
    case POLY_ABI_LOC_NONE:
      return loc->reg == 0 && loc->stack_offset == 0 && loc->size == 0;
    case POLY_ABI_LOC_GPR:
    case POLY_ABI_LOC_FPR:
    case POLY_ABI_LOC_VEC:
    case POLY_ABI_LOC_SRET_PTR:
      return loc->size != 0;
    case POLY_ABI_LOC_STACK:
      return loc->size != 0;
    default:
      return 0;
  }
}

static int valid_op_locs(const struct poly_bridge_op *op) {
  switch (op->kind) {
    case POLY_BRIDGE_MOVE_REG_TO_REG:
      return op->src.kind != POLY_ABI_LOC_STACK &&
        op->src.kind != POLY_ABI_LOC_NONE &&
        op->dst.kind != POLY_ABI_LOC_STACK &&
        op->dst.kind != POLY_ABI_LOC_NONE;
    case POLY_BRIDGE_MOVE_REG_TO_STACK:
      return op->src.kind != POLY_ABI_LOC_STACK &&
        op->src.kind != POLY_ABI_LOC_NONE &&
        op->dst.kind == POLY_ABI_LOC_STACK;
    case POLY_BRIDGE_MOVE_STACK_TO_REG:
      return op->src.kind == POLY_ABI_LOC_STACK &&
        op->dst.kind != POLY_ABI_LOC_STACK &&
        op->dst.kind != POLY_ABI_LOC_NONE;
    case POLY_BRIDGE_MOVE_STACK_TO_STACK:
      return op->src.kind == POLY_ABI_LOC_STACK &&
        op->dst.kind == POLY_ABI_LOC_STACK;
    case POLY_BRIDGE_MOVE_LOAD_IMM:
      return op->src.kind == POLY_ABI_LOC_NONE &&
        op->dst.kind != POLY_ABI_LOC_NONE;
    case POLY_BRIDGE_MOVE_SAVE_CALLER_REG:
    case POLY_BRIDGE_MOVE_RESTORE_CALLER_REG:
      return op->src.kind != POLY_ABI_LOC_NONE ||
        op->dst.kind != POLY_ABI_LOC_NONE;
    case POLY_BRIDGE_MOVE_SET_STATE_KEY:
    case POLY_BRIDGE_MOVE_SET_SIGNATURE_SLOT:
    case POLY_BRIDGE_MOVE_PCALL:
      return op->src.kind == POLY_ABI_LOC_NONE &&
        op->dst.kind == POLY_ABI_LOC_NONE;
    default:
      return 0;
  }
}

int poly_bridge_plan_add_op(struct poly_bridge_plan *plan,
    enum poly_bridge_move_kind kind, struct poly_abi_loc src,
    struct poly_abi_loc dst, uint64_t imm) {
  if (plan == 0 || plan->op_count >= POLY_BRIDGE_PLAN_MAX_OPS)
    return -1;
  struct poly_bridge_op *op = &plan->ops[plan->op_count++];
  op->kind = kind;
  op->src = src;
  op->dst = dst;
  op->imm = imm;
  return 0;
}

int poly_bridge_plan_validate(const struct poly_bridge_plan *plan) {
  if (plan == 0 || !valid_arch(plan->caller) || !valid_arch(plan->callee))
    return -1;
  if (plan->op_count > POLY_BRIDGE_PLAN_MAX_OPS)
    return -1;
  for (size_t n = 0; n < plan->op_count; n++) {
    const struct poly_bridge_op *op = &plan->ops[n];
    if (!valid_loc(&op->src) || !valid_loc(&op->dst) || !valid_op_locs(op))
      return -1;
  }
  return 0;
}
