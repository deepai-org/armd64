#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ISA_DOC="$ROOT_DIR/docs/poly-isa.md"
FROZEN_ISA_DOC="$ROOT_DIR/docs/poly-isa-v1.md"
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
assert_contains "poly-isa-v1.md" "$ISA_DOC" \
  "ISA quick reference must link the frozen v1 spec"

assert_contains "^# Poly Frozen ISA v1" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 document must exist"
assert_contains "## Opcode Ownership" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover production opcode ownership"
assert_contains "vendor-owned architectural extension page" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must require a vendor-owned production opcode page"
assert_contains "POLY_X86_OPCODE_FLAG_VENDOR_PROTOTYPE" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must distinguish prototype opcode ownership"
assert_contains "POLY_X86_OPCODE_FLAG_PRODUCTION_REASSIGNABLE" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must keep prototype opcode family reassignable"
assert_contains "## Frontends" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must define frontend IDs"
assert_contains "3\\.\\.255.*Reserved" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must reserve invalid frontend IDs"
assert_contains "defined baseline plus precise traps" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must choose the AArch64/RISC-V extension baseline policy"
assert_contains "## Control Encodings" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover control op encodings"
assert_contains "0xd503201f" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must include the AArch64 control encoding"
assert_contains "0x0000700b" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must include the RISC-V control encoding"
assert_contains "## CPUID Leaves" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover CPUID leaves and subleaves"
assert_contains "subleafs 32 and 33" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover x86 opcode CPUID discovery subleafs"
assert_contains "POLY_CPUID_BASE \\+ 4.*1\\.\\.15" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must document spill-image layout CPUID subleaves"
assert_contains "## User Spill State Image" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover the user spill state image"
assert_contains "POLY_STATE_XSAVE_BYTES_ARCH" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must name the 8KB Poly state image"
assert_contains "reserved bytes and reserved flags are write-zero/read-zero" \
  "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must define reserved state behavior"
assert_contains "## Auto-Spill And Monitor Trampoline" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must document the auto-spill trampoline"
assert_contains "PSET_SPILL_PTR\\(buffer_addr, resume_rip\\)" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must define spill pointer setup"
assert_contains "PRESTORE" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must define state restore from the spill image"
assert_contains "## Zero-Kernel-Change OS Contract" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must document the zero-kernel OS contract"
assert_contains "not required to" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must state that the OS is not required to manage Poly state"
assert_contains "Poly XCR0 bit" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must explicitly avoid requiring an OS Poly XCR0 bit"
assert_contains "XSAVE/XRSTOR" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must explicitly avoid requiring OS XSAVE/XRSTOR"
assert_contains "unmodified OS" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must define unmodified-OS behavior"
assert_contains "## Trap Packet" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover the trap packet format"
assert_contains "POLY_TRAP_PACKET_ARG_COUNT" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must include trap packet argument count"
assert_contains "eight native ABI argument lanes" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must keep trap packet argument ordering visible"
assert_contains "## Error Precedence" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover error precedence"
assert_contains "before frontend/PC mutation" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must require pre-mutation validation"
assert_contains "## Forward Compatibility" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover forward-compatibility rules"
assert_contains "Reserved frontend IDs, subops, CPUID bits, state flags, packet flags" \
  "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must cover reserved bits and fields"
assert_contains "## Conformance Matrix" "$FROZEN_ISA_DOC" \
  "frozen ISA v1 must include a conformance matrix"

for matrix_rule in \
  "x86 opcode family" \
  "Invalid frontend IDs" \
  "Canonical and frontend alignment faults" \
  "ABI signature slot" \
  "Trap packet ordering" \
  "Return-cookie recovery" \
  "TSO barriers/fences" \
  "User spill import/export" \
  "Auto-spill trampoline" \
  "CPUID discovery" \
  "Zero-kernel-change"; do
  assert_contains "$matrix_rule" "$FROZEN_ISA_DOC" \
    "frozen ISA v1 conformance matrix must cover ${matrix_rule}"
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
assert_contains "PSET_SPILL_PTR" "$DESIGN_DOC" \
  "design directions must include the auto-spill setup control"
assert_contains "unmodified OS" "$DESIGN_DOC" \
  "design directions must keep the OS out of Poly state management"
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
  POLY_CPUID_STATE_USER_SPILL \
  POLY_CPUID_STATE_MONITOR_TRAMPOLINE \
  POLY_CPUID_STATE_OS_XSAVE_NOT_REQUIRED \
  POLY_STATE_XSAVE_FLAG_USER_SPILL \
  POLY_STATE_XSAVE_FLAG_MONITOR_TRAMPOLINE \
  POLY_STATE_XSAVE_FLAG_OS_XSAVE_NOT_REQUIRED \
  POLY_X86_CTRL_SPILL_PTR_SET \
  POLY_X86_CTRL_PRESTORE \
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
