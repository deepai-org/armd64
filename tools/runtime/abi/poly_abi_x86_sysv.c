#include "poly_abi_classify.h"

struct x86_cursor {
  uint8_t gpr;
  uint8_t fpr;
  uint16_t stack;
};

static uint16_t align_to(uint16_t value, uint16_t align) {
  return (uint16_t) ((value + align - 1u) & ~(uint16_t) (align - 1u));
}

static int is_float_kind(enum poly_abi_type_kind kind) {
  return kind == POLY_ABI_TYPE_F32 || kind == POLY_ABI_TYPE_F64;
}

static int is_integer_kind(enum poly_abi_type_kind kind) {
  return kind == POLY_ABI_TYPE_I32 || kind == POLY_ABI_TYPE_I64 ||
    kind == POLY_ABI_TYPE_PTR;
}

static struct poly_abi_loc x86_stack_loc(struct x86_cursor *cursor,
    uint16_t size, uint16_t align) {
  cursor->stack = align_to(cursor->stack, align > 8 ? align : 8);
  const uint16_t offset = cursor->stack;
  cursor->stack = (uint16_t) (cursor->stack + align_to(size, 8));
  return poly_abi_loc_stack(offset, size);
}

static int x86_add_arg_loc(struct poly_abi_layout *out, size_t arg_index,
    struct x86_cursor *cursor, enum poly_abi_loc_kind kind, uint16_t size) {
  if (kind == POLY_ABI_LOC_GPR && cursor->gpr < 6)
    return poly_abi_layout_add_arg_loc(out, arg_index,
      poly_abi_loc_reg(kind, cursor->gpr++, size));
  if ((kind == POLY_ABI_LOC_FPR || kind == POLY_ABI_LOC_VEC) &&
      cursor->fpr < 8)
    return poly_abi_layout_add_arg_loc(out, arg_index,
      poly_abi_loc_reg(kind, cursor->fpr++, size));
  return poly_abi_layout_add_arg_loc(out, arg_index,
    x86_stack_loc(cursor, size, size >= 16 ? 16 : 8));
}

static int x86_classify_arg(const struct poly_abi_type *type,
    struct poly_abi_layout *out, size_t arg_index, struct x86_cursor *cursor) {
  if (is_integer_kind(type->kind))
    return x86_add_arg_loc(out, arg_index, cursor, POLY_ABI_LOC_GPR,
      type->size);
  if (is_float_kind(type->kind))
    return x86_add_arg_loc(out, arg_index, cursor, POLY_ABI_LOC_FPR,
      type->size);
  if (type->kind == POLY_ABI_TYPE_VEC128)
    return x86_add_arg_loc(out, arg_index, cursor, POLY_ABI_LOC_VEC,
      type->size);
  if (type->kind == POLY_ABI_TYPE_AGGREGATE && type->size <= 16 &&
      type->field_count > 0) {
    for (uint8_t n = 0; n < type->field_count; n++) {
      const enum poly_abi_loc_kind kind = is_float_kind(type->fields[n].kind) ?
        POLY_ABI_LOC_FPR : POLY_ABI_LOC_GPR;
      if (x86_add_arg_loc(out, arg_index, cursor, kind,
            type->fields[n].size) != 0)
        return -1;
    }
    return 0;
  }
  if (type->kind == POLY_ABI_TYPE_HFA && type->size <= 16) {
    const uint8_t parts = (uint8_t) ((type->size + 7u) / 8u);
    for (uint8_t n = 0; n < parts; n++) {
      const uint16_t part_size = n + 1u == parts ?
        (uint16_t) (type->size - (uint16_t) n * 8u) : 8;
      if (x86_add_arg_loc(out, arg_index, cursor, POLY_ABI_LOC_FPR,
            part_size) != 0)
        return -1;
    }
    return 0;
  }
  return poly_abi_layout_add_arg_loc(out, arg_index,
    x86_stack_loc(cursor, type->size, type->align));
}

static int x86_add_ret_loc(struct poly_abi_layout *out,
    enum poly_abi_loc_kind kind, uint8_t reg, uint16_t size) {
  return poly_abi_layout_add_ret_loc(out, poly_abi_loc_reg(kind, reg, size));
}

static int x86_classify_ret(const struct poly_abi_type *type,
    struct poly_abi_layout *out) {
  if (type->kind == POLY_ABI_TYPE_VOID)
    return 0;
  if (is_integer_kind(type->kind))
    return x86_add_ret_loc(out, POLY_ABI_LOC_GPR, 0, type->size);
  if (is_float_kind(type->kind))
    return x86_add_ret_loc(out, POLY_ABI_LOC_FPR, 0, type->size);
  if (type->kind == POLY_ABI_TYPE_VEC128)
    return x86_add_ret_loc(out, POLY_ABI_LOC_VEC, 0, type->size);
  if (type->kind == POLY_ABI_TYPE_AGGREGATE && type->size <= 16 &&
      type->field_count > 0) {
    uint8_t gpr = 0;
    uint8_t fpr = 0;
    for (uint8_t n = 0; n < type->field_count; n++) {
      const int is_float = is_float_kind(type->fields[n].kind);
      if (x86_add_ret_loc(out, is_float ? POLY_ABI_LOC_FPR :
            POLY_ABI_LOC_GPR, is_float ? fpr++ : gpr++,
            type->fields[n].size) != 0)
        return -1;
    }
    return 0;
  }
  if (type->kind == POLY_ABI_TYPE_HFA && type->size <= 16) {
    const uint8_t parts = (uint8_t) ((type->size + 7u) / 8u);
    for (uint8_t n = 0; n < parts; n++) {
      const uint16_t part_size = n + 1u == parts ?
        (uint16_t) (type->size - (uint16_t) n * 8u) : 8;
      if (x86_add_ret_loc(out, POLY_ABI_LOC_FPR, n, part_size) != 0)
        return -1;
    }
    return 0;
  }
  out->uses_sret = 1;
  return poly_abi_layout_add_ret_loc(out, poly_abi_loc_sret_ptr(0,
    type->size));
}

int poly_classify_x86_sysv_call_abi(const struct poly_function_sig *sig,
    struct poly_abi_layout *out) {
  if (sig == 0 || out == 0 || sig->arg_count > POLY_FUNCTION_SIG_MAX_ARGS)
    return -1;
  poly_abi_layout_init(out, POLY_ABI_ARCH_X86);
  struct x86_cursor cursor = { 0 };
  if ((sig->flags & POLY_FUNCTION_SIG_FLAG_RET_BY_SRET) != 0) {
    out->uses_sret = 1;
    cursor.gpr = 1;
    if (poly_abi_layout_add_ret_loc(out, poly_abi_loc_sret_ptr(0,
          sig->ret.size)) != 0)
      return -1;
  } else if (x86_classify_ret(&sig->ret, out) != 0) {
    return -1;
  }
  for (size_t n = 0; n < sig->arg_count; n++) {
    if (x86_classify_arg(&sig->args[n], out, n, &cursor) != 0)
      return -1;
  }
  out->stack_arg_size = cursor.stack;
  return 0;
}
