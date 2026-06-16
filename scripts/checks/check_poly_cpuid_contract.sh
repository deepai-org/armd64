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

# Some header enum values are aliases of earlier enum values. Bash arithmetic
# can evaluate those expressions as long as the aliased names are bound here.
readonly POLY_FRONTEND_X86=0
readonly POLY_FRONTEND_AARCH64=1
readonly POLY_FRONTEND_RISCV=2
readonly POLY_MODE_X86=0
readonly POLY_MODE_RAW_AARCH64=1
readonly POLY_MODE_RAW_RISCV=2

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

compare_aarch64_ctrl() {
  local bochs_name="$1"
  local header_subop_name="$2"
  local bochs_expr subop_expr bochs_value subop_value header_value

  bochs_expr="$(bochs_const_expr "$bochs_name")"
  subop_expr="$(header_const_expr "$header_subop_name")"
  [[ -n "$bochs_expr" ]] || fail "missing Bochs constant $bochs_name"
  [[ -n "$subop_expr" ]] || fail "missing header constant $header_subop_name"

  if [[ "$bochs_expr" =~ ^BX_POLY_AARCH64_CTRL\((.*)\)$ ]]; then
    local bochs_subop_value
    bochs_subop_value="$(eval_expr "${BASH_REMATCH[1]}")"
    bochs_value="$((0xd503201f | ((bochs_subop_value & 0x7f) << 5)))"
  else
    bochs_value="$(eval_expr "$bochs_expr")"
  fi
  subop_value="$(eval_expr "$subop_expr")"
  header_value="$((0xd503201f | ((subop_value & 0x7f) << 5)))"
  if [[ "$bochs_value" != "$header_value" ]]; then
    fail "$bochs_name=$bochs_value differs from encoded $header_subop_name=$header_value"
  fi
}

compare_riscv_ctrl() {
  local bochs_name="$1"
  local header_subop_name="$2"
  local bochs_expr subop_expr bochs_value subop_value header_value

  bochs_expr="$(bochs_const_expr "$bochs_name")"
  subop_expr="$(header_const_expr "$header_subop_name")"
  [[ -n "$bochs_expr" ]] || fail "missing Bochs constant $bochs_name"
  [[ -n "$subop_expr" ]] || fail "missing header constant $header_subop_name"

  if [[ "$bochs_expr" =~ ^BX_POLY_RISCV_CTRL\((.*)\)$ ]]; then
    local bochs_subop_value
    bochs_subop_value="$(eval_expr "${BASH_REMATCH[1]}")"
    bochs_value="$((0x0000700b | ((bochs_subop_value & 0x7f) << 25)))"
  else
    bochs_value="$(eval_expr "$bochs_expr")"
  fi
  subop_value="$(eval_expr "$subop_expr")"
  header_value="$((0x0000700b | ((subop_value & 0x7f) << 25)))"
  if [[ "$bochs_value" != "$header_value" ]]; then
    fail "$bochs_name=$bochs_value differs from encoded $header_subop_name=$header_value"
  fi
}

compare_polycall_const() {
  local header_name="$1"
  local polycall_name="${2:-$1}"
  local header_expr polycall_expr header_value polycall_value

  header_expr="$(header_const_expr "$header_name")"
  polycall_expr="$(polycall_const_expr "$polycall_name")"
  [[ -n "$header_expr" ]] || fail "missing header constant $header_name"
  if [[ -z "$polycall_expr" ]]; then
    assert_contains "\\b$polycall_name\\b" "$POLYCALL" \
      "polycall must use header constant $polycall_name"
    return
  fi

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
compare_aarch64_ctrl BX_POLY_AARCH64_CTRL_X86_ESCAPE POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE
compare_aarch64_ctrl BX_POLY_AARCH64_CTRL_TRAP_RETURN POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN
compare_aarch64_ctrl BX_POLY_AARCH64_CTRL_SWITCH_MODE POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE
compare_aarch64_ctrl BX_POLY_AARCH64_CTRL_CALL_MODE POLY_AARCH64_CTRL_SUBOP_CALL_MODE
compare_aarch64_ctrl BX_POLY_AARCH64_CTRL_CALL_SIG_MODE POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE
compare_riscv_ctrl BX_POLY_RISCV_CTRL_X86_ESCAPE POLY_RISCV_CTRL_SUBOP_X86_ESCAPE
compare_riscv_ctrl BX_POLY_RISCV_CTRL_TRAP_RETURN POLY_RISCV_CTRL_SUBOP_TRAP_RETURN
compare_riscv_ctrl BX_POLY_RISCV_CTRL_SWITCH_MODE POLY_RISCV_CTRL_SUBOP_SWITCH_MODE
compare_riscv_ctrl BX_POLY_RISCV_CTRL_CALL_MODE POLY_RISCV_CTRL_SUBOP_CALL_MODE
compare_riscv_ctrl BX_POLY_RISCV_CTRL_CALL_SIG_MODE POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE
compare_const BX_POLY_X86_CTRL_AUTO_SPILL_COUNT_STATUS POLY_X86_CTRL_AUTO_SPILL_COUNT_STATUS
compare_const BX_POLY_X86_CTRL_AUTO_SPILL_BYTES_STATUS POLY_X86_CTRL_AUTO_SPILL_BYTES_STATUS
compare_const BX_POLY_X86_CTRL_AUTO_SPILL_CYCLES_STATUS POLY_X86_CTRL_AUTO_SPILL_CYCLES_STATUS
compare_const BX_POLY_CPUID_V2_IMPLEMENTED_FEATURES POLY_CPUID_V2_IMPLEMENTED_FEATURES
compare_const BX_POLY_CPUID_V2_REQUIRED_FEATURES POLY_CPUID_V2_REQUIRED_FEATURES
compare_const BX_POLY_V2_DEBUG_NOTE_BYTES POLY_V2_DEBUG_NOTE_BYTES
compare_const BX_POLY_V2_DEBUG_NOTE_ALIGN POLY_V2_DEBUG_NOTE_ALIGN
compare_const BX_POLY_V2_DEBUG_NOTE_HEADER_BYTES POLY_V2_DEBUG_NOTE_HEADER_BYTES
compare_const BX_POLY_V2_DEBUG_NOTE_EVENT_OFFSET POLY_V2_DEBUG_NOTE_EVENT_OFFSET
compare_const BX_POLY_V2_DEBUG_NOTE_XSAVE_OFFSET POLY_V2_DEBUG_NOTE_XSAVE_OFFSET
compare_const BX_POLY_V2_MEM_PROBE_FLAGS_SUPPORTED POLY_V2_MEM_PROBE_FLAGS_SUPPORTED
compare_const BX_POLY_V2_MEM_PROBE_RESULT_FAILURE_UNMAPPED POLY_V2_MEM_PROBE_RESULT_FAILURE_UNMAPPED
compare_const BX_POLY_V2_MEM_PROBE_RESULT_FAILURE_PERMISSION POLY_V2_MEM_PROBE_RESULT_FAILURE_PERMISSION
compare_const BX_POLY_V2_MEM_PROBE_RESULT_FAILURE_NONCANONICAL POLY_V2_MEM_PROBE_RESULT_FAILURE_NONCANONICAL
compare_const BX_POLY_V2_DERIVE_DESC_BYTES POLY_V2_DERIVE_DESC_BYTES
compare_const BX_POLY_V2_DERIVE_DESC_ALIGN POLY_V2_DERIVE_DESC_ALIGN
compare_const BX_POLY_V2_DERIVE_DESC_HEADER_BYTES POLY_V2_DERIVE_DESC_HEADER_BYTES
compare_const BX_POLY_V2_DERIVE_FLAGS_SUPPORTED POLY_V2_DERIVE_FLAGS_SUPPORTED
compare_const BX_POLY_IMPORT_SELECTOR_COUNT POLY_IMPORT_SELECTOR_COUNT
compare_const BX_POLY_CPUID_STATE_OVERLAP_GPRS POLY_CPUID_STATE_OVERLAP_GPRS
compare_const BX_POLY_CPUID_STATE_USER_RETURN_RESTORE POLY_CPUID_STATE_USER_RETURN_RESTORE
compare_const BX_POLY_CPUID_STATE_X86_TSO POLY_CPUID_STATE_X86_TSO
compare_const BX_POLY_CPUID_STATE_XSAVE_VISIBLE POLY_CPUID_STATE_XSAVE_VISIBLE
compare_const BX_POLY_CPUID_STATE_KEY_EXPLICIT POLY_CPUID_STATE_KEY_EXPLICIT
compare_const BX_POLY_CPUID_STATE_TRANSITION_FRAME_32 POLY_CPUID_STATE_TRANSITION_FRAME_32
compare_const BX_POLY_CPUID_STATE_EXPLICIT_SAVE_RESTORE POLY_CPUID_STATE_EXPLICIT_SAVE_RESTORE
compare_const BX_POLY_CPUID_STATE_XSAVE_ARCH_CONTRACT POLY_CPUID_STATE_XSAVE_ARCH_CONTRACT
compare_const BX_POLY_CPUID_STATE_USER_SPILL POLY_CPUID_STATE_USER_SPILL
compare_const BX_POLY_CPUID_STATE_MONITOR_TRAMPOLINE POLY_CPUID_STATE_MONITOR_TRAMPOLINE
compare_const BX_POLY_CPUID_STATE_OS_XSAVE_NOT_REQUIRED POLY_CPUID_STATE_OS_XSAVE_NOT_REQUIRED
assert_not_contains "regs\\.(eax|ebx|ecx|edx)[[:space:]]*=[[:space:]]*POLY_.*MONITOR_PACKET" "$HEADER" \
  "public CPUID escape helpers must not advertise retired monitor-packet controls"
assert_not_contains "POLY_(AARCH64|RISCV|X86)_CTRL_(SUBOP_)?MONITOR_PACKET" "$HEADER" \
  "public header must not expose retired monitor-packet control opcodes"
assert_not_contains "POLY_CPUID_STATE_MONITOR_PACKET_XSAVE" "$HEADER" \
  "public header must not expose retired monitor-packet XSAVE state constants"
assert_not_contains "POLY_STATE_XSAVE_FLAG_MONITOR_PACKET" "$HEADER" \
  "public header must not expose retired monitor-packet XSAVE flag constants"
assert_not_contains "POLY_TRAP_PACKET_FLAG_MONITOR_MEMORY" "$HEADER" \
  "public header must not expose retired monitor-packet trap flag constants"
assert_not_contains "POLY_CPUID_STATE_MONITOR_PACKET_XSAVE[[:space:]]*\\|" "$HEADER" \
  "public CPUID state leaf must not advertise retired monitor-packet XSAVE state"
assert_not_contains "POLY_STATE_XSAVE_FLAG_MONITOR_PACKET[[:space:]]*\\|" "$HEADER" \
  "public XSAVE arch-state leaf must not advertise retired monitor-packet state"
assert_not_contains "\\b[RE][ABCD]X[[:space:]]*=[[:space:]]*BX_POLY_.*MONITOR_PACKET" "$BOCHS_CPU" \
  "Bochs CPUID leaves must not advertise retired monitor-packet controls"
assert_not_contains "BX_POLY_(AARCH64|RISCV|X86)_CTRL_MONITOR_PACKET" "$BOCHS_CPU" \
  "Bochs must not retain retired monitor-packet control opcodes"
assert_not_contains "BX_POLY_CPUID_STATE_MONITOR_PACKET_XSAVE" "$BOCHS_CPU" \
  "Bochs must not retain retired monitor-packet XSAVE state constants"
assert_not_contains "BX_POLY_STATE_XSAVE_FLAG_MONITOR_PACKET" "$BOCHS_CPU" \
  "Bochs must not retain retired monitor-packet XSAVE flag constants"
assert_not_contains "BX_POLY_TRAP_PACKET_FLAG_MONITOR_MEMORY[[:space:]]*\\|" "$BOCHS_CPU" \
  "Bochs CPUID trap leaf must not advertise retired monitor-packet trap flags"
assert_not_contains "BX_POLY_CPUID_STATE_MONITOR_PACKET_XSAVE[[:space:]]*\\|" "$BOCHS_CPU" \
  "Bochs CPUID state leaf must not advertise retired monitor-packet XSAVE state"
assert_not_contains "BX_POLY_STATE_XSAVE_FLAG_MONITOR_PACKET[[:space:]]*\\|" "$BOCHS_CPU" \
  "Bochs XSAVE arch-state leaf must not advertise retired monitor-packet state"
compare_const BX_POLY_STATE_XSAVE_MAGIC POLY_STATE_XSAVE_MAGIC
compare_const BX_POLY_STATE_XSAVE_COMPONENT_ARCH POLY_STATE_XSAVE_COMPONENT_ARCH
compare_const BX_POLY_STATE_XSAVE_BYTES_ARCH POLY_STATE_XSAVE_BYTES_ARCH
compare_const BX_POLY_STATE_XSAVE_ALIGN_ARCH POLY_STATE_XSAVE_ALIGN_ARCH
compare_const BX_POLY_STATE_XSAVE_LAYOUT_VERSION POLY_STATE_XSAVE_LAYOUT_VERSION
compare_const BX_POLY_STATE_XSAVE_FLAG_XCR0_USER POLY_STATE_XSAVE_FLAG_XCR0_USER
compare_const BX_POLY_STATE_XSAVE_FLAG_OSXSAVE_REQUIRED POLY_STATE_XSAVE_FLAG_OSXSAVE_REQUIRED
compare_const BX_POLY_STATE_XSAVE_FLAG_INTERRUPT_RESUME POLY_STATE_XSAVE_FLAG_INTERRUPT_RESUME
compare_const BX_POLY_STATE_XSAVE_FLAG_TRAP_STATE POLY_STATE_XSAVE_FLAG_TRAP_STATE
compare_const BX_POLY_STATE_XSAVE_FLAG_COMPLETE_BANK_EXPORT POLY_STATE_XSAVE_FLAG_COMPLETE_BANK_EXPORT
compare_const BX_POLY_STATE_XSAVE_FLAG_IMPORT_RETURN POLY_STATE_XSAVE_FLAG_IMPORT_RETURN
compare_const BX_POLY_STATE_XSAVE_FLAG_ABI_SIGNATURES POLY_STATE_XSAVE_FLAG_ABI_SIGNATURES
compare_const BX_POLY_STATE_XSAVE_FLAG_CROSS_RETURN POLY_STATE_XSAVE_FLAG_CROSS_RETURN
compare_const BX_POLY_STATE_XSAVE_FLAG_FRONTEND_TLS POLY_STATE_XSAVE_FLAG_FRONTEND_TLS
compare_const BX_POLY_STATE_XSAVE_FLAG_LANDING_POLICY POLY_STATE_XSAVE_FLAG_LANDING_POLICY
compare_const BX_POLY_STATE_XSAVE_FLAG_STATE_KEY POLY_STATE_XSAVE_FLAG_STATE_KEY
compare_const BX_POLY_STATE_XSAVE_FLAG_USER_SPILL POLY_STATE_XSAVE_FLAG_USER_SPILL
compare_const BX_POLY_STATE_XSAVE_FLAG_MONITOR_TRAMPOLINE POLY_STATE_XSAVE_FLAG_MONITOR_TRAMPOLINE
compare_const BX_POLY_STATE_XSAVE_FLAG_OS_XSAVE_NOT_REQUIRED POLY_STATE_XSAVE_FLAG_OS_XSAVE_NOT_REQUIRED
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
assert_not_contains "\\bPOLY_TRAP_PACKET_ARG_COUNT\\b" "$HEADER" \
  "retired trap-packet arg count must not remain in the public header"
assert_contains "\\bPOLY_V2_EVENT_ARG_COUNT\\b" "$HEADER" \
  "v2 event-frame arg count must size active trap/event argument lanes"
compare_const BX_POLY_TRAP_PACKET_FLAG_VECTOR_DELIVERY POLY_TRAP_PACKET_FLAG_VECTOR_DELIVERY
compare_const BX_POLY_TRAP_PACKET_FLAG_NO_VECTOR_X86_EXCEPTIONS POLY_TRAP_PACKET_FLAG_NO_VECTOR_X86_EXCEPTIONS
compare_const BX_POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE
compare_const BX_POLY_TRAP_PACKET_FLAG_ALL_FRONTEND_HANDLERS POLY_TRAP_PACKET_FLAG_ALL_FRONTEND_HANDLERS
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
compare_const BX_POLY_TRANSITION_FLAG_NATIVE_FRONTEND_WIDTHS POLY_TRANSITION_FLAG_NATIVE_FRONTEND_WIDTHS
compare_const BX_POLY_TRANSITION_FLAG_NEUTRAL_FOREIGN POLY_TRANSITION_FLAG_NEUTRAL_FOREIGN
compare_const BX_POLY_TRANSITION_FLAG_NATIVE_RETURN_COOKIE POLY_TRANSITION_FLAG_NATIVE_RETURN_COOKIE
compare_const BX_POLY_TRANSITION_FLAG_TRAP_RETURN POLY_TRANSITION_FLAG_TRAP_RETURN
compare_const BX_POLY_TRANSITION_FLAG_INTERRUPTED_RAW POLY_TRANSITION_FLAG_INTERRUPTED_RAW
compare_const BX_POLY_TRANSITION_FLAG_LANDING_PADS POLY_TRANSITION_FLAG_LANDING_PADS
compare_const BX_POLY_TRANSITION_FLAG_LANDING_POLICY POLY_TRANSITION_FLAG_LANDING_POLICY
compare_const BX_POLY_TRANSITION_AARCH64_ALIGN POLY_TRANSITION_AARCH64_ALIGN
compare_const BX_POLY_TRANSITION_RISCV_ALIGN POLY_TRANSITION_RISCV_ALIGN
compare_const BX_POLY_ABI_BRIDGE_ABI_VERSION POLY_ABI_BRIDGE_ABI_VERSION
compare_const BX_POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64 POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64
compare_const BX_POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV
compare_const BX_POLY_ABI_BRIDGE_FLAG_SRET POLY_ABI_BRIDGE_FLAG_SRET
compare_const BX_POLY_ABI_BRIDGE_FLAG_SCALAR_FP POLY_ABI_BRIDGE_FLAG_SCALAR_FP
compare_const BX_POLY_ABI_BRIDGE_FLAG_REGISTER_ONLY_AGGREGATES POLY_ABI_BRIDGE_FLAG_REGISTER_ONLY_AGGREGATES
compare_const BX_POLY_ABI_BRIDGE_FLAG_HARDWARE_STACK_ARGS POLY_ABI_BRIDGE_FLAG_HARDWARE_STACK_ARGS
compare_const BX_POLY_ABI_BRIDGE_FLAG_TLS_BASE POLY_ABI_BRIDGE_FLAG_TLS_BASE
compare_const BX_POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK
compare_const BX_POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET
compare_const BX_POLY_ABI_BRIDGE_GPR_ARG_COUNT POLY_ABI_BRIDGE_GPR_ARG_COUNT
compare_const BX_POLY_ABI_BRIDGE_FP_ARG_COUNT POLY_ABI_BRIDGE_FP_ARG_COUNT
compare_const BX_POLY_ABI_BRIDGE_STACK_ALIGN POLY_ABI_BRIDGE_STACK_ALIGN
compare_polycall_const POLY_CPUID_BASE
compare_polycall_const POLY_IMPORT_CALL_BASE
compare_polycall_const POLY_ABI_BRIDGE_ABI_VERSION
compare_polycall_const POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64
compare_polycall_const POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV
compare_polycall_const POLY_ABI_BRIDGE_FLAG_SRET
compare_polycall_const POLY_ABI_BRIDGE_FLAG_SCALAR_FP
compare_polycall_const POLY_ABI_BRIDGE_FLAG_REGISTER_ONLY_AGGREGATES
compare_polycall_const POLY_ABI_BRIDGE_FLAG_HARDWARE_STACK_ARGS
compare_polycall_const POLY_ABI_BRIDGE_FLAG_HARDWARE_IMPORT_DESCRIPTORS
compare_polycall_const POLY_ABI_BRIDGE_FLAG_TLS_BASE
compare_polycall_const POLY_ABI_BRIDGE_FLAG_HARDWARE_USER_DESCRIPTORS
compare_polycall_const POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK
compare_polycall_const POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET
compare_polycall_const POLY_ABI_BRIDGE_GPR_ARG_COUNT
compare_polycall_const POLY_ABI_BRIDGE_FP_ARG_COUNT
compare_polycall_const POLY_ABI_BRIDGE_STACK_ALIGN

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
