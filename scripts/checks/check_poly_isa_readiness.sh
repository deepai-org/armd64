#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ISA_DOC="$ROOT_DIR/docs/poly-isa.md"
DESIGN_DOC="$ROOT_DIR/docs/poly-isa-design-directions.md"
HEADER="$ROOT_DIR/tools/include/polycpuid.h"
MAKEFILE="$ROOT_DIR/Makefile"

fail() {
  echo "poly ISA readiness check failed: $*" >&2
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

assert_contains "## FPGA/Silicon ISA Readiness Boundary" "$ISA_DOC" \
  "ISA doc must have an explicit FPGA/silicon readiness boundary"
assert_contains "x86_64 remains the system ISA" "$ISA_DOC" \
  "ISA boundary must keep x86 as the system ISA"
assert_contains "user-mode native fetch frontends" "$ISA_DOC" \
  "ISA boundary must define AArch64/RISC-V as native user-mode fetch frontends"
assert_contains "fixed-latency decoded control operations" "$ISA_DOC" \
  "ISA boundary must require fixed-latency decoded control operations"
assert_contains "#UD.*trap envelopes" "$ISA_DOC" \
  "ISA boundary must reject #UD trap-envelope switching"
assert_contains "CPUID-discovered" "$ISA_DOC" \
  "ISA boundary must require opcode discovery through CPUID"
assert_contains "not a production x86 allocation" "$ISA_DOC" \
  "ISA boundary must not claim the prototype x86 opcode is production-owned"
assert_contains "register-only ABI signature slots" "$ISA_DOC" \
  "ISA boundary must keep fast PCALL signatures register-only"
assert_contains "runtime policy" "$ISA_DOC" \
  "ISA boundary must leave complex ABI/syscall/helper policy to userspace"
assert_contains "XSAVE-style per-thread state" "$ISA_DOC" \
  "ISA boundary must require explicit XSAVE-style state"
assert_contains "OS-neutral trap packets" "$ISA_DOC" \
  "ISA boundary must require OS-neutral trap packets"
assert_contains "transition-stack return cookie" "$ISA_DOC" \
  "ISA boundary must specify native return-cookie recovery"
assert_contains "rejected before mutating architectural frontend/PC state" "$ISA_DOC" \
  "ISA boundary must require pre-mutation validation"
assert_contains "x86-TSO no-ops" "$ISA_DOC" \
  "ISA boundary must keep foreign barriers/fences under x86 TSO"
assert_contains "FPGA fabric, Verilog structure" "$ISA_DOC" \
  "ISA boundary must separate ISA readiness from RTL/FPGA implementation"
assert_contains "timing closure" "$ISA_DOC" \
  "ISA boundary must not claim FPGA timing closure"

assert_contains "Hardware must not implement Linux, libc, libgcc, libatomic, dynamic-linker" \
  "$DESIGN_DOC" \
  "design directions must keep OS/runtime policy out of hardware"
assert_contains "#UD.*envelopes" \
  "$DESIGN_DOC" \
  "design directions must reject #UD envelope switching"
assert_contains "Signature slots are architectural" \
  "$DESIGN_DOC" \
  "design directions must define exact ABI signature-slot encoding"
assert_contains "register_map << 32" "$DESIGN_DOC" \
  "design directions must include the ABI register-map encoding"
assert_contains "XSAVE-style Poly component" "$DESIGN_DOC" \
  "design directions must keep Poly state explicit and XSAVE-style"
assert_contains "If a monitor vector is enabled, hardware must publish the monitor packet before" \
  "$DESIGN_DOC" \
  "design directions must order trap packet writes before monitor redirects"

for symbol in \
  POLY_CPUID_FEATURE_RAW_AARCH64 \
  POLY_CPUID_FEATURE_RAW_RISCV \
  POLY_CPUID_FEATURE_NEUTRAL_SWITCH \
  POLY_CPUID_FEATURE_NATIVE_RET \
  POLY_CPUID_FEATURE_TRAP_RECORDS \
  POLY_CPUID_FEATURE_X86_TSO \
  POLY_CPUID_FEATURE_PER_THREAD_STATE \
  POLY_CPUID_FEATURE_X86_POLY_OPCODES \
  POLY_CPUID_STATE_XSAVE_ARCH_CONTRACT \
  POLY_MEMORY_MODEL_X86_TSO \
  POLY_X86_OPCODE_FLAG_CPUID_DISCOVERED \
  POLY_X86_OPCODE_FLAG_DEDICATED_DECODE \
  POLY_X86_OPCODE_FLAG_NOT_UD_TRAP \
  POLY_X86_OPCODE_FLAG_VENDOR_PROTOTYPE \
  POLY_X86_OPCODE_FLAG_PRODUCTION_REASSIGNABLE; do
  assert_contains "\\b${symbol}\\b" "$HEADER" \
    "polycpuid.h must expose ISA readiness symbol ${symbol}"
done

assert_contains "check-poly-isa-readiness" "$MAKEFILE" \
  "Makefile must expose the ISA readiness gate"
assert_contains "./scripts/checks/check_poly_isa_readiness.sh" "$MAKEFILE" \
  "Makefile must run the ISA readiness script"
assert_contains "check-poly-contracts:.*check-poly-import-ids" "$MAKEFILE" \
  "Makefile contract aggregate must include the import selector gate"
assert_contains "check-poly-contracts:.*check-poly-isa-readiness" "$MAKEFILE" \
  "Makefile contract aggregate must include the ISA readiness gate"
assert_contains "check-poly-contracts:.*check-poly-arch-contract" "$MAKEFILE" \
  "Makefile contract aggregate must include the architecture contract gate"
assert_contains "check-poly-contracts:.*check-poly-cpuid-contract" "$MAKEFILE" \
  "Makefile contract aggregate must include the CPUID contract gate"
assert_contains "check-poly-contracts:.*check-poly-state-layout" "$MAKEFILE" \
  "Makefile contract aggregate must include the XSAVE state layout gate"

assert_not_contains "is[[:space:]]+a[[:space:]]+production x86 allocation" "$ISA_DOC" \
  "ISA doc must not claim the prototype opcode is production-owned"

echo "poly ISA readiness OK"
