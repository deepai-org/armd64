#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ISA_DOC="$ROOT_DIR/docs/poly-isa.md"
V2_ISA_DOC="$ROOT_DIR/docs/poly-isa-v2-draft.md"
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
assert_contains "versioned v2 spill descriptor" "$ISA_DOC" \
  "ISA boundary must require descriptor-owned spill state"
assert_contains "PSET_EVENT_PTR" "$ISA_DOC" \
  "ISA boundary must include v2 event-frame setup"
assert_contains "PSET_SPILL_DESC" "$ISA_DOC" \
  "ISA boundary must include v2 spill descriptor setup"
assert_contains "trampoline RIP" "$ISA_DOC" \
  "ISA boundary must route OS-visible interrupts through the monitor trampoline"
assert_contains "OS-neutral v2 event frames" "$ISA_DOC" \
  "ISA boundary must require OS-neutral v2 event frames"
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
assert_not_contains "PSET_SPILL_PTR" "$ISA_DOC" \
  "active ISA quick reference must not advertise the retired raw spill pointer control"

assert_contains "^# Poly ISA v2 Draft" "$V2_ISA_DOC" \
  "active ISA v2 draft document must exist"
assert_contains "## Opcode Ownership And Discovery" "$V2_ISA_DOC" \
  "active ISA v2 draft must cover production opcode ownership"
assert_contains "vendor-owned architectural extension page" "$V2_ISA_DOC" \
  "active ISA v2 draft must require a vendor-owned production opcode page"
assert_contains "POLY_X86_OPCODE_FLAG_VENDOR_PROTOTYPE" "$V2_ISA_DOC" \
  "active ISA v2 draft must distinguish prototype opcode ownership"
assert_contains "POLY_X86_OPCODE_FLAG_PRODUCTION_REASSIGNABLE" "$V2_ISA_DOC" \
  "active ISA v2 draft must keep prototype opcode family reassignable"
assert_contains "3\\.\\.255.*Reserved" "$V2_ISA_DOC" \
  "active ISA v2 draft must reserve invalid frontend IDs"
assert_contains "defined baseline plus precise traps" "$V2_ISA_DOC" \
  "active ISA v2 draft must choose the AArch64/RISC-V extension baseline policy"
assert_contains "## New Control Operations" "$V2_ISA_DOC" \
  "active ISA v2 draft must cover control operations"
assert_contains "PSET_EVENT_PTR" "$V2_ISA_DOC" \
  "active ISA v2 draft must include event-frame registration"
assert_contains "PSET_SPILL_DESC" "$V2_ISA_DOC" \
  "active ISA v2 draft must include spill-descriptor registration"
assert_contains "## CPUID And Compatibility" "$V2_ISA_DOC" \
  "active ISA v2 draft must cover CPUID leaves and compatibility"
assert_contains "control geometry" "$V2_ISA_DOC" \
  "active ISA v2 draft must cover opcode CPUID discovery"
assert_contains "event-frame layout" "$V2_ISA_DOC" \
  "active ISA v2 draft must document v2 layout CPUID coverage"
assert_contains "## Spill/Resume Descriptor" "$V2_ISA_DOC" \
  "active ISA v2 draft must cover descriptor-owned spill state"
assert_contains "struct poly_v2_spill_descriptor" "$V2_ISA_DOC" \
  "active ISA v2 draft must define the spill descriptor layout"
assert_contains "reserved-zero/read-zero" "$V2_ISA_DOC" \
  "active ISA v2 draft must define reserved state behavior"
assert_contains "## Canonical Event Frame" "$V2_ISA_DOC" \
  "active ISA v2 draft must cover canonical event frames"
assert_contains "struct poly_v2_event_frame" "$V2_ISA_DOC" \
  "active ISA v2 draft must define canonical event-frame layout"
assert_contains "PRESTORE" "$V2_ISA_DOC" \
  "active ISA v2 draft must define state restore from descriptor-selected images"
assert_contains "zero-kernel-change contract" "$V2_ISA_DOC" \
  "active ISA v2 draft must document the zero-kernel OS contract"
assert_contains "unmodified OS" "$V2_ISA_DOC" \
  "active ISA v2 draft must define unmodified-OS behavior"
assert_contains "debug-note layout" "$V2_ISA_DOC" \
  "active ISA v2 draft must cover OS-neutral debug-note export"
assert_contains "## Error Precedence" "$V2_ISA_DOC" \
  "active ISA v2 draft must cover error precedence"
assert_contains "before mutating architectural" "$V2_ISA_DOC" \
  "active ISA v2 draft must require pre-mutation validation"
for reserved_category in \
  "Reserved frontend IDs" \
  "subops" \
  "CPUID bits" \
  "state flags" \
  "event-frame fields" \
  "descriptor fields"; do
  assert_contains "$reserved_category" "$V2_ISA_DOC" \
    "active ISA v2 draft must cover reserved category ${reserved_category}"
done
assert_contains "## Conformance Matrix" "$V2_ISA_DOC" \
  "active ISA v2 draft must include a conformance matrix"
assert_contains "PSET_SPILL_PTR\\(buffer_addr, resume_rip\\).*not part of the v2 active surface" "$V2_ISA_DOC" \
  "active ISA v2 draft must explicitly retire raw spill pointer setup"

for matrix_rule in \
  "x86 opcode family" \
  "Invalid frontend IDs" \
  "Canonical and frontend alignment faults" \
  "ABI signature slot" \
  "v2 event-frame ordering" \
  "Return-cookie recovery" \
  "TSO barriers/fences" \
  "User spill import/export" \
  "Auto-spill trampoline" \
  "CPUID discovery" \
  "Zero-kernel-change"; do
  assert_contains "$matrix_rule" "$V2_ISA_DOC" \
    "active ISA v2 conformance matrix must cover ${matrix_rule}"
done

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
assert_contains "Poly spill image" "$DESIGN_DOC" \
  "design directions must keep Poly state explicit and user-spilled"
assert_contains "PSET_EVENT_PTR" "$DESIGN_DOC" \
  "design directions must include the v2 event-frame setup control"
assert_contains "PSET_SPILL_DESC" "$DESIGN_DOC" \
  "design directions must include the v2 spill descriptor setup control"
assert_contains "unmodified OS" "$DESIGN_DOC" \
  "design directions must keep the OS out of Poly state management"
assert_contains "If a monitor vector is enabled, hardware must publish the canonical event frame before" \
  "$DESIGN_DOC" \
  "design directions must order event-frame writes before monitor redirects"

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
  POLY_CPUID_STATE_USER_SPILL \
  POLY_CPUID_STATE_MONITOR_TRAMPOLINE \
  POLY_CPUID_STATE_OS_XSAVE_NOT_REQUIRED \
  POLY_STATE_XSAVE_FLAG_USER_SPILL \
  POLY_STATE_XSAVE_FLAG_MONITOR_TRAMPOLINE \
  POLY_STATE_XSAVE_FLAG_OS_XSAVE_NOT_REQUIRED \
  POLY_X86_CTRL_PRESTORE \
  POLY_X86_CTRL_EVENT_PTR_SET \
  POLY_X86_CTRL_SPILL_DESC_SET \
  POLY_X86_CTRL_AUTO_SPILL_COUNT_STATUS \
  POLY_X86_CTRL_AUTO_SPILL_BYTES_STATUS \
  POLY_X86_CTRL_AUTO_SPILL_CYCLES_STATUS \
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
  "Makefile contract aggregate must include the state layout gate"

assert_not_contains "is[[:space:]]+a[[:space:]]+production x86 allocation" "$ISA_DOC" \
  "ISA doc must not claim the prototype opcode is production-owned"

echo "poly ISA readiness OK"
