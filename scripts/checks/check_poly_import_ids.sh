#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BOCHS_SRC="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"
TOOLS_SRC="$ROOT_DIR/tools/runtime/polycall.c"
TMP_DIR="${TMPDIR:-/tmp}/poly-import-ids.$$"

mkdir -p "$TMP_DIR"
trap 'rm -rf "$TMP_DIR"' EXIT

extract_ids() {
  local src="$1"
  local prefix="$2"
  local out="$3"

  awk -v prefix="$prefix" '
    function trim(s) {
      gsub(/^[ \t]+|[ \t]+$/, "", s)
      return s
    }
    {
      line = $0
      sub(/\/\/.*/, "", line)
      pattern = prefix "[A-Z0-9_]+[ \t]*="
      if (line !~ pattern)
        next

      name = line
      sub(".*" prefix, "POLY_IMPORT_FUNC_", name)
      sub(/[ \t]*=.*/, "", name)
      if (name == "POLY_IMPORT_FUNC_COUNT")
        next

      value = line
      sub(/.*=[ \t]*/, "", value)
      sub(/,.*/, "", value)
      print name "=" trim(value)
    }
  ' "$src" > "$out"
}

extract_count() {
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
      gsub(/^[ \t]+|[ \t]+$/, "", value)
      print value
      exit
    }
  ' "$src"
}

BOCHS_IDS="$TMP_DIR/bochs.ids"
TOOLS_IDS="$TMP_DIR/tools.ids"

extract_ids "$BOCHS_SRC" "BX_POLY_IMPORT_FUNC_" "$BOCHS_IDS"
extract_ids "$TOOLS_SRC" "POLY_IMPORT_FUNC_" "$TOOLS_IDS"

if ! diff -u "$BOCHS_IDS" "$TOOLS_IDS"; then
  echo "poly import ID mismatch between Bochs and tools/runtime/polycall.c" >&2
  exit 1
fi

bochs_count="$(extract_count "$BOCHS_SRC" "BX_POLY_IMPORT_CALL_COUNT")"
tools_count="$(extract_count "$TOOLS_SRC" "POLY_IMPORT_FUNC_COUNT")"
actual_count="$(wc -l < "$TOOLS_IDS" | tr -d '[:space:]')"

if [[ -z "$bochs_count" || -z "$tools_count" ]]; then
  echo "failed to extract poly import count constants" >&2
  exit 1
fi

if [[ "$bochs_count" != "$tools_count" || "$tools_count" != "$actual_count" ]]; then
  echo "poly import count mismatch: bochs=$bochs_count tools=$tools_count actual=$actual_count" >&2
  exit 1
fi

echo "poly import ID manifest OK: $actual_count ids"
