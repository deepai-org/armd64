#include "poly_abi_legacy_bridge.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "../bridge/poly_process_bridge_kind.h"
#include "../../include/polycpuid.h"

static struct poly_abi_type aggregate2(enum poly_abi_type_kind first,
    enum poly_abi_type_kind second) {
  struct poly_abi_type type = { 0 };
  const struct poly_abi_type first_type = poly_abi_scalar_type(first);
  const struct poly_abi_type second_type = poly_abi_scalar_type(second);
  type.kind = POLY_ABI_TYPE_AGGREGATE;
  type.field_count = 2;
  type.size = (uint16_t) (first_type.size + second_type.size);
  type.align = first_type.align > second_type.align ?
    first_type.align : second_type.align;
  type.fields[0].kind = first;
  type.fields[0].offset = 0;
  type.fields[0].size = first_type.size;
  type.fields[1].kind = second;
  type.fields[1].offset = first_type.size;
  type.fields[1].size = second_type.size;
  return type;
}

static struct poly_abi_type aggregate4(enum poly_abi_type_kind kind) {
  struct poly_abi_type type = { 0 };
  const struct poly_abi_type field_type = poly_abi_scalar_type(kind);
  type.kind = POLY_ABI_TYPE_AGGREGATE;
  type.field_count = 4;
  type.size = (uint16_t) (field_type.size * 4);
  type.align = field_type.align;
  for (uint8_t n = 0; n < 4; n++) {
    type.fields[n].kind = kind;
    type.fields[n].offset = (uint16_t) (n * field_type.size);
    type.fields[n].size = field_type.size;
  }
  return type;
}

static void set_void_ret(struct poly_function_sig *sig) {
  sig->ret = poly_abi_scalar_type(POLY_ABI_TYPE_VOID);
}

static void set_scalar_ret(struct poly_function_sig *sig,
    enum poly_abi_type_kind kind) {
  sig->ret = poly_abi_scalar_type(kind);
}

static void add_scalar_arg(struct poly_function_sig *sig,
    enum poly_abi_type_kind kind) {
  sig->args[sig->arg_count++] = poly_abi_scalar_type(kind);
}

static void set_hfa_ret(struct poly_function_sig *sig,
    enum poly_abi_type_kind elem_kind, uint8_t count) {
  for (uint8_t n = 0; n < count; n++)
    add_scalar_arg(sig, elem_kind);
  sig->ret = poly_abi_hfa_type(elem_kind, count);
}

static void set_hfa_arg(struct poly_function_sig *sig,
    enum poly_abi_type_kind elem_kind, uint8_t count) {
  sig->args[sig->arg_count++] = poly_abi_hfa_type(elem_kind, count);
  add_scalar_arg(sig, elem_kind);
  set_scalar_ret(sig, elem_kind);
}

static void set_fpair_arg(struct poly_function_sig *sig,
    enum poly_abi_type_kind elem_kind) {
  sig->args[sig->arg_count++] = aggregate2(elem_kind, elem_kind);
  add_scalar_arg(sig, elem_kind);
  set_scalar_ret(sig, elem_kind);
}

static void set_fpair_ret(struct poly_function_sig *sig,
    enum poly_abi_type_kind elem_kind) {
  add_scalar_arg(sig, elem_kind);
  add_scalar_arg(sig, elem_kind);
  add_scalar_arg(sig, elem_kind);
  sig->ret = aggregate2(elem_kind, elem_kind);
}

static void set_sret(struct poly_function_sig *sig, uint32_t flags) {
  add_scalar_arg(sig, POLY_ABI_TYPE_I64);
  add_scalar_arg(sig, POLY_ABI_TYPE_I64);
  add_scalar_arg(sig, POLY_ABI_TYPE_I64);
  sig->ret = aggregate4(POLY_ABI_TYPE_I64);
  sig->flags |= flags;
}

int poly_decode_legacy_bridge_kind(int bridge_kind,
    struct poly_function_sig *sig) {
  if (sig == NULL)
    return -1;
  memset(sig, 0, sizeof(*sig));
  set_void_ret(sig);

  switch (bridge_kind) {
    case POLY_PROCESS_BRIDGE_DEFAULT:
      sig->flags |= POLY_FUNCTION_SIG_FLAG_LEGACY_OPAQUE;
      set_scalar_ret(sig, POLY_ABI_TYPE_I64);
      return 0;
    case POLY_PROCESS_BRIDGE_VEC128_U32:
      add_scalar_arg(sig, POLY_ABI_TYPE_VEC128);
      add_scalar_arg(sig, POLY_ABI_TYPE_VEC128);
      set_scalar_ret(sig, POLY_ABI_TYPE_VEC128);
      return 0;
    case POLY_PROCESS_BRIDGE_COMPACT_U32_F32:
      sig->args[sig->arg_count++] =
        aggregate2(POLY_ABI_TYPE_I32, POLY_ABI_TYPE_F32);
      add_scalar_arg(sig, POLY_ABI_TYPE_I32);
      sig->ret = aggregate2(POLY_ABI_TYPE_I32, POLY_ABI_TYPE_F32);
      return 0;
    case POLY_PROCESS_BRIDGE_COMPACT_F32_U32:
      sig->args[sig->arg_count++] =
        aggregate2(POLY_ABI_TYPE_F32, POLY_ABI_TYPE_I32);
      add_scalar_arg(sig, POLY_ABI_TYPE_I32);
      sig->ret = aggregate2(POLY_ABI_TYPE_F32, POLY_ABI_TYPE_I32);
      return 0;
    case POLY_PROCESS_BRIDGE_U64_STACK9:
      for (unsigned n = 0; n < 9; n++)
        add_scalar_arg(sig, POLY_ABI_TYPE_I64);
      set_scalar_ret(sig, POLY_ABI_TYPE_I64);
      return 0;
    case POLY_PROCESS_BRIDGE_FP64:
      set_scalar_ret(sig, POLY_ABI_TYPE_F64);
      return 0;
    case POLY_PROCESS_BRIDGE_FP32:
      set_scalar_ret(sig, POLY_ABI_TYPE_F32);
      return 0;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET:
      set_hfa_ret(sig, POLY_ABI_TYPE_F32, 3);
      return 0;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET:
      set_hfa_ret(sig, POLY_ABI_TYPE_F32, 4);
      return 0;
    case POLY_PROCESS_BRIDGE_FPAIR32_ARG:
      set_fpair_arg(sig, POLY_ABI_TYPE_F32);
      return 0;
    case POLY_PROCESS_BRIDGE_FPAIR64_ARG:
      set_fpair_arg(sig, POLY_ABI_TYPE_F64);
      return 0;
    case POLY_PROCESS_BRIDGE_FPAIR32_RET:
      set_fpair_ret(sig, POLY_ABI_TYPE_F32);
      return 0;
    case POLY_PROCESS_BRIDGE_FPAIR64_RET:
      set_fpair_ret(sig, POLY_ABI_TYPE_F64);
      return 0;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG:
      set_hfa_arg(sig, POLY_ABI_TYPE_F32, 3);
      return 0;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG:
      set_hfa_arg(sig, POLY_ABI_TYPE_F32, 4);
      return 0;
    case POLY_PROCESS_BRIDGE_SRET_X86_SYSV:
      set_sret(sig, POLY_FUNCTION_SIG_FLAG_RET_BY_SRET);
      return 0;
    case POLY_PROCESS_BRIDGE_NATIVE_SRET:
      set_sret(sig, POLY_FUNCTION_SIG_FLAG_RET_BY_SRET |
        POLY_FUNCTION_SIG_FLAG_NATIVE_SRET);
      return 0;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET:
      set_hfa_ret(sig, POLY_ABI_TYPE_F64, 3);
      return 0;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET:
      set_hfa_ret(sig, POLY_ABI_TYPE_F64, 4);
      return 0;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_ARG:
      set_hfa_arg(sig, POLY_ABI_TYPE_F64, 3);
      return 0;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG:
      set_hfa_arg(sig, POLY_ABI_TYPE_F64, 4);
      return 0;
    default:
      return -1;
  }
}

uint32_t poly_legacy_bridge_signature_kind(int bridge_kind) {
  switch (bridge_kind) {
    case POLY_PROCESS_BRIDGE_VEC128_U32:
      return POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32;
    case POLY_PROCESS_BRIDGE_COMPACT_U32_F32:
      return POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_U32_F32;
    case POLY_PROCESS_BRIDGE_COMPACT_F32_U32:
      return POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_F32_U32;
    case POLY_PROCESS_BRIDGE_FP64:
      return POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64;
    case POLY_PROCESS_BRIDGE_FP32:
      return POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP32;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA3_F32_RET;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA4_F32_RET;
    case POLY_PROCESS_BRIDGE_FPAIR32_ARG:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FPAIR32_ARG;
    case POLY_PROCESS_BRIDGE_FPAIR64_ARG:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FPAIR64_ARG;
    case POLY_PROCESS_BRIDGE_FPAIR32_RET:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FPAIR32_RET;
    case POLY_PROCESS_BRIDGE_FPAIR64_RET:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FPAIR64_RET;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA3_F32_ARG;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA4_F32_ARG;
    case POLY_PROCESS_BRIDGE_SRET_X86_SYSV:
      return POLY_ABI_SIGNATURE_KIND_SRET_X86_SYSV_REGS;
    case POLY_PROCESS_BRIDGE_NATIVE_SRET:
      return POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA3_F64_RET;
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET:
      return POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA4_F64_RET;
    case POLY_PROCESS_BRIDGE_DEFAULT:
    case POLY_PROCESS_BRIDGE_U64_STACK9:
      return POLY_ABI_SIGNATURE_KIND_NATIVE_REGS;
    default:
      return UINT_MAX;
  }
}

const char *poly_legacy_bridge_kind_name(int bridge_kind) {
  switch (bridge_kind) {
    case POLY_PROCESS_BRIDGE_DEFAULT: return "default";
    case POLY_PROCESS_BRIDGE_VEC128_U32: return "vec128_u32";
    case POLY_PROCESS_BRIDGE_COMPACT_U32_F32: return "compact_u32_f32";
    case POLY_PROCESS_BRIDGE_COMPACT_F32_U32: return "compact_f32_u32";
    case POLY_PROCESS_BRIDGE_U64_STACK9: return "u64_stack9";
    case POLY_PROCESS_BRIDGE_FP64: return "fp64";
    case POLY_PROCESS_BRIDGE_FP32: return "fp32";
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET:
      return "aarch64_hfa3_f32_ret";
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET:
      return "aarch64_hfa4_f32_ret";
    case POLY_PROCESS_BRIDGE_FPAIR32_ARG: return "fpair32_arg";
    case POLY_PROCESS_BRIDGE_FPAIR64_ARG: return "fpair64_arg";
    case POLY_PROCESS_BRIDGE_FPAIR32_RET: return "fpair32_ret";
    case POLY_PROCESS_BRIDGE_FPAIR64_RET: return "fpair64_ret";
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG:
      return "aarch64_hfa3_f32_arg";
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG:
      return "aarch64_hfa4_f32_arg";
    case POLY_PROCESS_BRIDGE_SRET_X86_SYSV: return "sret_x86_sysv";
    case POLY_PROCESS_BRIDGE_NATIVE_SRET: return "native_sret";
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET:
      return "aarch64_hfa3_f64_ret";
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET:
      return "aarch64_hfa4_f64_ret";
    case POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_ARG:
      return "aarch64_hfa3_f64_arg";
    case POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG:
      return "aarch64_hfa4_f64_arg";
    default:
      return NULL;
  }
}
