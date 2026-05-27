#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOCHS_CPU="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"
BOCHS_DIR="$ROOT_DIR/bochs-prepoly-src/bochs"
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

if grep -R -I -n -E "BXPN_POLY_COMPAT_TRAPS|poly_compat_traps|compat_traps" \
    --exclude=config.cc --exclude=param_names.h "$BOCHS_DIR" \
    > "$TMP_DIR/compat-uses"; then
  cat "$TMP_DIR/compat-uses" >&2
  fail "deprecated compat trap knob must not be used by CPU execution code"
fi

echo "poly architecture contract OK"
