# Poly ISA v2 Draft

Status: historical design draft, superseded for production use by
`docs/poly-isa-userspace-offload-proposal.md`.

This file records the descriptor-backed auto-spill prototype and historical v2
bring-up details. New production work should use the userspace-offload
proposal, which keeps canonical event frames but replaces mandatory
interrupt-time spill descriptors with OS-neutral memory-probe, state-derive,
event-complete, debug-note, and shared-memory-ordering primitives.

Objective: thin the Ring 3 monitor by moving precise, OS-neutral execution
state primitives into the ISA. v2 must make core dumps, signal delivery,
seccomp-style policy, shared-memory I/O rings, clone/thread handoff, and
debugging easier without teaching hardware Linux, libc, ELF, BPF, io_uring, or
container policy.

## Design Rules

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, syscalls, VM control, and global TSO memory ordering.
- AArch64 and RISC-V64 remain user-mode frontends in the same virtual address
  space.
- Hardware publishes facts it already knows: event reason, fault address,
  completion state, frontend, PC, resume PC, raw arguments, and register state.
- Userspace owns policy: syscall translation, POSIX signals, seccomp-BPF,
  debugger file formats, dynamic linking, ABI stack reshaping, container
  behavior, and OS-specific emulation.
- All new memory-visible records are versioned, size-tagged, naturally aligned,
  little-endian, and reserved-zero/read-zero.
- v2 must preserve the zero-kernel-change contract: an unmodified OS sees
  ordinary x86 user state.

## Opcode Ownership And Discovery

The current x86 prototype encoding, `0f 3a fc <subop>`, is a
vendor/prototype dedicated decode page, not a production x86 allocation.
Production silicon must advertise a vendor-owned architectural extension page,
or a compatible vendor-owned production page, through Poly CPUID before runtime
code uses it. Software must treat opcode geometry as CPUID-discovered, not as
hard-coded documentation.

The active prototype reports `POLY_X86_OPCODE_FLAG_VENDOR_PROTOTYPE` and
`POLY_X86_OPCODE_FLAG_PRODUCTION_REASSIGNABLE` so runtimes know that the
encoding can move behind the same discovery contract. Production
implementations must not rely on `#UD` trap envelopes for ordinary Poly
switching.

Frontend IDs remain:

| ID | Frontend |
| --- | --- |
| `0` | x86_64 system frontend |
| `1` | raw AArch64 user frontend |
| `2` | raw RISC-V64 user frontend |
| `3..255` | Reserved, must be rejected before frontend/PC mutation |

AArch64 and RISC-V64 support is a defined baseline plus precise traps for
unsupported, illegal, unavailable, or implementation-reserved foreign
instructions. v2 does not require every optional extension before the hardware
contract is useful.

## CPUID And Compatibility

CPUID remains the compatibility contract for control geometry, frontend
features, v2 feature bits, state layout, event-frame layout, spill-descriptor
layout, debug-note layout, memory-probe result bits, derivation descriptors,
and opcode ownership flags. Reserved frontend IDs, subops, CPUID bits, state
flags, event-frame fields, descriptor fields, and packet compatibility flags
are write-zero/read-zero or must fault before mutation, depending on whether
they are memory records or control inputs.

The retired v1 `PSET_SPILL_PTR(buffer_addr, resume_rip)` control is not part of the v2 active surface.
Implementations may keep legacy monitor-packet controls behind compatibility
probes while software is being migrated, but active monitors must use
`PSET_EVENT_PTR` and `PSET_SPILL_DESC`.

Reserved frontend IDs, subops, CPUID bits, state flags, event-frame fields,
descriptor fields, and packet compatibility flags are all part of the v2
forward-compatibility contract.

## New Control Operations

Names are draft mnemonics. Final opcode allocation remains CPUID-discovered.

| Operation | Purpose |
| --- | --- |
| `PSET_EVENT_PTR addr, bytes` | Register the per-thread canonical event frame. |
| `PSET_SPILL_DESC addr, bytes` | Register the per-thread spill/resume descriptor. Replaces raw `<spill, resume_rip>` programming. |
| `PDUMP_STATE dst, bytes, selector` | Export a dense debug/register-note blob for one live or spilled frontend context. |
| `PMEM_PROBE_RANGE addr, len, flags` | Probe current-process virtual-memory accessibility without delivering a host signal to the monitor. |
| `PDERIVE_STATE dst, src, desc` | Derive a child frontend state from a parent state using explicit SP/TLS/result parameters. |
| `PCOMPLETE_EVENT state, desc` | Atomically apply monitor-selected result registers and resume metadata to a frontend state image. |
| `PFENCE scope` | Make foreign and x86 shared-memory ordering points explicit for monitor/syscall/ring handoff. |

All operations are OS-neutral. They do not issue host syscalls, parse ELF, run
BPF, build POSIX signal frames, or know io_uring structures.

## Canonical Event Frame

v1 trap packets are intentionally small, but the runtime still has to infer too
much. v2 defines one canonical event frame written before monitor entry for
recoverable traps, syscalls, faults, async exits, and raw-mode auto-spills.

The event frame is a 512-byte, 64-byte-aligned record. Hardware must either
write the complete frame before monitor redirect or report a precise x86 fault
without redirecting.

```c
#define POLY_V2_EVENT_MAGIC 0x32545645594c4f50ULL /* "POLYEVT2" */
#define POLY_V2_EVENT_VERSION 2
#define POLY_V2_EVENT_BYTES 512
#define POLY_V2_EVENT_ARG_COUNT 8

struct poly_v2_event_frame {
  /* 0x000 */
  uint64_t magic;
  uint32_t bytes;
  uint16_t version;
  uint16_t header_bytes;

  /* 0x010 */
  uint64_t sequence;
  uint32_t frontend;
  uint16_t event_kind;
  uint16_t event_subkind;

  /* 0x020 */
  uint64_t flags;
  uint32_t completion;
  uint16_t resume_policy;
  uint16_t arg_count;

  /* 0x030 */
  uint64_t insn_pc;
  uint64_t resume_pc;
  uint64_t fallthrough_pc;
  uint64_t target_pc;

  /* 0x050 */
  uint64_t foreign_sp;
  uint64_t foreign_tls;
  uint64_t frontend_status0;
  uint64_t frontend_status1;

  /* 0x070 */
  uint64_t selector;
  uint64_t args[POLY_V2_EVENT_ARG_COUNT];

  /* 0x0b8 */
  uint64_t fault_address;
  uint64_t fault_status;
  uint64_t raw_syndrome;
  uint64_t host_error_code;

  /* 0x0d8 */
  uint64_t side_effect_class;
  uint64_t memory_order_class;
  uint64_t result_register;
  uint64_t error_convention;

  /* 0x0f8 */
  uint64_t spill_descriptor;
  uint64_t spill_generation;
  uint64_t monitor_cookie;
  uint64_t state_key;

  /* 0x118 */
  uint64_t arch_private[16];

  /* 0x198 */
  uint8_t reserved[104];
};
```

Required semantics:

- `insn_pc` is the architectural PC of the instruction or boundary that caused
  the event.
- `resume_pc` is the architecturally correct PC to use if userspace resumes the
  interrupted context without redirecting it.
- `fallthrough_pc` is the decoded sequential PC when that concept exists. It is
  zero when not meaningful.
- `completion` says whether the instruction did not execute, partially
  executed with no architectural commit, fully completed, or trapped at an
  interrupt boundary.
- `selector` is opaque. For Linux AArch64 it may be a syscall number; for
  another OS or runtime it may be a different selector. Hardware does not
  translate it.
- `side_effect_class` is descriptive metadata only: none, syscall-like,
  blocking wait, memory-sharing setup, thread-like birth, or implementation
  reserved. It is not permission policy.
- `frontend_status0/1` are frontend-defined scalar status snapshots. For
  AArch64, `status0` carries PSTATE/NZCV-derived state and `status1` may carry
  FPCR/FPSR summary bits. Full registers come from the spill image or
  `PDUMP_STATE`.

This frame is the first v2 primitive to implement. It removes monitor-side
instruction-length guessing, syscall-specific resume whitelists, and ad hoc
fault metadata reconstruction.

## Error Precedence

All control operations validate inputs before mutating architectural
frontend/PC state, transition-stack state, registered monitor state, spill
descriptor state, or event-frame publication state. Validation order is:
unsupported subop, unsupported frontend, malformed width/version/header,
nonzero reserved input bits, non-canonical addresses, frontend alignment,
range crossing, permission or memory-write failure, then semantic conflicts
such as transition-stack overflow/underflow or stale descriptor generation.

If validation fails, hardware returns the documented error value or raises the
documented precise fault while preserving the prior active frontend/PC and
monitor registration.

## Spill/Resume Descriptor

v1 registers a spill buffer and an x86 resume RIP directly. v2 makes that a
versioned descriptor so hardware can validate ownership, generation, state
layout, event publication, and resume stack metadata atomically.

The descriptor is a 256-byte, 64-byte-aligned record. It is per thread. The
runtime registers it with `PSET_SPILL_DESC`.

```c
#define POLY_V2_SPILL_DESC_MAGIC 0x32445053594c4f50ULL /* "POLYSPD2" */
#define POLY_V2_SPILL_DESC_VERSION 2
#define POLY_V2_SPILL_DESC_BYTES 256

struct poly_v2_spill_descriptor {
  /* 0x000 */
  uint64_t magic;
  uint32_t bytes;
  uint16_t version;
  uint16_t header_bytes;

  /* 0x010 */
  uint64_t flags;
  uint64_t owner_cookie;
  uint64_t generation;
  uint64_t valid_mask;

  /* 0x030 */
  uint64_t state_addr;
  uint32_t state_bytes;
  uint16_t state_align;
  uint16_t state_layout_version;

  /* 0x040 */
  uint64_t event_addr;
  uint32_t event_bytes;
  uint32_t event_flags;

  /* 0x050 */
  uint64_t resume_rip;
  uint64_t resume_stack_base;
  uint64_t resume_stack_bytes;
  uint64_t resume_stack_top;

  /* 0x070 */
  uint64_t reserved_070[2];
  uint64_t state_key;
  uint64_t frontend_mask;

  /* 0x090 */
  uint64_t last_event_sequence;
  uint64_t last_spill_reason;
  uint64_t last_spill_cycles;
  uint64_t last_spill_bytes;

  /* 0x0b0 */
  uint64_t debug_note_addr;
  uint64_t debug_note_bytes;
  uint64_t policy_hook_addr;
  uint64_t policy_hook_bytes;

  /* 0x0d0 */
  uint8_t reserved[48];
};
```

Required semantics:

- `state_addr`, `event_addr`, `resume_rip`, and resume-stack fields are
  validated by `PSET_SPILL_DESC` before raw frontend entry.
- `owner_cookie` and `generation` prevent stale descriptors from being reused
  across threads or after runtime teardown.
- On a raw-mode interrupt/fault, hardware writes the state image, writes the
  canonical event frame, updates `last_*`, switches back to x86, and makes the
  unmodified OS see `resume_rip`.
- `PDERIVE_STATE` with `ACTIVATE_DST` validates an already-derived state image,
  imports the requested frontend state, and arms the raw frontend PC consumed
  by the following `PENTER`.
- `policy_hook_*` is metadata for userspace monitors. Hardware must not call
  it, interpret it, or enforce its result.

This descriptor replaces hardcoded reserved spill offsets and monitor-side stack
pivot conventions with one hardware-validated per-thread record.

## Debug/Register Note Export

`PDUMP_STATE` exports a dense, versioned register-note blob. The blob is not
ELF, Mach-O, minidump, ptrace, or DWARF. It is an architectural source record
that userspace can wrap in any debugger/container format.

The v2 debug note is a 9216-byte, 64-byte-aligned record. It contains a compact
header, a copied canonical event frame, and an embedded `poly_xsave_state`
register image. Selector `0` exports the live current thread. Selector `1`
exports the state/event pair selected by the currently registered spill
descriptor.

```c
#define POLY_V2_DEBUG_NOTE_MAGIC 0x32474244594c4f50ULL /* "POLYDBG2" */
#define POLY_V2_DEBUG_NOTE_VERSION 2
#define POLY_V2_DEBUG_NOTE_BYTES 9216
#define POLY_V2_DEBUG_NOTE_HEADER_BYTES 512
#define POLY_V2_DEBUG_NOTE_EVENT_OFFSET 512
#define POLY_V2_DEBUG_NOTE_XSAVE_OFFSET 1024

struct poly_v2_debug_note {
  uint64_t magic;
  uint32_t bytes;
  uint16_t version;
  uint16_t header_bytes;
  uint64_t selector;
  uint64_t flags;
  uint32_t frontend;
  uint32_t current_mode;
  uint32_t state_layout_version;
  uint32_t state_bytes;
  uint64_t pc;
  uint64_t sp;
  uint64_t tls;
  uint64_t status0;
  uint64_t status1;
  uint64_t event_sequence;
  uint64_t event_kind;
  uint64_t fault_address;
  uint64_t raw_syndrome;
  uint64_t gpr_valid_mask;
  uint64_t fp_valid_mask;
  uint64_t transition_depth;
  uint64_t transition_top_cookie;
  uint64_t state_key;
  uint64_t spill_descriptor;
  uint64_t spill_generation;
  uint64_t event_offset;
  uint64_t event_bytes;
  uint64_t xsave_offset;
  uint64_t xsave_bytes;
  uint8_t reserved[304];
  struct poly_v2_event_frame event;
  struct poly_xsave_state state;
};
```

Minimum note contents:

- frontend ID and state layout version
- PC, SP, TLS, scalar status, and valid masks
- complete GPR state for the selected frontend
- complete vector/FPU state for the selected frontend when present
- current event sequence, event kind, fault address, and raw syndrome
- transition-stack depth and top return cookie metadata

`PDUMP_STATE` must work for the live current thread and for a valid spilled
state image selected through a spill descriptor. Multi-thread core dumps remain
userspace policy: the monitor enumerates threads, asks each thread or spill
image for a note blob, then writes ELF notes, minidump streams, or another file
format. The current Linux `polyexec` runtime uses this split directly: when
`POLYEXEC_FATAL_DEBUG_NOTE_DIR` is set, an unhandled auto-spilled fatal fault
writes an ELF64 `ET_CORE`/`PT_NOTE` container whose vendor `POLY` note payload
uses this `poly_v2_debug_note` layout. The runtime asks `PDUMP_STATE` first and
can synthesize the same layout directly from the validated spill descriptor's
state/event pair when the fault context cannot execute a live dump control.
Linux/GDB-specific signal notes, register notes, mapped-file notes, and memory
segments are intentionally a higher-level runtime compatibility layer, not part
of the ISA.
The current Linux `polyexec` runtime implements that layer for fatal
single-thread cores by wrapping the OS-neutral note/state record in ELF64
`ET_CORE` files with `NT_SIGINFO`, `NT_PRSTATUS`, `NT_PRPSINFO`, `NT_AUXV`,
main-executable `NT_FILE` metadata, executable and stack `PT_LOAD` segments,
and symbolized fatal frames that `gdb-multiarch` can load for AArch64 and
RISC-V inspection. Broader multi-object `NT_FILE` enumeration and perfect
unwind metadata remain runtime/debugger-format work above `PDUMP_STATE`.

## Memory Probe

`PMEM_PROBE_RANGE(addr, len, flags)` answers whether the current process virtual
range is readable, writable, executable, and canonical according to the active
address-space state. It does not install mappings, fault pages in through an OS
handler, or suppress real faults for later code.

Return metadata:

- first failing address
- readable/writable/executable/canonical bits
- page-crossing and alignment diagnostics
- permission-failure vs unmapped/translation-failure classification when the
  hardware can distinguish them

Exact prototype register ABI:

- input `RAX=addr`, `RDX=len`, `RCX=flags`
- output `RAX=status`, `RDX=first_failure_addr`, `RCX=result_metadata`

Supported request flags are `READ`, `WRITE`, and `EXECUTE`. A zero flag value
probes canonicality, translation, and common range capabilities without
requiring a specific access class. `status` is `0` when every page in the range
is translated and satisfies the requested access classes. It is `-EINVAL` for
unsupported flags, `-EFAULT` for noncanonical, overflowed, unmapped, or
translation-failed ranges, and `-EACCES` for translated ranges that fail a
requested permission. `result_metadata` reports canonical, present, readable,
writable, executable, cross-page, page-aligned, unsupported, noncanonical,
overflow, unmapped/translation-failed, and permission-failed bits. Hardware
must not set accessed/dirty bits, allocate pages, invoke OS fault handlers, or
deliver a host signal as part of this operation.

This replaces monitor-side `/proc/self/maps` parsing and intentional host
SIGSEGV probes in performance-sensitive paths.

## Thread State Derivation

`PDERIVE_STATE` creates a child frontend state image from a parent image and a
small userspace descriptor:

- child frontend SP
- child frontend TLS
- child return-value register
- parent return-value register
- optional cleared signal/event/pending-interrupt state
- optional state-key replacement

The OS still creates threads and processes. `PDERIVE_STATE` only initializes
Poly architectural state after userspace has an OS thread or process in which
to run it. This accelerates clone/fork/vfork-style runtimes without naming
those OS calls in the ISA.

Exact v2 descriptor layout:

```c
struct poly_v2_derive_descriptor {
  uint64_t magic;              // "POLYDRV2"
  uint32_t bytes;              // 128
  uint16_t version;            // 2
  uint16_t header_bytes;       // 16
  uint64_t flags;
  uint32_t frontend;           // AArch64 or RISC-V raw frontend
  uint32_t reserved0;
  uint64_t child_sp;
  uint64_t child_tls;
  uint64_t child_return_value;
  uint64_t parent_return_value;
  uint64_t state_key;
  uint64_t reserved[7];
};
```

`flags` may request child SP, child TLS, child return-value, parent
return-value, event-state clearing, and state-key replacement. Hardware copies
the parent `poly_xsave_state` to the child image, applies child register/TLS
updates for the selected frontend, optionally writes the parent return-value
register back into the parent image, and optionally clears trap/event,
transition, import-return, cross-return, trap-restore, and native-return state
in the child image. Unsupported frontends, nonzero reserved bits, malformed
headers, bad state images, and non-canonical descriptor-controlled addresses are
rejected before mutation.

## Policy Preflight For Seccomp-Like Filters

v2 does not implement BPF. Instead, every OS-bound event exposes the canonical
event frame before the monitor performs a host syscall. A Linux runtime can feed
`frontend`, `selector`, `args[]`, and `insn_pc` into a userspace cBPF evaluator.
Another OS can use another policy engine.

The hardware contract is only:

- publish complete preflight arguments before monitor entry
- identify the correct resume PC and completion state
- never apply guest policy to host x86 syscalls by accident

The current `polyexec` Linux runtime consumes that contract by intercepting
guest `prctl(PR_SET_SECCOMP)` and `seccomp(SECCOMP_SET_MODE_FILTER)`, copying
classic BPF filters into monitor-owned memory, evaluating them against guest
syscall numbers and arguments before host syscall translation, and returning
guest `ERRNO` decisions without installing the guest filter on the host x86
thread.

## Shared-Memory Ring Ordering

v2 does not add io_uring instructions. It strengthens the memory-order contract
around monitor-visible shared memory:

- foreign stores before a syscall-like event are visible to x86 monitor code
  before the monitor issues a host syscall
- x86 monitor or host-shared writes before `PDERIVE_STATE/PENTER` are visible to the
  resumed foreign frontend
- `PFENCE scope` exposes explicit acquire/release/full ordering points for
  runtimes that hand ownership of rings or queues between frontends

Struct layout remains OS/runtime policy. The ISA only guarantees visibility and
ordering in the shared address space.

## ABI Bridge Descriptors

v2 should not move full ABI classification into hardware. Hardware keeps the
v1 rule: fast paths are register-only aliases, and complex memory-shaped ABI
work remains userspace.

The v2 improvement is a software-owned, versioned ABI descriptor format that
the runtime can use to generate or cache trampoline JIT code. The current
runtime layer uses `poly_abi_descriptor` to decode process bridge cases into
typed function signatures and bridge plans; hardware does not interpret those
descriptors. A future hardware cache may cache only the register-alias subset
after validation:

- source/target frontend
- integer and vector register alias maps
- TLS-base alias bit
- return-register map
- explicit "no stack/aggregate/variadic work" flag

Descriptors that require stack repacking, aggregate decomposition, variadics,
or hidden memory returns must fall back to userspace thunks.

## Performance And Diagnostics

v2 should expose counters and a small optional event-history ring:

- events by kind/frontend
- auto-spill count, bytes, and cycles
- failed descriptor validations
- memory-probe failures
- v2 derive/import activations and rejected generations
- policy-preflight exits
- shared-memory fence counts

These counters are diagnostic. They must not alter architectural state or
become required for correctness.

## Implementation Order

1. Add CPUID leaves for v2 draft discovery and the sizes/alignments above.
2. Implement `PSET_EVENT_PTR` and canonical event-frame publication in Bochs.
3. Teach `polyexec` to consume `resume_pc`, `completion`, `fault_address`, and
   `args[]` from the event frame instead of reconstructing them.
4. Replace raw spill pointer programming with `PSET_SPILL_DESC`.
5. Add `PDUMP_STATE` and build userspace ELF core-note synthesis on top of it.
6. Add `PMEM_PROBE_RANGE` and remove `/proc/self/maps` probes from hot paths.
7. Add `PDERIVE_STATE` for clone/thread handoff.
8. Add `PCOMPLETE_EVENT` for trap-return result and resume-state completion.
9. Strengthen `PFENCE`/shared-memory ordering tests before enabling io_uring
   pass-through experiments.
10. Keep ABI descriptor work in userspace. Only advertise hardware descriptor
   caching after a descriptor proves it is a pure register-alias fast path and
   has emulator/runtime tests.

The first breaking change should be the event frame. Once the monitor consumes
that cleanly, signals, core dumps, seccomp mediation, clone handoff, and
shared-memory I/O become smaller follow-on changes instead of more special
cases in `polyexec`.

## Conformance Matrix

| Rule | Evidence |
| --- | --- |
| x86 opcode family is CPUID-discovered and prototype-owned | CPUID opcode geometry and opcode ownership leaves; `POLY_X86_OPCODE_FLAG_VENDOR_PROTOTYPE`; readiness check |
| Invalid frontend IDs are rejected before mutation | Bochs control validation; nativecheck invalid frontend probes |
| Canonical and frontend alignment faults are pre-mutation | Bochs target validation; nativecheck noncanonical/alignment probes |
| ABI signature slot fast path remains register-only | `polycpuid.h` signature-slot masks; nativecheck signature probes; ABI descriptor tests |
| v2 event-frame ordering precedes monitor redirect | Bochs `PSET_EVENT_PTR` publication; polyexec/nativecheck event-frame dispatch; architecture contract check |
| Return-cookie recovery remains native-return based | Bochs/nativecheck return-cookie probes; RTL transition-stack tests |
| TSO barriers/fences preserve x86 shared-memory ordering | memory-order CPUID bit; RTL memory-order tests |
| User spill import/export uses the 8KB explicit state image | `poly_xsave_state` layout; `polylayout --check`; state import/export tests |
| Auto-spill trampoline uses descriptor-backed v2 state | `PSET_SPILL_DESC`, `PDERIVE_STATE ACTIVATE_DST`, page-fault/preemption gates |
| CPUID discovery covers v2 sizes and implemented features | `POLY_CPUID_BASE + 10`; CPUID contract check |
| Zero-kernel-change OS behavior remains intact | unmodified Bochs/Linux boot gates; polyexec page-fault and nativecheck gates |
