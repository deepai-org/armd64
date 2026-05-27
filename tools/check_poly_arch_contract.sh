#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOCHS_CPU="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"
BOCHS_EXCEPTION="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/exception.cc"
BOCHS_CTRL_XFER64="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/ctrl_xfer64.cc"
BOCHS_DIR="$ROOT_DIR/bochs-prepoly-src/bochs"
POLYPROBE="$ROOT_DIR/tools/polyprobe.c"
TMP_DIR="${TMPDIR:-/tmp}/poly-arch-contract.$$"

mkdir -p "$TMP_DIR"
trap 'rm -rf "$TMP_DIR"' EXIT

fail() {
  echo "poly architecture contract check failed: $*" >&2
  exit 1
}

extract_function_from_file() {
  local name="$1"
  local file="$2"
  local out="$3"

  awk -v name="$name" '
    index($0, "BX_CPU_C::" name "(") {
      in_sig = 1
    }
    in_sig {
      print
      if (index($0, "{")) {
        in_func = 1
        in_sig = 0
      }
      next
    }
    in_func {
      print
      if ($0 ~ /^}/)
        exit
    }
  ' "$file" > "$out"

  [[ -s "$out" ]] || fail "could not extract $name"
}

extract_function() {
  extract_function_from_file "$1" "$BOCHS_CPU" "$2"
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

SYSCALL_FUNC="$TMP_DIR/handle_poly_foreign_syscall.cc"
extract_function "handle_poly_foreign_syscall" "$SYSCALL_FUNC"
assert_contains "bx_poly_record_syscall_trap" "$SYSCALL_FUNC" \
  "foreign syscalls must record an architectural trap packet"
assert_contains "deliver_poly_architectural_trap" "$SYSCALL_FUNC" \
  "foreign syscalls must exit through the architectural trap path"
assert_not_contains "write_poly_(aarch64|riscv)_reg|RAX[[:space:]]*=|read_virtual_|write_virtual_|switch[[:space:]]*\\(|case[[:space:]]" \
  "$SYSCALL_FUNC" \
  "foreign syscall handler must not synthesize guest results or decode Linux policy"

assert_not_contains "poly_raw: import x86 call|BX_POLY_IMPORT_X86_ADD_HELPER_SIZE|BX_POLY_IMPORT_FUNC_X86_ADD" \
  "$BOCHS_CPU" \
  "legacy fixed x86 import helper fallback must stay removed"

assert_contains "bx_poly_aarch64_barrier_name" "$BOCHS_CPU" \
  "AArch64 barrier decoder must remain present for the x86 TSO contract"
assert_contains "0xd503309f" "$BOCHS_CPU" \
  "AArch64 DSB barrier mask must remain decoded"
assert_contains "0xd50330bf" "$BOCHS_CPU" \
  "AArch64 DMB barrier mask must remain decoded"
assert_contains "0xd50330df" "$BOCHS_CPU" \
  "AArch64 ISB barrier mask must remain decoded"
assert_contains "bx_poly_riscv_fence_name" "$BOCHS_CPU" \
  "RISC-V fence decoder must remain present for the x86 TSO contract"
assert_contains "0x0000000f" "$BOCHS_CPU" \
  "RISC-V FENCE must remain decoded"
assert_contains "0x0000100f" "$BOCHS_CPU" \
  "RISC-V FENCE.I must remain decoded"
assert_contains "aarch64 .*x86-tso no-op" "$BOCHS_CPU" \
  "AArch64 barriers must remain explicit x86-TSO no-ops"
assert_contains "riscv .*x86-tso no-op" "$BOCHS_CPU" \
  "RISC-V fences must remain explicit x86-TSO no-ops"
assert_contains "0xd5033fbf|0xd5033f9f|0xd5033fdf" "$POLYPROBE" \
  "polyprobe must exercise AArch64 DMB/DSB/ISB barrier decode"
assert_contains "0x0ff0000f" "$POLYPROBE" \
  "polyprobe must exercise RISC-V FENCE decode"
assert_contains "0x0000100f" "$POLYPROBE" \
  "polyprobe must exercise RISC-V FENCE.I decode"

INTERRUPT_FUNC="$TMP_DIR/poly_interrupt_enter.cc"
RESTORE_FUNC="$TMP_DIR/poly_restore_raw_return_to_user.cc"
IRET64_FUNC="$TMP_DIR/IRET64.cc"
SYSRET_FUNC="$TMP_DIR/SYSRET.cc"
SYSEXIT_FUNC="$TMP_DIR/SYSEXIT.cc"
extract_function "poly_interrupt_enter" "$INTERRUPT_FUNC"
extract_function "poly_restore_raw_return_to_user" "$RESTORE_FUNC"
extract_function_from_file "IRET64" "$BOCHS_CTRL_XFER64" "$IRET64_FUNC"
extract_function "SYSRET" "$SYSRET_FUNC"
extract_function "SYSEXIT" "$SYSEXIT_FUNC"
assert_contains "CPL[[:space:]]*!=[[:space:]]*3" "$INTERRUPT_FUNC" \
  "raw interrupt capture must be restricted to userspace foreign execution"
assert_contains "bx_poly_is_raw_mode\\(bx_poly_current_mode\\)" "$INTERRUPT_FUNC" \
  "raw interrupt capture must only arm for raw foreign frontends"
assert_contains "bx_poly_interrupted_raw_valid[[:space:]]*=[[:space:]]*true" "$INTERRUPT_FUNC" \
  "raw interrupt capture must mark interrupted foreign state valid"
assert_contains "bx_poly_interrupted_raw_mode[[:space:]]*=[[:space:]]*bx_poly_current_mode" "$INTERRUPT_FUNC" \
  "raw interrupt capture must save interrupted foreign frontend mode"
assert_contains "bx_poly_interrupted_raw_rip[[:space:]]*=[[:space:]]*RIP" "$INTERRUPT_FUNC" \
  "raw interrupt capture must save interrupted foreign RIP"
assert_contains "bx_poly_save_current_reg_state" "$INTERRUPT_FUNC" \
  "raw interrupt capture must save synthetic foreign state before x86 kernel entry"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*BX_POLY_MODE_X86" "$INTERRUPT_FUNC" \
  "raw interrupt capture must route interrupt handling through x86 decode"
assert_contains "bx_poly_update_raw_owner" "$INTERRUPT_FUNC" \
  "raw interrupt capture must update the keyed raw owner state"
assert_contains "CPL[[:space:]]*!=[[:space:]]*3" "$RESTORE_FUNC" \
  "raw interrupt restore must only run on return to userspace"
assert_contains "bx_poly_interrupted_raw_valid" "$RESTORE_FUNC" \
  "raw interrupt restore must require an armed interrupted foreign state"
assert_contains "bx_poly_is_raw_mode\\(bx_poly_interrupted_raw_mode\\)" "$RESTORE_FUNC" \
  "raw interrupt restore must require a recorded raw foreign mode"
assert_contains "bx_poly_interrupted_raw_rip[[:space:]]*!=[[:space:]]*RIP" "$RESTORE_FUNC" \
  "raw interrupt restore must only resume when IRET/SYSRET reaches the recorded RIP"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*bx_poly_interrupted_raw_mode" "$RESTORE_FUNC" \
  "raw interrupt restore must switch back to the recorded foreign frontend"
assert_contains "bx_poly_interrupted_raw_valid[[:space:]]*=[[:space:]]*false" "$RESTORE_FUNC" \
  "raw interrupt restore must consume the interrupted foreign state"
assert_contains "bx_poly_commit_reg_state" "$RESTORE_FUNC" \
  "raw interrupt restore must commit the keyed synthetic bank"
assert_contains "BX_ASYNC_EVENT_STOP_TRACE" "$RESTORE_FUNC" \
  "raw interrupt restore must split the current x86 trace before raw fetch resumes"
assert_contains "poly_interrupt_enter\\(\\)" "$BOCHS_EXCEPTION" \
  "x86 interrupt delivery must invoke raw foreign interrupt capture"
assert_contains "poly_iret_return_to_user\\(\\)" "$IRET64_FUNC" \
  "IRET64 return must invoke raw foreign frontend restore"
assert_contains "poly_sysret_return_to_user\\(\\)" "$SYSRET_FUNC" \
  "SYSRET return must invoke raw foreign frontend restore"
assert_contains "poly_sysexit_return_to_user\\(\\)" "$SYSEXIT_FUNC" \
  "SYSEXIT return must invoke raw foreign frontend restore"

if grep -R -I -n -E "BXPN_POLY_COMPAT_TRAPS|poly_compat_traps|compat_traps" \
    --exclude=config.cc --exclude=param_names.h "$BOCHS_DIR" \
    > "$TMP_DIR/compat-uses"; then
  cat "$TMP_DIR/compat-uses" >&2
  fail "deprecated compat trap knob must not be used by CPU execution code"
fi

echo "poly architecture contract OK"
