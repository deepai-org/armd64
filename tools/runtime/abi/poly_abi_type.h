#ifndef POLY_ABI_TYPE_H
#define POLY_ABI_TYPE_H

#include <stddef.h>
#include <stdint.h>

enum poly_abi_type_kind {
  POLY_ABI_TYPE_VOID = 0,
  POLY_ABI_TYPE_I32,
  POLY_ABI_TYPE_I64,
  POLY_ABI_TYPE_F32,
  POLY_ABI_TYPE_F64,
  POLY_ABI_TYPE_PTR,
  POLY_ABI_TYPE_VEC128,
  POLY_ABI_TYPE_AGGREGATE,
  POLY_ABI_TYPE_HFA
};

enum {
  POLY_ABI_TYPE_MAX_FIELDS = 4,
  POLY_FUNCTION_SIG_MAX_ARGS = 16,
  POLY_FUNCTION_SIG_FLAG_OPAQUE_DESCRIPTOR = 1u << 0,
  POLY_FUNCTION_SIG_FLAG_RET_BY_SRET = 1u << 1,
  POLY_FUNCTION_SIG_FLAG_NATIVE_SRET = 1u << 2
};

struct poly_abi_field {
  enum poly_abi_type_kind kind;
  uint16_t offset;
  uint16_t size;
};

struct poly_abi_type {
  enum poly_abi_type_kind kind;
  enum poly_abi_type_kind elem_kind;
  uint8_t count;
  uint8_t field_count;
  uint16_t size;
  uint16_t align;
  struct poly_abi_field fields[POLY_ABI_TYPE_MAX_FIELDS];
};

struct poly_function_sig {
  struct poly_abi_type args[POLY_FUNCTION_SIG_MAX_ARGS];
  size_t arg_count;
  struct poly_abi_type ret;
  uint32_t flags;
};

static inline struct poly_abi_type poly_abi_scalar_type(
    enum poly_abi_type_kind kind) {
  struct poly_abi_type type = { 0 };
  type.kind = kind;
  switch (kind) {
    case POLY_ABI_TYPE_VOID:
      type.size = 0;
      type.align = 1;
      break;
    case POLY_ABI_TYPE_I32:
    case POLY_ABI_TYPE_F32:
      type.size = 4;
      type.align = 4;
      break;
    case POLY_ABI_TYPE_I64:
    case POLY_ABI_TYPE_F64:
    case POLY_ABI_TYPE_PTR:
      type.size = 8;
      type.align = 8;
      break;
    case POLY_ABI_TYPE_VEC128:
      type.size = 16;
      type.align = 16;
      break;
    default:
      type.size = 0;
      type.align = 1;
      break;
  }
  return type;
}

static inline struct poly_abi_type poly_abi_hfa_type(
    enum poly_abi_type_kind elem_kind, uint8_t count) {
  struct poly_abi_type type = { 0 };
  const struct poly_abi_type elem = poly_abi_scalar_type(elem_kind);
  type.kind = POLY_ABI_TYPE_HFA;
  type.elem_kind = elem_kind;
  type.count = count;
  type.size = (uint16_t) (elem.size * count);
  type.align = elem.align;
  return type;
}

static inline int poly_abi_type_equal(const struct poly_abi_type *left,
    const struct poly_abi_type *right) {
  if (left->kind != right->kind ||
      left->elem_kind != right->elem_kind ||
      left->count != right->count ||
      left->field_count != right->field_count ||
      left->size != right->size ||
      left->align != right->align)
    return 0;
  for (uint8_t n = 0; n < left->field_count; n++) {
    if (left->fields[n].kind != right->fields[n].kind ||
        left->fields[n].offset != right->fields[n].offset ||
        left->fields[n].size != right->fields[n].size)
      return 0;
  }
  return 1;
}

static inline int poly_function_sig_equal(
    const struct poly_function_sig *left,
    const struct poly_function_sig *right) {
  if (left->arg_count != right->arg_count ||
      left->flags != right->flags ||
      !poly_abi_type_equal(&left->ret, &right->ret))
    return 0;
  for (size_t n = 0; n < left->arg_count; n++) {
    if (!poly_abi_type_equal(&left->args[n], &right->args[n]))
      return 0;
  }
  return 1;
}

#endif
