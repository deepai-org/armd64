#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOCHS_CPU="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"
BOCHS_EXCEPTION="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/exception.cc"
BOCHS_CTRL_XFER64="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/ctrl_xfer64.cc"
BOCHS_FETCHDECODE32="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/decoder/fetchdecode32.cc"
BOCHS_FETCHDECODE64="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/decoder/fetchdecode64.cc"
BOCHS_OPMAP="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/decoder/fetchdecode_opmap.h"
BOCHS_OPCODES="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/decoder/ia_opcodes.def"
BOCHS_DIR="$ROOT_DIR/bochs-prepoly-src/bochs"
POLYPROBE="$ROOT_DIR/tools/polyprobe.c"
POLYBENCH="$ROOT_DIR/tools/polybench.c"
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

assert_contains "BxOpcodeTable0F24\\[\\].*BX_IA_POLYMODE" "$BOCHS_OPMAP" \
  "x86 poly opcode family must be decoded as BX_IA_POLYMODE, not #UD"
assert_contains "0F 24.*decoder_creg32.*BxOpcodeTable0F24" "$BOCHS_FETCHDECODE32" \
  "32-bit x86 decode must route 0f 24 to the POLYMODE opcode table"
assert_contains "0F 24.*decoder_creg64.*BxOpcodeTable0F24" "$BOCHS_FETCHDECODE64" \
  "64-bit x86 decode must route 0f 24 to the POLYMODE opcode table"
assert_contains "BX_IA_POLYMODE.*BX_CPU_C::POLYMODE" "$BOCHS_OPCODES" \
  "BX_IA_POLYMODE must dispatch to the dedicated POLYMODE handler"

BXERROR_FUNC="$TMP_DIR/BxError.cc"
UNDEFINED_FUNC="$TMP_DIR/UndefinedOpcode.cc"
POLYMODE_FUNC="$TMP_DIR/POLYMODE.cc"
extract_function "BxError" "$BXERROR_FUNC"
extract_function "UndefinedOpcode" "$UNDEFINED_FUNC"
extract_function "POLYMODE" "$POLYMODE_FUNC"
assert_contains "handle_poly_opcode" "$POLYMODE_FUNC" \
  "POLYMODE must handle x86 poly opcodes through the dedicated decoded path"
assert_not_contains "handle_poly_(opcode|ud)" "$BXERROR_FUNC" \
  "BxError must not mask decoder regressions by accepting poly opcodes from #UD"
assert_not_contains "handle_poly_(opcode|ud)" "$UNDEFINED_FUNC" \
  "UndefinedOpcode must not mask decoder regressions by accepting poly opcodes from #UD"

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

CROSS_A64_FUNC="$TMP_DIR/execute_poly_raw_aarch64.cc"
CROSS_RV_FUNC="$TMP_DIR/execute_poly_raw_riscv.cc"
CROSS_ENTER_FUNC="$TMP_DIR/enter_poly_cross_call.cc"
CROSS_RETURN_FUNC="$TMP_DIR/return_poly_cross_call.cc"
extract_function "execute_poly_raw_aarch64" "$CROSS_A64_FUNC"
extract_function "execute_poly_raw_riscv" "$CROSS_RV_FUNC"
extract_function "enter_poly_cross_call" "$CROSS_ENTER_FUNC"
extract_function "return_poly_cross_call" "$CROSS_RETURN_FUNC"
assert_contains "BX_POLY_AARCH64_BRK_RISCV_SWITCH" "$CROSS_A64_FUNC" \
  "AArch64 raw decoder must recognize the direct RISC-V switch opcode"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*BX_POLY_MODE_RAW_RISCV" "$CROSS_A64_FUNC" \
  "AArch64 direct switch must enter RISC-V without routing through x86"
assert_contains "BX_POLY_AARCH64_BRK_RISCV_CALL" "$CROSS_A64_FUNC" \
  "AArch64 raw decoder must recognize the native RISC-V call gate"
assert_contains "read_poly_aarch64_reg\\(16" "$CROSS_A64_FUNC" \
  "AArch64 cross-call gate must take the RISC-V target from x16"
assert_contains "read_poly_aarch64_reg\\(17" "$CROSS_A64_FUNC" \
  "AArch64 cross-call gate must take the AArch64 return PC from x17"
assert_contains "BX_POLY_MODE_RAW_AARCH64" "$CROSS_A64_FUNC" \
  "AArch64 cross-call gate must record AArch64 as the caller mode"
assert_contains "BX_POLY_MODE_RAW_RISCV" "$CROSS_A64_FUNC" \
  "AArch64 cross-call gate must record RISC-V as the callee mode"
assert_contains "BX_POLY_RISCV_AARCH64_SWITCH" "$CROSS_RV_FUNC" \
  "RISC-V raw decoder must recognize the direct AArch64 switch opcode"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*BX_POLY_MODE_RAW_AARCH64" "$CROSS_RV_FUNC" \
  "RISC-V direct switch must enter AArch64 without routing through x86"
assert_contains "BX_POLY_RISCV_AARCH64_CALL" "$CROSS_RV_FUNC" \
  "RISC-V raw decoder must recognize the native AArch64 call gate"
assert_contains "read_poly_riscv_reg\\(5" "$CROSS_RV_FUNC" \
  "RISC-V cross-call gate must take the AArch64 target from x5/t0"
assert_contains "read_poly_riscv_reg\\(6" "$CROSS_RV_FUNC" \
  "RISC-V cross-call gate must take the RISC-V return PC from x6/t1"
assert_contains "BX_POLY_MODE_RAW_RISCV" "$CROSS_RV_FUNC" \
  "RISC-V cross-call gate must record RISC-V as the caller mode"
assert_contains "BX_POLY_MODE_RAW_AARCH64" "$CROSS_RV_FUNC" \
  "RISC-V cross-call gate must record AArch64 as the callee mode"
assert_contains "write_poly_aarch64_reg\\(30,[[:space:]]*BX_POLY_CROSS_RETURN_COOKIE\\)" "$CROSS_ENTER_FUNC" \
  "cross-call entry must use the native AArch64 link register return cookie"
assert_contains "write_poly_riscv_reg\\(1,[[:space:]]*BX_POLY_CROSS_RETURN_COOKIE\\)" "$CROSS_ENTER_FUNC" \
  "cross-call entry must use the native RISC-V ra return cookie"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*callee_mode" "$CROSS_ENTER_FUNC" \
  "cross-call entry must switch directly to the callee frontend"
assert_contains "RIP[[:space:]]*=[[:space:]]*target_rip" "$CROSS_ENTER_FUNC" \
  "cross-call entry must branch directly to the foreign target"
assert_contains "BX_ASYNC_EVENT_STOP_TRACE" "$CROSS_ENTER_FUNC" \
  "cross-call entry must split the current trace before the callee frontend runs"
assert_not_contains "BX_POLY_MODE_X86|return_poly_abi_call|deliver_poly_architectural_trap|handle_poly_foreign_syscall" \
  "$CROSS_ENTER_FUNC" \
  "cross-call entry must not route native foreign-to-foreign calls through x86 policy"
assert_contains "target_rip[[:space:]]*!=[[:space:]]*\\(bx_address\\)[[:space:]]*BX_POLY_CROSS_RETURN_COOKIE" "$CROSS_RETURN_FUNC" \
  "cross-call return must require the native return-cookie target"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*frame->caller_mode" "$CROSS_RETURN_FUNC" \
  "cross-call return must restore the caller frontend directly"
assert_contains "RIP[[:space:]]*=[[:space:]]*frame->return_rip" "$CROSS_RETURN_FUNC" \
  "cross-call return must resume the caller's native return PC"
assert_contains "BX_ASYNC_EVENT_STOP_TRACE" "$CROSS_RETURN_FUNC" \
  "cross-call return must split the trace before caller frontend resumes"
assert_not_contains "BX_POLY_MODE_X86|return_poly_abi_call|deliver_poly_architectural_trap|handle_poly_foreign_syscall" \
  "$CROSS_RETURN_FUNC" \
  "cross-call return must not route native foreign-to-foreign returns through x86 policy"
assert_contains "0xd42fffc0" "$POLYPROBE" \
  "polyprobe must exercise AArch64-to-RISC-V direct switch"
assert_contains "0x0000002b" "$POLYPROBE" \
  "polyprobe must exercise RISC-V-to-AArch64 direct switch"
assert_contains "aarch64-to-riscv" "$POLYBENCH" \
  "polybench must cover AArch64-to-RISC-V mixed execution"
assert_contains "riscv-to-aarch64" "$POLYBENCH" \
  "polybench must cover RISC-V-to-AArch64 mixed execution"
assert_contains "POLYBENCH_CROSS_CALL_RESULT" "$POLYBENCH" \
  "polybench must cover native cross-frontend call/return"

if grep -R -I -n -E "BXPN_POLY_COMPAT_TRAPS|poly_compat_traps|compat_traps" \
    --exclude=config.cc --exclude=param_names.h "$BOCHS_DIR" \
    > "$TMP_DIR/compat-uses"; then
  cat "$TMP_DIR/compat-uses" >&2
  fail "deprecated compat trap knob must not be used by CPU execution code"
fi

echo "poly architecture contract OK"
