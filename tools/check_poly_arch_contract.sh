#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOCHS_CPU="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"
BOCHS_DIR="$ROOT_DIR/bochs-prepoly-src/bochs"
POLYPROBE="$ROOT_DIR/tools/polyprobe.c"
TMP_DIR="${TMPDIR:-/tmp}/poly-arch-contract.$$"

mkdir -p "$TMP_DIR"
trap 'rm -rf "$TMP_DIR"' EXIT

fail() {
  echo "poly architecture contract check failed: $*" >&2
  exit 1
}

extract_function() {
  local name="$1"
  local out="$2"

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
  ' "$BOCHS_CPU" > "$out"

  [[ -s "$out" ]] || fail "could not extract $name"
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

if grep -R -I -n -E "BXPN_POLY_COMPAT_TRAPS|poly_compat_traps|compat_traps" \
    --exclude=config.cc --exclude=param_names.h "$BOCHS_DIR" \
    > "$TMP_DIR/compat-uses"; then
  cat "$TMP_DIR/compat-uses" >&2
  fail "deprecated compat trap knob must not be used by CPU execution code"
fi

echo "poly architecture contract OK"
