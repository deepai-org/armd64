#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BOCHS_SRC="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"
TOOLS_SRC="$ROOT_DIR/tools/runtime/polycall.c"
HEADER_SRC="$ROOT_DIR/tools/include/polycpuid.h"

extract_const() {
  local src="$1"
  local name="$2"

  awk -v name="$name" '
    {
      line = $0
      sub(/\/\/.*/, "", line)
      pattern = name "[ \t]*="
      if (line !~ pattern)
        next

      value = line
      sub(/.*=[ \t]*/, "", value)
      sub(/[;,].*/, "", value)
      gsub(/[()UuLl]/, "", value)
      gsub(/^[ \t]+|[ \t]+$/, "", value)
      print value
      exit
    }
  ' "$src"
}

compare_const() {
  local left_src="$1"
  local left_name="$2"
  local right_src="$3"
  local right_name="$4"

  local left_value
  local right_value
  left_value="$(extract_const "$left_src" "$left_name")"
  right_value="$(extract_const "$right_src" "$right_name")"

  if [[ -z "$left_value" || -z "$right_value" ]]; then
    echo "failed to extract import constant: $left_name or $right_name" >&2
    exit 1
  fi

  if [[ "$left_value" != "$right_value" ]]; then
    echo "poly import constant mismatch: $left_name=$left_value $right_name=$right_value" >&2
    exit 1
  fi
}

validate_dense_runtime_ids() {
  local src="$1"
  local count="$2"

  awk -v count="$count" '
    BEGIN {
      fail = 0
    }
    {
      line = $0
      sub(/\/\/.*/, "", line)
      if (line !~ /POLY_IMPORT_FUNC_[A-Z0-9_]+[ \t]*=/)
        next

      name = line
      sub(/.*POLY_IMPORT_FUNC_/, "POLY_IMPORT_FUNC_", name)
      sub(/[ \t]*=.*/, "", name)
      if (name == "POLY_IMPORT_FUNC_COUNT")
        next

      value = line
      sub(/.*=[ \t]*/, "", value)
      sub(/[;,].*/, "", value)
      gsub(/^[ \t]+|[ \t]+$/, "", value)
      if (value !~ /^[0-9]+$/)
        next

      numeric = value + 0
      if (numeric >= count) {
        printf("runtime import ID out of range: %s=%u count=%u\n",
          name, numeric, count) > "/dev/stderr"
        fail = 1
      }
      if (numeric in seen) {
        printf("duplicate runtime import ID %u: %s and %s\n",
          numeric, seen[numeric], name) > "/dev/stderr"
        fail = 1
      }
      seen[numeric] = name
      actual++
    }
    END {
      for (id = 0; id < count; id++) {
        if (!(id in seen)) {
          printf("missing runtime import ID %u below count %u\n",
            id, count) > "/dev/stderr"
          fail = 1
        }
      }
      if (actual != count) {
        printf("runtime import ID count mismatch: declared=%u actual=%u\n",
          count, actual) > "/dev/stderr"
        fail = 1
      }
      exit fail
    }
  ' "$src"
}

ensure_bochs_import_surface_is_generic() {
  awk '
    {
      line = $0
      sub(/\/\/.*/, "", line)
      if (line !~ /BX_POLY_IMPORT_FUNC_[A-Z0-9_]+[ \t]*=/)
        next

      name = line
      sub(/.*BX_POLY_IMPORT_FUNC_/, "BX_POLY_IMPORT_FUNC_", name)
      sub(/[ \t]*=.*/, "", name)
      if (name != "BX_POLY_IMPORT_FUNC_X86_SLOT0" &&
          name != "BX_POLY_IMPORT_FUNC_X86_SLOT7") {
        printf("Bochs must not expose software import selector %s\n",
          name) > "/dev/stderr"
        fail = 1
      }
    }
    END {
      exit fail
    }
  ' "$BOCHS_SRC"
}

compare_const "$BOCHS_SRC" "BX_POLY_IMPORT_FUNC_X86_SLOT0" \
  "$TOOLS_SRC" "POLY_IMPORT_FUNC_X86_SLOT0"
compare_const "$BOCHS_SRC" "BX_POLY_IMPORT_FUNC_X86_SLOT7" \
  "$TOOLS_SRC" "POLY_IMPORT_FUNC_X86_SLOT7"
compare_const "$BOCHS_SRC" "BX_POLY_IMPORT_CALL_COUNT" \
  "$TOOLS_SRC" "POLY_IMPORT_FUNC_COUNT"

compare_const "$HEADER_SRC" "POLY_IMPORT_FUNC_X86_SLOT0" \
  "$TOOLS_SRC" "POLY_IMPORT_FUNC_X86_SLOT0"
compare_const "$HEADER_SRC" "POLY_IMPORT_FUNC_X86_SLOT7" \
  "$TOOLS_SRC" "POLY_IMPORT_FUNC_X86_SLOT7"
compare_const "$HEADER_SRC" "POLY_IMPORT_FUNC_X86_MIXED_U64_FP64_STACK" \
  "$TOOLS_SRC" "POLY_IMPORT_FUNC_X86_MIXED_U64_FP64_STACK"
compare_const "$HEADER_SRC" "POLY_IMPORT_FUNC_COUNT" \
  "$TOOLS_SRC" "POLY_IMPORT_FUNC_COUNT"

tools_count="$(extract_const "$TOOLS_SRC" "POLY_IMPORT_FUNC_COUNT")"
validate_dense_runtime_ids "$TOOLS_SRC" "$tools_count"
ensure_bochs_import_surface_is_generic

echo "poly import trap selector contract OK: $tools_count selectors"
