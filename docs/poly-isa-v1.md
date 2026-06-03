# Poly Frozen ISA v1

Status: frozen v1 contract for the current Poly ISA, CPUID discovery surface,
user-owned spill state image, monitor trampoline, trap packet, and
compatibility rules.

This document freezes the architectural contract only. It does not claim timing,
area, power, upstream OS enablement, or a complete production frontend decoder.

## Opcode Ownership

The v1 x86 control family is production-discovered, not production-hardcoded.
Software must discover the active x86 byte geometry through
`POLY_CPUID_BASE + 2`, subleafs 32 and 33 before emitting x86 Poly controls.

The Bochs bring-up allocation is `0f 3a fc <subop>`. It is a vendor/prototype
dedicated decode page advertised with `POLY_X86_OPCODE_FLAG_VENDOR_PROTOTYPE`
and `POLY_X86_OPCODE_FLAG_PRODUCTION_REASSIGNABLE`. A silicon implementation
must either allocate a vendor-owned architectural extension page and advertise
that geometry through the same CPUID subleafs, or keep a compatible
vendor-owned production page with `POLY_X86_OPCODE_FLAG_VENDOR_PROTOTYPE`
cleared. In all cases the family is decoded directly and never delivered
through a `#UD` trap envelope.

## Frontends

| ID | Frontend | v1 baseline |
| --- | --- | --- |
| `0` | x86_64 | System ISA for boot, privilege, paging, interrupts, faults, atomics, syscalls, VM control, and global x86 TSO. |
| `1` | AArch64 | User-mode raw fetch frontend with 4-byte aligned instruction fetch. |
| `2` | RISC-V64 | User-mode raw fetch frontend with 16/32-bit fetch, including RVC alignment. |
| `3..255` | Reserved | Must be rejected before frontend/PC mutation. |

AArch64 and RISC-V64 support is a defined baseline plus precise traps for
unsupported instructions. v1 does not promise a full architectural
implementation of every extension. Unsupported, illegal, unavailable, or
implementation-reserved foreign instructions must trap precisely as
`POLY_TRAP_ILLEGAL` or the narrower architectural trap type when one exists.

## Control Encodings

| Frontend | Encoding |
| --- | --- |
| x86_64 | CPUID-discovered family; Bochs prototype reports `0f 3a fc <subop>`. |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Control subops cover `PENTER`, `PSWITCH`, `PCALL`, signature-slot immediate
calls, trap vector setup/query, monitor packet setup/query, state-key
setup/query, `PLANDING`, `PTRAPRET`, `PSET_SPILL_PTR`, and `PRESTORE`. ABI
signature slots are architectural `kind | (register_map << 32)` values.
`ABI_SIGNATURE_GET` returns the full encoded value.

Controls are fixed-latency decoded operations. They do not parse user
descriptors, repack stacks, invoke libc/libgcc helpers, translate syscalls, or
enter exception delivery for normal frontend switching.

## CPUID Leaves

Poly CPUID starts at `POLY_CPUID_BASE` and is present only when the vendor leaf
matches the Poly vendor string and `EAX >= POLY_CPUID_MAX`.

| Leaf | Subleaf | Architectural payload |
| --- | --- | --- |
| `POLY_CPUID_BASE + 0` | `0` | Vendor string and maximum Poly leaf. |
| `POLY_CPUID_BASE + 1` | `0` | ABI version, frontend mode mask, feature bits, and `POLY_STATE_XSAVE_COMPONENT_ARCH`. |
| `POLY_CPUID_BASE + 2` | `0..31` | Control-op and ABI signature manifests. |
| `POLY_CPUID_BASE + 2` | `32` | Active x86 opcode geometry: prefix bytes, total bytes, subop offset, family ID. |
| `POLY_CPUID_BASE + 2` | `33` | x86 opcode contract version and flags. |
| `POLY_CPUID_BASE + 2` | `34` | Auto-spill profiling status controls: spill count, spilled bytes, and estimated cycles. |
| `POLY_CPUID_BASE + 3` | `0` | Poly state feature flags and the 8KB user spill image identity. |
| `POLY_CPUID_BASE + 4` | `0` | User spill image size, alignment, layout version, and state flags. |
| `POLY_CPUID_BASE + 4` | `1..15` | Fixed offsets and sizes for header, trap, GPR/FP/status, ABI, TLS, policy, return, and reserved regions. |
| `POLY_CPUID_BASE + 5` | `0` | Trap packet layout version, bytes, argument count, and required flags. |
| `POLY_CPUID_BASE + 6` | `0` | Interrupt ABI version, flags, return forms, and raw mode mask. |
| `POLY_CPUID_BASE + 7` | `0` | Memory ABI version, `POLY_MEMORY_MODEL_X86_TSO`, memory flags, and raw mode mask. |
| `POLY_CPUID_BASE + 8` | `0..5` | Transition ABI, frontend IDs, transition frame, cross/import/native return layout. |
| `POLY_CPUID_BASE + 9` | `0` | ABI bridge version, flags, register counts, and stack alignment. |

Unsupported Poly leaves and subleafs return no architectural feature. Software
must ignore unknown set feature bits only when the relevant versioned leaf says
the bit is forward-compatible; otherwise unknown bits are reserved and must not
be required for v1 execution.

## User Spill State Image

Poly state is a fixed 8KB user-owned state image. The historical
`POLY_STATE_XSAVE_*` names identify the layout and are retained for source
compatibility; they do not require the OS to enumerate, enable, execute XSAVE
for, or context-switch a custom xstate component. The current v1 image is:

| Field | Value |
| --- | --- |
| Layout ID | `POLY_STATE_XSAVE_COMPONENT_ARCH` |
| Legacy offset | `POLY_STATE_XSAVE_OFFSET_ARCH` |
| Bytes | `POLY_STATE_XSAVE_BYTES_ARCH` |
| Alignment | `POLY_STATE_XSAVE_ALIGN_ARCH` |
| Layout version | `POLY_STATE_XSAVE_LAYOUT_VERSION` |

The image contains a fixed header, trap packet, trap arguments, transition
frame, AArch64 GPR/FP/status state, RISC-V GPR/FP/status state, import-return
stack, ABI signature slots, cross-return stack, frontend TLS, landing policy,
state key, trap-restore state, native-return stack, and a reserved tail. All
reserved bytes and reserved flags are write-zero/read-zero for v1 unless a
future versioned CPUID leaf states otherwise. `PRESTORE` validates magic,
layout version, image size, alignment, state flags, reserved bits, and frontend
IDs before making the state live.

The header contains the current foreign PC in `foreign_pc`. On an auto-spill,
the high 32 bits of the `trap_vector_mode` header word carry a
`POLY_SPILL_REASON_*` value: `NONE`, `INTERRUPT`, `PAGE_FAULT`, or `FAULT`.
For a page fault, trap argument lane 0 contains the x86 vector, lane 1 contains
the event type, lane 2 contains the x86 error code, and lane 3 contains CR2.

## Auto-Spill And Monitor Trampoline

Before entering Poly code, the Ring 3 monitor allocates one aligned
`POLY_STATE_XSAVE_BYTES_ARCH` image per thread and registers it with
`PSET_SPILL_PTR(buffer_addr, resume_rip)`. The buffer must be canonical,
64-byte aligned, and large enough for the complete image. `resume_rip` must be
a canonical x86 userspace monitor trampoline. Registering `<0, 0>` disables
auto-spill for the current thread.

When an external interrupt, hardware exception, or page fault arrives while CPL3
is executing a raw Poly frontend and auto-spill is enabled, the CPU intercepts
the event before building the kernel-visible interrupt frame:

1. Export the complete 8KB Poly state image to `buffer_addr`.
2. Record the current Poly PC, spill reason, and fault metadata in the image.
3. Switch the architectural frontend back to x86.
4. Replace the kernel-visible interrupted x86 RIP with `resume_rip`.
5. Vector to the ordinary OS interrupt or exception handler.

When the OS later returns to userspace, it returns to `resume_rip` as an
ordinary x86 instruction pointer. The monitor reads the spill header. For a
timer/preemption event it executes `PRESTORE` on the image and then `PENTER` to
resume the raw frontend. For a page fault or POSIX signal, the monitor uses the
spilled Poly PC, frontend state, CR2, vector, and error code to translate the
native signal into a Poly-context exception in userspace.

Implementations should expose profiling counters for successful auto-spills,
total spilled bytes, and estimated spill cycles. The counters are diagnostic and
do not change the architectural spill image or OS contract.

## Zero-Kernel-Change OS Contract

An unmodified OS must only see ordinary x86 user state. It is not required to
enumerate Poly CPUID leaves, enable a Poly XCR0 bit, execute XSAVE/XRSTOR for a
custom component, know foreign register files, or place Poly state in signal,
ptrace, core-dump, fork/clone, exec, migration, or context-switch paths.

The CPU must not expose live raw-frontend state to the OS interrupt frame. If
auto-spill is enabled and the spill write succeeds, the OS sees the trampoline
RIP. If auto-spill is disabled, runtimes must use a kernel-aware/legacy path or
refuse to execute raw state across asynchronous OS boundaries. A spill-buffer
validation failure before `PENTER` is a control-instruction error; a fault while
writing the spill image is reported as a native x86 fault at the trampoline and
must not leave hidden live Poly state for the OS to save.

## Trap Packet

Trap records are OS-neutral. v1 reports `POLY_TRAP_PACKET_LAYOUT_VERSION`,
`POLY_TRAP_PACKET_HEADER_BYTES`, `POLY_TRAP_PACKET_ARG_COUNT`, and
`POLY_TRAP_PACKET_REQUIRED_FLAGS` through CPUID. The monitor packet contains the
source frontend, source PC, trap type, flags, opaque selector or syscall number,
fault/auxiliary fields, and eight native ABI argument lanes.

If a monitor vector is enabled, packet publication precedes frontend redirect.
Invalid, non-canonical, unaligned, or boundary-crossing monitor packet addresses
are rejected before redirect. A packet memory fault prevents redirect and is
reported as the precise fault. Hardware does not translate syscall numbers,
resolve imports, or name libc/libgcc helpers.

## Error Precedence

For one retiring Poly control or recoverable trap event, v1 applies this order:

1. Older architectural faults and interrupt-boundary holds block retirement.
2. Decode/fetch faults block control commit.
3. Invalid frontend IDs, non-canonical targets, frontend alignment faults, and
   invalid ABI signature slots are rejected before frontend/PC mutation.
4. Transition-stack overflow/underflow and return-cookie conflicts are rejected
   before push/pop state becomes visible.
5. Auto-spill buffer and trampoline validation happens before raw frontend
   entry when `PSET_SPILL_PTR` is executed.
6. On an asynchronous raw-mode event, the auto-spill image write completes and
   the x86 RIP is changed to the trampoline before the OS-visible interrupt
   frame is built.
7. Trap packet address validation happens before monitor redirect.
8. Trap packet memory completion happens before monitor redirect; packet memory
   faults suppress redirect.
9. Accepted commits, trap redirects, trap returns, interrupt restores, and
   native return-cookie resumes update frontend/PC through the same validation
   boundary.

## Memory Ordering

The shared address space is x86 TSO. Foreign barriers and fences are
architectural x86-TSO no-ops, and foreign memory operations cannot expose weak
reordering relative to x86 memory operations. Atomic operations are coherent
with the x86 memory system; natural-alignment restrictions are enforced where
the raw data-memory boundary requires them.

## Forward Compatibility

Reserved frontend IDs, subops, CPUID bits, state flags, packet flags, and
reserved bytes must be zero unless a later versioned CPUID leaf defines them.
Software must use CPUID feature and layout leaves instead of deriving behavior
from Bochs prototype encodings. Hardware may add new frontends, subops, state
regions, or trap metadata only behind versioned discovery and must preserve v1
semantics for existing leaves.

## Conformance Matrix

| Rule | Evidence gate |
| --- | --- |
| x86 opcode family is CPUID-discovered and not a `#UD` envelope | `scripts/checks/check_poly_arch_contract.sh`, `scripts/checks/check_poly_cpuid_contract.sh`, `tools/programs/nativecheck.c`, `rtl/test_poly_cpuid_rom.py` |
| Invalid frontend IDs are rejected before state mutation | `rtl/test_poly_frontend_state.py`, `rtl/test_poly_frontend_core.py`, `tools/programs/nativecheck.c` |
| Canonical and frontend alignment faults are precise | `rtl/test_poly_frontend_fetch_issue.py`, `rtl/test_poly_raw_fetch_request.py`, `rtl/test_poly_frontend_state.py`, `tools/programs/nativecheck.c` |
| ABI signature slot encoding and invalid slot behavior | `rtl/test_poly_abi_signature_slots.py`, `scripts/checks/check_poly_cpuid_contract.sh`, `tools/programs/nativecheck.c` |
| Trap packet ordering, packet address validation, and memory-fault suppression | `rtl/test_poly_trap_packet_encode.py`, `rtl/test_poly_trap_packet_stage.py`, `scripts/checks/check_poly_arch_contract.sh`, `tools/programs/nativecheck.c` |
| Return-cookie recovery and transition-stack underflow/overflow | `rtl/test_poly_return_cookie_recover.py`, `rtl/test_poly_transition_stack.py`, `rtl/poly_transition_stack_return_formal.sv`, `tools/programs/nativecheck.c` |
| TSO barriers/fences and no weak foreign reordering | `rtl/test_poly_memory_order.py`, `rtl/test_poly_memory_order_litmus.py`, `rtl/poly_memory_order_formal.sv`, `tools/include/polycpuid.h` |
| User spill import/export and fixed 8KB state layout | `scripts/checks/check_poly_state_layout.sh`, `tools/programs/polylayout.c`, `tools/programs/nativecheck.c`, `scripts/checks/check_poly_arch_contract.sh` |
| Auto-spill trampoline before OS-visible interrupt/fault entry | `scripts/checks/check_poly_arch_contract.sh`, `bochs/cpu/proc_ctrl.cc`, `bochs/cpu/exception.cc` |
| CPUID discovery leaves and reserved opcode contract flags | `scripts/checks/check_poly_cpuid_contract.sh`, `rtl/test_poly_cpuid_rom.py`, `tools/runtime/polyexec.c`, `tools/programs/nativecheck.c` |
| AArch64/RISC-V baseline plus precise trap for unsupported instructions | `tools/fixtures/polyapps/*`, `tools/fixtures/polycall/*`, `scripts/boot.sh`, `tools/programs/nativecheck.c` |
| Zero-kernel-change OS contract and no required custom XSAVE/XCR0 path | `docs/poly-isa-v1.md`, `scripts/checks/check_poly_isa_readiness.sh`, `scripts/checks/check_poly_cpuid_contract.sh` |

The fast non-boot gate is `make check-poly-contracts`. The broad boot evidence
gate for this v1 contract is:

```bash
make check-poly-contracts
```
