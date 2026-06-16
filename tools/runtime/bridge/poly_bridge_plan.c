#include "poly_bridge_plan.h"

#include "../abi/poly_abi_classify.h"
#include "../abi/poly_abi_descriptor.h"

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

static int loc_is_register_like(const struct poly_abi_loc *loc) {
  return loc->kind == POLY_ABI_LOC_GPR ||
    loc->kind == POLY_ABI_LOC_FPR ||
    loc->kind == POLY_ABI_LOC_VEC ||
    loc->kind == POLY_ABI_LOC_SRET_PTR;
}

static int add_move_for_locs(struct poly_bridge_plan *plan,
    struct poly_abi_loc src, struct poly_abi_loc dst) {
  if (src.size != dst.size)
    return -1;
  if (loc_is_register_like(&src) && loc_is_register_like(&dst))
    return poly_bridge_plan_add_op(plan, POLY_BRIDGE_MOVE_REG_TO_REG,
      src, dst, 0);
  if (loc_is_register_like(&src) && dst.kind == POLY_ABI_LOC_STACK)
    return poly_bridge_plan_add_op(plan, POLY_BRIDGE_MOVE_REG_TO_STACK,
      src, dst, 0);
  if (src.kind == POLY_ABI_LOC_STACK && loc_is_register_like(&dst))
    return poly_bridge_plan_add_op(plan, POLY_BRIDGE_MOVE_STACK_TO_REG,
      src, dst, 0);
  if (src.kind == POLY_ABI_LOC_STACK && dst.kind == POLY_ABI_LOC_STACK)
    return poly_bridge_plan_add_op(plan, POLY_BRIDGE_MOVE_STACK_TO_STACK,
      src, dst, 0);
  return -1;
}

static int add_arg_moves(struct poly_bridge_plan *plan,
    const struct poly_abi_layout *caller_layout,
    const struct poly_abi_layout *callee_layout) {
  if (caller_layout->arg_count != callee_layout->arg_count)
    return -1;
  for (size_t arg = 0; arg < caller_layout->arg_count; arg++) {
    const struct poly_abi_layout_arg *src_arg = &caller_layout->args[arg];
    const struct poly_abi_layout_arg *dst_arg = &callee_layout->args[arg];
    if (src_arg->loc_count != dst_arg->loc_count)
      return -1;
    for (size_t loc = 0; loc < src_arg->loc_count; loc++) {
      if (add_move_for_locs(plan, src_arg->locs[loc],
            dst_arg->locs[loc]) != 0)
        return -1;
    }
  }
  return 0;
}

static int add_ret_moves(struct poly_bridge_plan *plan,
    const struct poly_abi_layout *caller_layout,
    const struct poly_abi_layout *callee_layout) {
  if (caller_layout->ret_count != callee_layout->ret_count)
    return -1;
  for (size_t ret = 0; ret < caller_layout->ret_count; ret++) {
    if (add_move_for_locs(plan, callee_layout->ret[ret],
          caller_layout->ret[ret]) != 0)
      return -1;
  }
  return 0;
}

int poly_bridge_plan_from_layouts(const struct poly_abi_layout *caller_layout,
    const struct poly_abi_layout *callee_layout, uint32_t signature_slot,
    int needs_state_key, struct poly_bridge_plan *plan) {
  const struct poly_abi_loc none = poly_abi_loc_none();
  if (caller_layout == 0 || callee_layout == 0 || plan == 0 ||
      !valid_arch(caller_layout->arch) || !valid_arch(callee_layout->arch))
    return -1;
  poly_bridge_plan_init(plan, caller_layout->arch, callee_layout->arch);
  plan->signature_slot = signature_slot;
  plan->needs_state_key = needs_state_key != 0;

  if (plan->needs_state_key &&
      poly_bridge_plan_add_op(plan, POLY_BRIDGE_MOVE_SET_STATE_KEY,
        none, none, 0) != 0)
    return -1;
  if (signature_slot != POLY_BRIDGE_PLAN_NO_SIGNATURE_SLOT &&
      poly_bridge_plan_add_op(plan, POLY_BRIDGE_MOVE_SET_SIGNATURE_SLOT,
        none, none, signature_slot) != 0)
    return -1;
  if (add_arg_moves(plan, caller_layout, callee_layout) != 0)
    return -1;
  if (poly_bridge_plan_add_op(plan, POLY_BRIDGE_MOVE_PCALL,
        none, none, 0) != 0)
    return -1;
  if (add_ret_moves(plan, caller_layout, callee_layout) != 0)
    return -1;
  return poly_bridge_plan_validate(plan);
}

int poly_bridge_plan_from_descriptor_kind(enum poly_abi_arch caller,
    enum poly_abi_arch callee, int bridge_kind, uint32_t signature_slot,
    int needs_state_key, struct poly_bridge_plan *plan) {
  struct poly_function_sig sig;
  struct poly_abi_layout caller_layout;
  struct poly_abi_layout callee_layout;
  if (poly_abi_descriptor_decode_kind(bridge_kind, &sig) != 0 ||
      poly_classify_call_abi(caller, &sig, &caller_layout) != 0 ||
      poly_classify_call_abi(callee, &sig, &callee_layout) != 0)
    return -1;
  return poly_bridge_plan_from_layouts(&caller_layout, &callee_layout,
    signature_slot, needs_state_key, plan);
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
