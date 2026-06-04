#ifndef POLY_BRIDGE_PLAN_H
#define POLY_BRIDGE_PLAN_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../abi/poly_abi_layout.h"

enum poly_bridge_move_kind {
  POLY_BRIDGE_MOVE_REG_TO_REG = 0,
  POLY_BRIDGE_MOVE_REG_TO_STACK,
  POLY_BRIDGE_MOVE_STACK_TO_REG,
  POLY_BRIDGE_MOVE_STACK_TO_STACK,
  POLY_BRIDGE_MOVE_LOAD_IMM,
  POLY_BRIDGE_MOVE_SAVE_CALLER_REG,
  POLY_BRIDGE_MOVE_RESTORE_CALLER_REG,
  POLY_BRIDGE_MOVE_SET_STATE_KEY,
  POLY_BRIDGE_MOVE_SET_SIGNATURE_SLOT,
  POLY_BRIDGE_MOVE_PCALL
};

enum {
  POLY_BRIDGE_PLAN_MAX_OPS = 128,
  POLY_BRIDGE_PLAN_NO_SIGNATURE_SLOT = UINT32_MAX
};

struct poly_bridge_op {
  enum poly_bridge_move_kind kind;
  struct poly_abi_loc src;
  struct poly_abi_loc dst;
  uint64_t imm;
};

struct poly_bridge_plan {
  enum poly_abi_arch caller;
  enum poly_abi_arch callee;
  struct poly_bridge_op ops[POLY_BRIDGE_PLAN_MAX_OPS];
  size_t op_count;
  uint32_t signature_slot;
  int needs_x86_tls_wrapper;
  int needs_state_key;
};

static inline void poly_bridge_plan_init(struct poly_bridge_plan *plan,
    enum poly_abi_arch caller, enum poly_abi_arch callee) {
  memset(plan, 0, sizeof(*plan));
  plan->caller = caller;
  plan->callee = callee;
  plan->signature_slot = POLY_BRIDGE_PLAN_NO_SIGNATURE_SLOT;
}

int poly_bridge_plan_add_op(struct poly_bridge_plan *plan,
    enum poly_bridge_move_kind kind, struct poly_abi_loc src,
    struct poly_abi_loc dst, uint64_t imm);

int poly_bridge_plan_validate(const struct poly_bridge_plan *plan);

#endif
