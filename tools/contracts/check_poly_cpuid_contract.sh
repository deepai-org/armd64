#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BOCHS_CPU="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"
BOCHS_CRREGS="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/crregs.h"
HEADER="$ROOT_DIR/tools/include/polycpuid.h"
POLYCALL="$ROOT_DIR/tools/runtime/polycall.c"
POLYBENCH="$ROOT_DIR/tools/programs/polybench.c"
POLY_ISA_DOC="$ROOT_DIR/docs/poly-isa.md"
TMP_DIR="${TMPDIR:-/tmp}/poly-cpuid-contract.$$"

mkdir -p "$TMP_DIR"
trap 'rm -rf "$TMP_DIR"' EXIT

fail() {
  echo "poly CPUID contract check failed: $*" >&2
  exit 1
}

assert_contains() {
  local pattern="$1"
  local file="$2"
  local description="$3"

  if ! grep -Eq "$pattern" "$file"; then
    fail "$description"
  fi
}

assert_not_contains() {
  local pattern="$1"
  local file="$2"
  local description="$3"

  if grep -Eq "$pattern" "$file"; then
    fail "$description"
  fi
}

normalize_expr() {
  local expr="$1"
  expr="${expr%%//*}"
  expr="${expr//BX_CONST64(/(}"
  expr="${expr//Bit32u/}"
  expr="${expr//Bit64u/}"
  expr="${expr//uint32_t/}"
  expr="${expr//uint64_t/}"
  expr="${expr//ULL/}"
  expr="${expr//LLU/}"
  expr="${expr//LL/}"
  expr="${expr//U/}"
  expr="${expr//u/}"
  expr="${expr// /}"
  expr="${expr//;/}"
  expr="${expr//,/}"
  echo "$expr"
}

eval_expr() {
  local expr
  expr="$(normalize_expr "$1")"
  [[ -n "$expr" ]] || fail "empty expression"
  printf '%llu\n' "$((expr))"
}

bochs_const_expr() {
  local name="$1"
  awk -v name="$name" '
    $0 ~ name "[[:space:]]*=" {
      line = $0
      sub(/.*=[[:space:]]*/, "", line)
      sub(/;.*/, "", line)
      print line
      exit
    }
  ' "$BOCHS_CPU"
}

header_const_expr() {
  local name="$1"
  awk -v name="$name" '
    $0 ~ name "[[:space:]]*=" {
      line = $0
      sub(/.*=[[:space:]]*/, "", line)
      sub(/,.*/, "", line)
      print line
      exit
    }
  ' "$HEADER"
}

polycall_const_expr() {
  local name="$1"
  awk -v name="$name" '
    $0 ~ name "[[:space:]]*=" {
      line = $0
      sub(/.*=[[:space:]]*/, "", line)
      sub(/;.*/, "", line)
      sub(/,.*/, "", line)
      print line
      exit
    }
  ' "$POLYCALL"
}

polybench_const_expr() {
  local name="$1"
  awk -v name="$name" '
    $0 ~ name "[[:space:]]*=" {
      line = $0
      sub(/.*=[[:space:]]*/, "", line)
      sub(/;.*/, "", line)
      sub(/,.*/, "", line)
      print line
      exit
    }
  ' "$POLYBENCH"
}

compare_const() {
  local bochs_name="$1"
  local header_name="$2"
  local bochs_expr header_expr bochs_value header_value

  bochs_expr="$(bochs_const_expr "$bochs_name")"
  header_expr="$(header_const_expr "$header_name")"
  [[ -n "$bochs_expr" ]] || fail "missing Bochs constant $bochs_name"
  [[ -n "$header_expr" ]] || fail "missing header constant $header_name"

  bochs_value="$(eval_expr "$bochs_expr")"
  header_value="$(eval_expr "$header_expr")"
  if [[ "$bochs_value" != "$header_value" ]]; then
    fail "$bochs_name=$bochs_value differs from $header_name=$header_value"
  fi
}

compare_polycall_const() {
  local header_name="$1"
  local polycall_name="${2:-$1}"
  local header_expr polycall_expr header_value polycall_value

  header_expr="$(header_const_expr "$header_name")"
  polycall_expr="$(polycall_const_expr "$polycall_name")"
  [[ -n "$header_expr" ]] || fail "missing header constant $header_name"
  [[ -n "$polycall_expr" ]] || fail "missing polycall constant $polycall_name"

  header_value="$(eval_expr "$header_expr")"
  polycall_value="$(eval_expr "$polycall_expr")"
  if [[ "$header_value" != "$polycall_value" ]]; then
    fail "$header_name=$header_value differs from $polycall_name=$polycall_value"
  fi
}

compare_polybench_const() {
  local header_name="$1"
  local polybench_name="${2:-$1}"
  local header_expr polybench_expr header_value polybench_value

  header_expr="$(header_const_expr "$header_name")"
  polybench_expr="$(polybench_const_expr "$polybench_name")"
  [[ -n "$header_expr" ]] || fail "missing header constant $header_name"
  [[ -n "$polybench_expr" ]] || fail "missing polybench constant $polybench_name"

  header_value="$(eval_expr "$header_expr")"
  polybench_value="$(eval_expr "$polybench_expr")"
  if [[ "$header_value" != "$polybench_value" ]]; then
    fail "$header_name=$header_value differs from $polybench_name=$polybench_value"
  fi
}

compare_const BX_POLY_MODE_X86 POLY_MODE_X86
compare_const BX_POLY_MODE_RAW_AARCH64 POLY_MODE_RAW_AARCH64
compare_const BX_POLY_MODE_RAW_RISCV POLY_MODE_RAW_RISCV
compare_const BX_POLY_CPUID_BASE POLY_CPUID_BASE
compare_const BX_POLY_CPUID_MAX POLY_CPUID_MAX
compare_const BX_POLY_AARCH64_BRK_X86_ESCAPE POLY_AARCH64_BRK_X86_ESCAPE
compare_const BX_POLY_AARCH64_BRK_RISCV_SWITCH POLY_AARCH64_BRK_RISCV_SWITCH
compare_const BX_POLY_AARCH64_BRK_RISCV_CALL POLY_AARCH64_BRK_RISCV_CALL
compare_const BX_POLY_AARCH64_BRK_RISCV_CALL_COMPACT_U32_F32 POLY_AARCH64_BRK_RISCV_CALL_COMPACT_U32_F32
compare_const BX_POLY_AARCH64_BRK_RISCV_CALL_COMPACT_F32_U32 POLY_AARCH64_BRK_RISCV_CALL_COMPACT_F32_U32
compare_const BX_POLY_AARCH64_BRK_RISCV_CALL_FP64_STACK POLY_AARCH64_BRK_RISCV_CALL_FP64_STACK
compare_const BX_POLY_AARCH64_BRK_TRAP_RETURN POLY_AARCH64_BRK_TRAP_RETURN
compare_const BX_POLY_RISCV_X86_ESCAPE POLY_RISCV_X86_ESCAPE
compare_const BX_POLY_RISCV_AARCH64_SWITCH POLY_RISCV_AARCH64_SWITCH
compare_const BX_POLY_RISCV_AARCH64_CALL POLY_RISCV_AARCH64_CALL
compare_const BX_POLY_RISCV_AARCH64_CALL_COMPACT_U32_F32 POLY_RISCV_AARCH64_CALL_COMPACT_U32_F32
compare_const BX_POLY_RISCV_AARCH64_CALL_COMPACT_F32_U32 POLY_RISCV_AARCH64_CALL_COMPACT_F32_U32
compare_const BX_POLY_RISCV_AARCH64_CALL_FP64_STACK POLY_RISCV_AARCH64_CALL_FP64_STACK
compare_const BX_POLY_RISCV_TRAP_RETURN POLY_RISCV_TRAP_RETURN
compare_const BX_POLY_IMPORT_FUNC_X86_SLOT0 POLY_IMPORT_FUNC_X86_SLOT0
compare_const BX_POLY_IMPORT_FUNC_X86_SLOT7 POLY_IMPORT_FUNC_X86_SLOT7
compare_const BX_POLY_IMPORT_CALL_COUNT POLY_IMPORT_FUNC_COUNT
compare_const BX_POLY_IMPORT_X86_DESCRIPTOR_SIZE POLY_IMPORT_X86_DESCRIPTOR_SIZE
compare_const BX_POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS
compare_const BX_POLY_IMPORT_X86_DESCRIPTOR_RETURN_I128 POLY_IMPORT_X86_DESCRIPTOR_RETURN_I128
compare_const BX_POLY_IMPORT_X86_DESCRIPTOR_RETURN_FP128 POLY_IMPORT_X86_DESCRIPTOR_RETURN_FP128
compare_const BX_POLY_IMPORT_CALL_STRIDE POLY_IMPORT_CALL_STRIDE
compare_const BX_POLY_CPUID_STATE_OVERLAP_GPRS POLY_CPUID_STATE_OVERLAP_GPRS
compare_const BX_POLY_CPUID_STATE_SYNTHETIC_BANKS POLY_CPUID_STATE_SYNTHETIC_BANKS
compare_const BX_POLY_CPUID_STATE_KEY_CR3 POLY_CPUID_STATE_KEY_CR3
compare_const BX_POLY_CPUID_STATE_KEY_FSBASE POLY_CPUID_STATE_KEY_FSBASE
compare_const BX_POLY_CPUID_STATE_KEY_STACK_REGION POLY_CPUID_STATE_KEY_STACK_REGION
compare_const BX_POLY_CPUID_STATE_USER_RETURN_RESTORE POLY_CPUID_STATE_USER_RETURN_RESTORE
compare_const BX_POLY_CPUID_STATE_X86_TSO POLY_CPUID_STATE_X86_TSO
compare_const BX_POLY_CPUID_STATE_XSAVE_VISIBLE POLY_CPUID_STATE_XSAVE_VISIBLE
compare_const BX_POLY_CPUID_STATE_KEY_EXPLICIT POLY_CPUID_STATE_KEY_EXPLICIT
compare_const BX_POLY_CPUID_STATE_TRANSITION_FRAME_32 POLY_CPUID_STATE_TRANSITION_FRAME_32
compare_const BX_POLY_CPUID_STATE_EXPLICIT_SAVE_RESTORE POLY_CPUID_STATE_EXPLICIT_SAVE_RESTORE
compare_const BX_POLY_CPUID_STATE_XSAVE_ARCH_CONTRACT POLY_CPUID_STATE_XSAVE_ARCH_CONTRACT
compare_const BX_POLY_STATE_STACK_KEY_SHIFT POLY_STATE_STACK_KEY_SHIFT
compare_const BX_POLY_STATE_XSAVE_MAGIC POLY_STATE_XSAVE_MAGIC
compare_const BX_POLY_STATE_XSAVE_COMPONENT_ARCH POLY_STATE_XSAVE_COMPONENT_ARCH
compare_const BX_POLY_STATE_XSAVE_BYTES_ARCH POLY_STATE_XSAVE_BYTES_ARCH
compare_const BX_POLY_STATE_XSAVE_ALIGN_ARCH POLY_STATE_XSAVE_ALIGN_ARCH
compare_const BX_POLY_STATE_XSAVE_LAYOUT_VERSION POLY_STATE_XSAVE_LAYOUT_VERSION
compare_const BX_POLY_STATE_XSAVE_FLAG_XCR0_USER POLY_STATE_XSAVE_FLAG_XCR0_USER
compare_const BX_POLY_STATE_XSAVE_FLAG_OSXSAVE_REQUIRED POLY_STATE_XSAVE_FLAG_OSXSAVE_REQUIRED
compare_const BX_POLY_STATE_XSAVE_FLAG_INTERRUPT_RESUME POLY_STATE_XSAVE_FLAG_INTERRUPT_RESUME
compare_const BX_POLY_STATE_XSAVE_FLAG_TRAP_STATE POLY_STATE_XSAVE_FLAG_TRAP_STATE
compare_const BX_POLY_STATE_XSAVE_FLAG_NO_HIDDEN_BANKS POLY_STATE_XSAVE_FLAG_NO_HIDDEN_BANKS
compare_const BX_POLY_STATE_XSAVE_HEADER_OFFSET POLY_STATE_XSAVE_HEADER_OFFSET
compare_const BX_POLY_STATE_XSAVE_TRAP_PACKET_OFFSET POLY_STATE_XSAVE_TRAP_PACKET_OFFSET
compare_const BX_POLY_STATE_XSAVE_TRAP_ARGS_OFFSET POLY_STATE_XSAVE_TRAP_ARGS_OFFSET
compare_const BX_POLY_STATE_XSAVE_TRANSITION_OFFSET POLY_STATE_XSAVE_TRANSITION_OFFSET
compare_const BX_POLY_STATE_XSAVE_AARCH64_GPR_OFFSET POLY_STATE_XSAVE_AARCH64_GPR_OFFSET
compare_const BX_POLY_STATE_XSAVE_AARCH64_FP_OFFSET POLY_STATE_XSAVE_AARCH64_FP_OFFSET
compare_const BX_POLY_STATE_XSAVE_AARCH64_STATUS_OFFSET POLY_STATE_XSAVE_AARCH64_STATUS_OFFSET
compare_const BX_POLY_STATE_XSAVE_RISCV_GPR_OFFSET POLY_STATE_XSAVE_RISCV_GPR_OFFSET
compare_const BX_POLY_STATE_XSAVE_RISCV_FP_OFFSET POLY_STATE_XSAVE_RISCV_FP_OFFSET
compare_const BX_POLY_STATE_XSAVE_RISCV_STATUS_OFFSET POLY_STATE_XSAVE_RISCV_STATUS_OFFSET
compare_const BX_POLY_TRAP_PACKET_LAYOUT_VERSION POLY_TRAP_PACKET_LAYOUT_VERSION
compare_const BX_POLY_TRAP_PACKET_HEADER_BYTES POLY_TRAP_PACKET_HEADER_BYTES
compare_const BX_POLY_TRAP_PACKET_ARG_COUNT POLY_TRAP_PACKET_ARG_COUNT
compare_const BX_POLY_TRAP_PACKET_FLAG_VECTOR_DELIVERY POLY_TRAP_PACKET_FLAG_VECTOR_DELIVERY
compare_const BX_POLY_TRAP_PACKET_FLAG_NO_VECTOR_X86_EXCEPTIONS POLY_TRAP_PACKET_FLAG_NO_VECTOR_X86_EXCEPTIONS
compare_const BX_POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE
compare_const BX_POLY_TRAP_PACKET_FLAG_ALL_FRONTEND_HANDLERS POLY_TRAP_PACKET_FLAG_ALL_FRONTEND_HANDLERS
compare_const BX_POLY_TRAP_PACKET_FLAG_STATUS_OPS POLY_TRAP_PACKET_FLAG_STATUS_OPS
compare_const BX_POLY_INTERRUPT_ABI_VERSION POLY_INTERRUPT_ABI_VERSION
compare_const BX_POLY_INTERRUPT_FLAG_RAW_CPL3_ONLY POLY_INTERRUPT_FLAG_RAW_CPL3_ONLY
compare_const BX_POLY_INTERRUPT_FLAG_STANDARD_X86_ENTRY POLY_INTERRUPT_FLAG_STANDARD_X86_ENTRY
compare_const BX_POLY_INTERRUPT_FLAG_STATE_COMPONENT_SAVE POLY_INTERRUPT_FLAG_STATE_COMPONENT_SAVE
compare_const BX_POLY_INTERRUPT_FLAG_PRECISE_FOREIGN_PC POLY_INTERRUPT_FLAG_PRECISE_FOREIGN_PC
compare_const BX_POLY_INTERRUPT_FLAG_EVENT_CHECK_BETWEEN_INSNS POLY_INTERRUPT_FLAG_EVENT_CHECK_BETWEEN_INSNS
compare_const BX_POLY_INTERRUPT_RETURN_IRET64 POLY_INTERRUPT_RETURN_IRET64
compare_const BX_POLY_INTERRUPT_RETURN_SYSRET POLY_INTERRUPT_RETURN_SYSRET
compare_const BX_POLY_INTERRUPT_RETURN_SYSEXIT POLY_INTERRUPT_RETURN_SYSEXIT
compare_const BX_POLY_INTERRUPT_RETURN_SIGNAL POLY_INTERRUPT_RETURN_SIGNAL
compare_const BX_POLY_MEMORY_ABI_VERSION POLY_MEMORY_ABI_VERSION
compare_const BX_POLY_MEMORY_MODEL_X86_TSO POLY_MEMORY_MODEL_X86_TSO
compare_const BX_POLY_MEMORY_FLAG_SHARED_X86_MEMORY POLY_MEMORY_FLAG_SHARED_X86_MEMORY
compare_const BX_POLY_MEMORY_FLAG_AARCH64_BARRIERS_NOOP POLY_MEMORY_FLAG_AARCH64_BARRIERS_NOOP
compare_const BX_POLY_MEMORY_FLAG_RISCV_FENCES_NOOP POLY_MEMORY_FLAG_RISCV_FENCES_NOOP
compare_const BX_POLY_MEMORY_FLAG_ATOMICS_COHERENT POLY_MEMORY_FLAG_ATOMICS_COHERENT
compare_const BX_POLY_MEMORY_FLAG_NO_WEAK_REORDERING POLY_MEMORY_FLAG_NO_WEAK_REORDERING
compare_const BX_POLY_TRANSITION_ABI_VERSION POLY_TRANSITION_ABI_VERSION
compare_const BX_POLY_TRANSITION_FLAG_DECODED_X86_OPCODES POLY_TRANSITION_FLAG_DECODED_X86_OPCODES
compare_const BX_POLY_TRANSITION_FLAG_NATIVE_RAW_ESCAPES POLY_TRANSITION_FLAG_NATIVE_RAW_ESCAPES
compare_const BX_POLY_TRANSITION_FLAG_PIPELINE_FLUSH POLY_TRANSITION_FLAG_PIPELINE_FLUSH
compare_const BX_POLY_TRANSITION_FLAG_BLOCK_BOUNDARY POLY_TRANSITION_FLAG_BLOCK_BOUNDARY
compare_const BX_POLY_TRANSITION_FLAG_PRECISE_NEXT_PC POLY_TRANSITION_FLAG_PRECISE_NEXT_PC
compare_const BX_POLY_TRANSITION_FLAG_FIXED_RAW_WIDTH POLY_TRANSITION_FLAG_FIXED_RAW_WIDTH
compare_const BX_POLY_TRANSITION_FLAG_NEUTRAL_FOREIGN POLY_TRANSITION_FLAG_NEUTRAL_FOREIGN
compare_const BX_POLY_TRANSITION_FLAG_NATIVE_RETURN_COOKIE POLY_TRANSITION_FLAG_NATIVE_RETURN_COOKIE
compare_const BX_POLY_TRANSITION_FLAG_TRAP_RETURN POLY_TRANSITION_FLAG_TRAP_RETURN
compare_const BX_POLY_TRANSITION_AARCH64_ALIGN POLY_TRANSITION_AARCH64_ALIGN
compare_const BX_POLY_TRANSITION_RISCV_ALIGN POLY_TRANSITION_RISCV_ALIGN
compare_const BX_POLY_ABI_BRIDGE_ABI_VERSION POLY_ABI_BRIDGE_ABI_VERSION
compare_const BX_POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64 POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64
compare_const BX_POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV
compare_const BX_POLY_ABI_BRIDGE_FLAG_SRET POLY_ABI_BRIDGE_FLAG_SRET
compare_const BX_POLY_ABI_BRIDGE_FLAG_SCALAR_FP POLY_ABI_BRIDGE_FLAG_SCALAR_FP
compare_const BX_POLY_ABI_BRIDGE_FLAG_FOCUSED_AGGREGATES POLY_ABI_BRIDGE_FLAG_FOCUSED_AGGREGATES
compare_const BX_POLY_ABI_BRIDGE_FLAG_FP64_STACK POLY_ABI_BRIDGE_FLAG_FP64_STACK
compare_const BX_POLY_ABI_BRIDGE_FLAG_DESCRIPTOR_IMPORTS POLY_ABI_BRIDGE_FLAG_DESCRIPTOR_IMPORTS
compare_const BX_POLY_ABI_BRIDGE_FLAG_TLS_BASE POLY_ABI_BRIDGE_FLAG_TLS_BASE
compare_const BX_POLY_ABI_BRIDGE_FLAG_USER_DESCRIPTORS POLY_ABI_BRIDGE_FLAG_USER_DESCRIPTORS
compare_const BX_POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK
compare_const BX_POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET
compare_const BX_POLY_ABI_BRIDGE_GPR_ARG_COUNT POLY_ABI_BRIDGE_GPR_ARG_COUNT
compare_const BX_POLY_ABI_BRIDGE_FP_ARG_COUNT POLY_ABI_BRIDGE_FP_ARG_COUNT
compare_const BX_POLY_ABI_BRIDGE_STACK_ALIGN POLY_ABI_BRIDGE_STACK_ALIGN
compare_polycall_const POLY_CPUID_BASE
compare_polycall_const POLY_IMPORT_CALL_BASE
compare_polycall_const POLY_IMPORT_CALL_STRIDE
compare_polycall_const POLY_IMPORT_X86_DESCRIPTOR_SIZE POLY_X86_IMPORT_DESCRIPTOR_SIZE
compare_polycall_const POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS
compare_polycall_const POLY_IMPORT_X86_DESCRIPTOR_RETURN_I128
compare_polycall_const POLY_IMPORT_X86_DESCRIPTOR_RETURN_FP128
compare_polycall_const POLY_ABI_BRIDGE_ABI_VERSION
compare_polycall_const POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64
compare_polycall_const POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV
compare_polycall_const POLY_ABI_BRIDGE_FLAG_SRET
compare_polycall_const POLY_ABI_BRIDGE_FLAG_SCALAR_FP
compare_polycall_const POLY_ABI_BRIDGE_FLAG_FOCUSED_AGGREGATES
compare_polycall_const POLY_ABI_BRIDGE_FLAG_FP64_STACK
compare_polycall_const POLY_ABI_BRIDGE_FLAG_DESCRIPTOR_IMPORTS
compare_polycall_const POLY_ABI_BRIDGE_FLAG_TLS_BASE
compare_polycall_const POLY_ABI_BRIDGE_FLAG_USER_DESCRIPTORS
compare_polycall_const POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK
compare_polycall_const POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET
compare_polycall_const POLY_ABI_BRIDGE_GPR_ARG_COUNT
compare_polycall_const POLY_ABI_BRIDGE_FP_ARG_COUNT
compare_polycall_const POLY_ABI_BRIDGE_STACK_ALIGN
compare_polybench_const POLY_CPUID_BASE
compare_polybench_const POLY_IMPORT_CALL_BASE
compare_polybench_const POLY_IMPORT_CALL_STRIDE
compare_polybench_const POLY_IMPORT_X86_DESCRIPTOR_SIZE POLY_X86_IMPORT_DESCRIPTOR_SIZE
compare_polybench_const POLY_IMPORT_X86_DESCRIPTOR_STACK_ARGS
compare_polybench_const POLY_IMPORT_X86_DESCRIPTOR_RETURN_I128
compare_polybench_const POLY_IMPORT_X86_DESCRIPTOR_RETURN_FP128
compare_polybench_const POLY_ABI_BRIDGE_ABI_VERSION
compare_polybench_const POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64
compare_polybench_const POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV
compare_polybench_const POLY_ABI_BRIDGE_FLAG_SRET
compare_polybench_const POLY_ABI_BRIDGE_FLAG_SCALAR_FP
compare_polybench_const POLY_ABI_BRIDGE_FLAG_FOCUSED_AGGREGATES
compare_polybench_const POLY_ABI_BRIDGE_FLAG_FP64_STACK
compare_polybench_const POLY_ABI_BRIDGE_FLAG_DESCRIPTOR_IMPORTS
compare_polybench_const POLY_ABI_BRIDGE_FLAG_TLS_BASE
compare_polybench_const POLY_ABI_BRIDGE_FLAG_USER_DESCRIPTORS
compare_polybench_const POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK
compare_polybench_const POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET
compare_polybench_const POLY_ABI_BRIDGE_GPR_ARG_COUNT
compare_polybench_const POLY_ABI_BRIDGE_FP_ARG_COUNT
compare_polybench_const POLY_ABI_BRIDGE_STACK_ALIGN

assert_contains 'state import layout version is `3`' "$POLY_ISA_DOC" \
  "poly ISA doc must describe explicit state import with layout version 3"
assert_not_contains 'state import layout version is `1`' "$POLY_ISA_DOC" \
  "poly ISA doc must not describe the old state import layout version"

poly_component="$(eval_expr "$(header_const_expr POLY_STATE_XSAVE_COMPONENT_ARCH)")"
if (( poly_component < 2 || poly_component >= 32 )); then
  fail "POLY_STATE_XSAVE_COMPONENT_ARCH=$poly_component must be an XCR0 component bit in [2,31]"
fi

assigned_xcr0_component="$(
  awk '
    /enum[[:space:]]*\{/ { in_enum = 1; next }
    in_enum && /\};/ { exit }
    in_enum && match($0, /BX_XCR0_[A-Z0-9_]+_BIT[[:space:]]*=[[:space:]]*[0-9]+/) {
      line = substr($0, RSTART, RLENGTH)
      name = line
      sub(/[[:space:]]*=.*/, "", name)
      value = line
      sub(/.*=[[:space:]]*/, "", value)
      print value " " name
    }
  ' "$BOCHS_CRREGS" | awk -v component="$poly_component" '$1 == component { print $0; exit }'
)"
if [[ -n "$assigned_xcr0_component" ]]; then
  if [[ "$assigned_xcr0_component" != *"BX_XCR0_POLY_BIT" ]]; then
    fail "POLY_STATE_XSAVE_COMPONENT_ARCH=$poly_component collides with Bochs x86 xstate component $assigned_xcr0_component"
  fi
fi

cat > "$TMP_DIR/polycpuid_layout_check.c" <<EOF
#include "$HEADER"
int main(void) { return sizeof(struct poly_xsave_state) == POLY_STATE_XSAVE_BYTES_ARCH ? 0 : 1; }
EOF

cc -std=gnu11 -Wall -Wextra -Werror -fsyntax-only \
  "$TMP_DIR/polycpuid_layout_check.c"

echo "poly CPUID contract OK"
