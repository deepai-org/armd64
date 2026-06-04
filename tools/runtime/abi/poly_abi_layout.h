#ifndef POLY_ABI_LAYOUT_H
#define POLY_ABI_LAYOUT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../include/polycpuid.h"
#include "poly_abi_type.h"

enum poly_abi_arch {
  POLY_ABI_ARCH_X86 = POLY_FRONTEND_X86,
  POLY_ABI_ARCH_AARCH64 = POLY_FRONTEND_AARCH64,
  POLY_ABI_ARCH_RISCV = POLY_FRONTEND_RISCV
};

enum poly_abi_loc_kind {
  POLY_ABI_LOC_NONE = 0,
  POLY_ABI_LOC_GPR,
  POLY_ABI_LOC_FPR,
  POLY_ABI_LOC_VEC,
  POLY_ABI_LOC_STACK,
  POLY_ABI_LOC_SRET_PTR
};

enum {
  POLY_ABI_LOC_MAX_PARTS = 4,
  POLY_ABI_LAYOUT_MAX_ARGS = POLY_FUNCTION_SIG_MAX_ARGS,
  POLY_ABI_LAYOUT_MAX_RET = 4
};

struct poly_abi_loc {
  enum poly_abi_loc_kind kind;
  uint8_t reg;
  uint16_t stack_offset;
  uint16_t size;
};

struct poly_abi_layout_arg {
  struct poly_abi_loc locs[POLY_ABI_LOC_MAX_PARTS];
  size_t loc_count;
};

struct poly_abi_layout {
  enum poly_abi_arch arch;
  struct poly_abi_layout_arg args[POLY_ABI_LAYOUT_MAX_ARGS];
  size_t arg_count;
  struct poly_abi_loc ret[POLY_ABI_LAYOUT_MAX_RET];
  size_t ret_count;
  int uses_sret;
  uint16_t stack_arg_size;
  uint16_t stack_align;
};

static inline void poly_abi_layout_init(struct poly_abi_layout *layout,
    enum poly_abi_arch arch) {
  memset(layout, 0, sizeof(*layout));
  layout->arch = arch;
  layout->stack_align = 16;
}

static inline struct poly_abi_loc poly_abi_loc_none(void) {
  return (struct poly_abi_loc) { 0 };
}

static inline struct poly_abi_loc poly_abi_loc_reg(
    enum poly_abi_loc_kind kind, uint8_t reg, uint16_t size) {
  return (struct poly_abi_loc) {
    .kind = kind,
    .reg = reg,
    .size = size
  };
}

static inline struct poly_abi_loc poly_abi_loc_stack(uint16_t offset,
    uint16_t size) {
  return (struct poly_abi_loc) {
    .kind = POLY_ABI_LOC_STACK,
    .stack_offset = offset,
    .size = size
  };
}

static inline struct poly_abi_loc poly_abi_loc_sret_ptr(uint8_t reg,
    uint16_t size) {
  return (struct poly_abi_loc) {
    .kind = POLY_ABI_LOC_SRET_PTR,
    .reg = reg,
    .size = size
  };
}

static inline int poly_abi_layout_add_arg_loc(
    struct poly_abi_layout *layout, size_t arg_index,
    struct poly_abi_loc loc) {
  if (arg_index >= POLY_ABI_LAYOUT_MAX_ARGS)
    return -1;
  if (layout->args[arg_index].loc_count >= POLY_ABI_LOC_MAX_PARTS)
    return -1;
  layout->args[arg_index].locs[layout->args[arg_index].loc_count++] = loc;
  if (layout->arg_count <= arg_index)
    layout->arg_count = arg_index + 1;
  return 0;
}

static inline int poly_abi_layout_add_ret_loc(
    struct poly_abi_layout *layout, struct poly_abi_loc loc) {
  if (layout->ret_count >= POLY_ABI_LAYOUT_MAX_RET)
    return -1;
  layout->ret[layout->ret_count++] = loc;
  return 0;
}

static inline int poly_abi_loc_equal(const struct poly_abi_loc *left,
    const struct poly_abi_loc *right) {
  return left->kind == right->kind &&
    left->reg == right->reg &&
    left->stack_offset == right->stack_offset &&
    left->size == right->size;
}

#endif
