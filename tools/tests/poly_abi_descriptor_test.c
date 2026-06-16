#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../include/polycpuid.h"
#include "../runtime/abi/poly_abi_descriptor.h"
#include "../runtime/bridge/poly_process_bridge_kind.h"

static struct poly_abi_type aggregate2(enum poly_abi_type_kind first,
    enum poly_abi_type_kind second) {
  struct poly_abi_type type = { 0 };
  struct poly_abi_type first_type = poly_abi_scalar_type(first);
  struct poly_abi_type second_type = poly_abi_scalar_type(second);
  type.kind = POLY_ABI_TYPE_AGGREGATE;
  type.field_count = 2;
  type.size = (uint16_t) (first_type.size + second_type.size);
  type.align = first_type.align > second_type.align ?
    first_type.align : second_type.align;
  type.fields[0].kind = first;
  type.fields[0].size = first_type.size;
  type.fields[1].kind = second;
  type.fields[1].offset = first_type.size;
  type.fields[1].size = second_type.size;
  return type;
}

static struct poly_abi_type aggregate4(enum poly_abi_type_kind kind) {
  struct poly_abi_type type = { 0 };
  struct poly_abi_type field_type = poly_abi_scalar_type(kind);
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

static void add_scalar_arg(struct poly_function_sig *sig,
    enum poly_abi_type_kind kind) {
  sig->args[sig->arg_count++] = poly_abi_scalar_type(kind);
}

static struct poly_function_sig sig_default(void) {
  struct poly_function_sig sig = { 0 };
  sig.ret = poly_abi_scalar_type(POLY_ABI_TYPE_I64);
  sig.flags = POLY_FUNCTION_SIG_FLAG_OPAQUE_DESCRIPTOR;
  return sig;
}

static struct poly_function_sig sig_ret(enum poly_abi_type_kind kind) {
  struct poly_function_sig sig = { 0 };
  sig.ret = poly_abi_scalar_type(kind);
  return sig;
}

static struct poly_function_sig sig_vec128(void) {
  struct poly_function_sig sig = { 0 };
  add_scalar_arg(&sig, POLY_ABI_TYPE_VEC128);
  add_scalar_arg(&sig, POLY_ABI_TYPE_VEC128);
  sig.ret = poly_abi_scalar_type(POLY_ABI_TYPE_VEC128);
  return sig;
}

static struct poly_function_sig sig_compact(enum poly_abi_type_kind first,
    enum poly_abi_type_kind second) {
  struct poly_function_sig sig = { 0 };
  sig.args[sig.arg_count++] = aggregate2(first, second);
  add_scalar_arg(&sig, POLY_ABI_TYPE_I32);
  sig.ret = aggregate2(first, second);
  return sig;
}

static struct poly_function_sig sig_stack9(void) {
  struct poly_function_sig sig = { 0 };
  for (unsigned n = 0; n < 9; n++)
    add_scalar_arg(&sig, POLY_ABI_TYPE_I64);
  sig.ret = poly_abi_scalar_type(POLY_ABI_TYPE_I64);
  return sig;
}

static struct poly_function_sig sig_hfa_ret(enum poly_abi_type_kind elem,
    uint8_t count) {
  struct poly_function_sig sig = { 0 };
  for (uint8_t n = 0; n < count; n++)
    add_scalar_arg(&sig, elem);
  sig.ret = poly_abi_hfa_type(elem, count);
  return sig;
}

static struct poly_function_sig sig_hfa_arg(enum poly_abi_type_kind elem,
    uint8_t count) {
  struct poly_function_sig sig = { 0 };
  sig.args[sig.arg_count++] = poly_abi_hfa_type(elem, count);
  add_scalar_arg(&sig, elem);
  sig.ret = poly_abi_scalar_type(elem);
  return sig;
}

static struct poly_function_sig sig_fpair_arg(enum poly_abi_type_kind elem) {
  struct poly_function_sig sig = { 0 };
  sig.args[sig.arg_count++] = aggregate2(elem, elem);
  add_scalar_arg(&sig, elem);
  sig.ret = poly_abi_scalar_type(elem);
  return sig;
}

static struct poly_function_sig sig_fpair_ret(enum poly_abi_type_kind elem) {
  struct poly_function_sig sig = { 0 };
  add_scalar_arg(&sig, elem);
  add_scalar_arg(&sig, elem);
  add_scalar_arg(&sig, elem);
  sig.ret = aggregate2(elem, elem);
  return sig;
}

static struct poly_function_sig sig_sret(uint32_t flags) {
  struct poly_function_sig sig = { 0 };
  add_scalar_arg(&sig, POLY_ABI_TYPE_I64);
  add_scalar_arg(&sig, POLY_ABI_TYPE_I64);
  add_scalar_arg(&sig, POLY_ABI_TYPE_I64);
  sig.ret = aggregate4(POLY_ABI_TYPE_I64);
  sig.flags = flags;
  return sig;
}

struct bridge_case {
  int kind;
  const char *name;
  uint32_t signature_kind;
  struct poly_function_sig sig;
};

static int check_case(const struct bridge_case *test_case) {
  struct poly_function_sig actual;
  const char *actual_name = poly_abi_descriptor_kind_name(test_case->kind);
  if (actual_name == NULL || strcmp(actual_name, test_case->name) != 0) {
    fprintf(stderr, "bridge name mismatch kind=%d expected=%s actual=%s\n",
      test_case->kind, test_case->name, actual_name ? actual_name : "(null)");
    return 1;
  }
  if (poly_abi_descriptor_decode_kind(test_case->kind, &actual) != 0) {
    fprintf(stderr, "bridge decode failed kind=%d name=%s\n",
      test_case->kind, test_case->name);
    return 1;
  }
  if (!poly_function_sig_equal(&actual, &test_case->sig)) {
    fprintf(stderr,
      "bridge signature mismatch kind=%d name=%s args=%zu expected_args=%zu flags=0x%x expected_flags=0x%x ret_kind=%d expected_ret_kind=%d\n",
      test_case->kind, test_case->name, actual.arg_count,
      test_case->sig.arg_count, actual.flags, test_case->sig.flags,
      actual.ret.kind, test_case->sig.ret.kind);
    return 1;
  }
  const uint32_t actual_signature_kind =
    poly_abi_descriptor_signature_kind(test_case->kind);
  if (actual_signature_kind != test_case->signature_kind) {
    fprintf(stderr,
      "bridge signature kind mismatch kind=%d name=%s expected=%u actual=%u\n",
      test_case->kind, test_case->name, test_case->signature_kind,
      actual_signature_kind);
    return 1;
  }
  return 0;
}

int main(void) {
  const struct bridge_case cases[] = {
    { POLY_PROCESS_BRIDGE_DEFAULT, "default",
      POLY_ABI_SIGNATURE_KIND_NATIVE_REGS, sig_default() },
    { POLY_PROCESS_BRIDGE_VEC128_U32, "vec128_u32",
      POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32, sig_vec128() },
    { POLY_PROCESS_BRIDGE_COMPACT_U32_F32, "compact_u32_f32",
      POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_U32_F32,
      sig_compact(POLY_ABI_TYPE_I32, POLY_ABI_TYPE_F32) },
    { POLY_PROCESS_BRIDGE_COMPACT_F32_U32, "compact_f32_u32",
      POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_F32_U32,
      sig_compact(POLY_ABI_TYPE_F32, POLY_ABI_TYPE_I32) },
    { POLY_PROCESS_BRIDGE_U64_STACK9, "u64_stack9",
      POLY_ABI_SIGNATURE_KIND_NATIVE_REGS, sig_stack9() },
    { POLY_PROCESS_BRIDGE_FP64, "fp64",
      POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64,
      sig_ret(POLY_ABI_TYPE_F64) },
    { POLY_PROCESS_BRIDGE_FP32, "fp32",
      POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP32,
      sig_ret(POLY_ABI_TYPE_F32) },
    { POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_RET,
      "aarch64_hfa3_f32_ret",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA3_F32_RET,
      sig_hfa_ret(POLY_ABI_TYPE_F32, 3) },
    { POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_RET,
      "aarch64_hfa4_f32_ret",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA4_F32_RET,
      sig_hfa_ret(POLY_ABI_TYPE_F32, 4) },
    { POLY_PROCESS_BRIDGE_FPAIR32_ARG, "fpair32_arg",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FPAIR32_ARG,
      sig_fpair_arg(POLY_ABI_TYPE_F32) },
    { POLY_PROCESS_BRIDGE_FPAIR64_ARG, "fpair64_arg",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FPAIR64_ARG,
      sig_fpair_arg(POLY_ABI_TYPE_F64) },
    { POLY_PROCESS_BRIDGE_FPAIR32_RET, "fpair32_ret",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FPAIR32_RET,
      sig_fpair_ret(POLY_ABI_TYPE_F32) },
    { POLY_PROCESS_BRIDGE_FPAIR64_RET, "fpair64_ret",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FPAIR64_RET,
      sig_fpair_ret(POLY_ABI_TYPE_F64) },
    { POLY_PROCESS_BRIDGE_AARCH64_HFA3_F32_ARG,
      "aarch64_hfa3_f32_arg",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA3_F32_ARG,
      sig_hfa_arg(POLY_ABI_TYPE_F32, 3) },
    { POLY_PROCESS_BRIDGE_AARCH64_HFA4_F32_ARG,
      "aarch64_hfa4_f32_arg",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA4_F32_ARG,
      sig_hfa_arg(POLY_ABI_TYPE_F32, 4) },
    { POLY_PROCESS_BRIDGE_SRET_X86_SYSV, "sret_x86_sysv",
      POLY_ABI_SIGNATURE_KIND_SRET_X86_SYSV_REGS,
      sig_sret(POLY_FUNCTION_SIG_FLAG_RET_BY_SRET) },
    { POLY_PROCESS_BRIDGE_NATIVE_SRET, "native_sret",
      POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS,
      sig_sret(POLY_FUNCTION_SIG_FLAG_RET_BY_SRET |
        POLY_FUNCTION_SIG_FLAG_NATIVE_SRET) },
    { POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_RET,
      "aarch64_hfa3_f64_ret",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA3_F64_RET,
      sig_hfa_ret(POLY_ABI_TYPE_F64, 3) },
    { POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_RET,
      "aarch64_hfa4_f64_ret",
      POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_AARCH64_HFA4_F64_RET,
      sig_hfa_ret(POLY_ABI_TYPE_F64, 4) },
    { POLY_PROCESS_BRIDGE_AARCH64_HFA3_F64_ARG,
      "aarch64_hfa3_f64_arg", UINT_MAX,
      sig_hfa_arg(POLY_ABI_TYPE_F64, 3) },
    { POLY_PROCESS_BRIDGE_AARCH64_HFA4_F64_ARG,
      "aarch64_hfa4_f64_arg", UINT_MAX,
      sig_hfa_arg(POLY_ABI_TYPE_F64, 4) },
  };

  if ((int) (sizeof(cases) / sizeof(cases[0])) !=
      POLY_PROCESS_BRIDGE_KIND_COUNT) {
    fprintf(stderr, "bridge test case count mismatch expected=%d actual=%zu\n",
      POLY_PROCESS_BRIDGE_KIND_COUNT, sizeof(cases) / sizeof(cases[0]));
    return 1;
  }
  for (size_t n = 0; n < sizeof(cases) / sizeof(cases[0]); n++) {
    if (cases[n].kind != (int) n) {
      fprintf(stderr, "bridge enum density mismatch index=%zu kind=%d\n",
        n, cases[n].kind);
      return 1;
    }
    if (check_case(&cases[n]) != 0)
      return 1;
  }
  if (poly_abi_descriptor_decode_kind(POLY_PROCESS_BRIDGE_KIND_COUNT,
        &(struct poly_function_sig) { 0 }) == 0) {
    fprintf(stderr, "invalid bridge kind decoded successfully\n");
    return 1;
  }
  if (poly_abi_descriptor_kind_name(POLY_PROCESS_BRIDGE_KIND_COUNT) != NULL) {
    fprintf(stderr, "invalid bridge kind returned a name\n");
    return 1;
  }
  printf("poly ABI descriptor decoder OK\n");
  return 0;
}
