# Poly ISA Quick Reference

Bochs prototype for running existing precompiled x86_64, AArch64, and RISC-V64
user code in one x86_64 virtual address space. Design rationale lives in
[`poly-isa-design-directions.md`](poly-isa-design-directions.md). The active
state, event, and monitor contract is the v2 draft in
[`poly-isa-v2-draft.md`](poly-isa-v2-draft.md). The older v1 writeup has been
retired and is not an active implementation target.

```bash
make image
make check-poly-contracts
```

## Contract

- x86_64 is still the system ISA: boot, privilege, paging, interrupts, faults,
  atomics, syscalls, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode decode frontends in the same address space.
- ISA switches are decoded control instructions, not `#UD` traps.
- AArch64 uses 32-bit aligned fetch. RISC-V64 supports 16/32-bit fetch.
- `PCALL` switches ISA and can apply cached native-ABI register aliases.
- Foreign state is an explicit 8KB user-owned spill image; an unmodified OS is
  not required to save a custom Poly xstate component.
- Recoverable foreign traps write OS-neutral trap records.
- Foreign syscall numbers and import selectors are opaque trap fields; hardware
  does not translate them into OS calls or libc/helper functions.
- Import trap selectors are opaque CPU values; named libc/libgcc bindings are
  runtime policy, not CPUID/ISA features.
- Software handles stack arguments, aggregates, variadics, lazy binding,
  syscalls, libcalls, and debugger policy.

## Control Opcode Allocations

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops cover `PENTER`, `PSWITCH`, `PCALL`, signature-slot calls, setup/query,
`PLANDING`, `PTRAPRET`, and `PSET_EVENT_PTR`. The prototype also contains
opt-in spill-descriptor controls used by legacy stress tests, but that path is
not advertised as a production v2 feature because it makes the monitor depend
on raw interrupt-time userspace memory being reachable through the post-entry
host exception context. ABI signature setup writes a register-only signature slot as
`kind | (register_map << 32)`. Query returns the same encoded value, so
runtimes and hardware can verify the exact RAT/register-map policy instead of
inferring it from the kind alone.

These are fixed-latency decoded control operations: they do not read user
descriptors, repack stacks, or enter exception delivery for normal frontend
switching. The x86_64 byte family is the Bochs allocation for this prototype;
silicon/FPGA implementations can allocate a different dedicated opcode family,
but must expose its geometry through Poly CPUID so runtimes do not infer it
from documentation.

CPUID `POLY_CPUID_BASE + 2, subleaf 32` reports the active x86 byte geometry.
Subleaf 33 reports the opcode contract flags: the current Bochs encoding is a
vendor/prototype dedicated decode path, not a `#UD` trap envelope, and software
must discover it through CPUID instead of hard-coding it as a production x86
allocation.

CPUID `POLY_CPUID_BASE + 10` reports the implemented v2 feature mask, not the
entire v2 draft roadmap. The active mask currently covers canonical event
frames, `PDUMP_STATE` debug-note export, `PMEM_PROBE_RANGE` non-faulting
memory accessibility probes, `PDERIVE_STATE` thread-state derivation/import,
`PCOMPLETE_EVENT` trap-result completion, `PFENCE` shared-memory ordering,
the hardware monitor-entry stack/frame contract, and policy preflight for
seccomp-style userspace mediation. Core-file or
minidump wrapping remains userspace policy on top of the debug-note blob;
`polyexec` now has an opt-in fatal-fault path that writes a Linux ELF64
`ET_CORE` container with the OS-neutral `POLY` note from `PDUMP_STATE`,
GDB-compatible `NT_PRSTATUS`/`NT_PRPSINFO`/`NT_AUXV` notes, and a stack
`PT_LOAD` segment. Mapped-object `NT_FILE` metadata and fully symbolic unwind
polish remain userspace follow-up work, not ISA policy. Draft features such as
hardware ABI-descriptor caching and diagnostic counters must not be advertised
until their control paths and tests exist. The software-owned ABI descriptor
decoder is runtime policy and does not require a hardware feature bit.
`docs/poly-isa-userspace-offload-proposal.md` is the canonical proposal for
moving the remaining clone/signal/result/debug/shared-memory burden out of
`polyexec` without baking Linux policy into the ISA.

## FPGA/Silicon ISA Readiness Boundary

The ISA contract is ready for FPGA or silicon implementation when hardware can
implement the architecture below without inheriting emulator or runtime policy:

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, syscalls, VM control, and global TSO memory ordering.
- AArch64 and RISC-V64 are user-mode native fetch frontends in the same virtual
  address space, using their native instruction-width and alignment rules.
- Poly control transfers are fixed-latency decoded control operations, not
  `#UD` trap envelopes or software descriptor calls.
- The active x86 control opcode family is CPUID-discovered. The current
  `0f 3a fc <subop>` allocation is a vendor/prototype dedicated decode page,
  not a production x86 allocation; silicon may reassign it behind the same
  discovery contract.
- `PCALL` fast paths use register-only ABI signature slots. Hardware may apply
  those slots in rename/RAT logic; stack arguments, aggregates, variadics,
  lazy binding, syscall translation, libcalls, debugger policy, and helper
  imports remain userspace runtime policy.
- Foreign architectural state is explicit and user-owned. Production v2 relies
  on canonical event frames plus OS-neutral state transforms such as
  `PDERIVE_STATE`, `PCOMPLETE_EVENT`, and `PDUMP_STATE`, not on a monitor-owned
  interrupt-time trampoline. The descriptor-backed auto-spill prototype has
  been retired and is not advertised through CPUID.
- `PSET_EVENT_PTR` registers the per-thread canonical event frame. Recoverable
  traps, syscalls, and faults publish precise event/result metadata to that
  frame and return through the normal monitor-vector path.
- Recoverable exits publish OS-neutral v2 event frames before monitor-vector
  redirect. The default `polyexec` trap-vector path consumes that v2 event
  frame directly. Failed event-frame writes or invalid event addresses prevent
  the redirect and report precise faults.
- Native returns use ordinary frontend return instructions and a hardware
  transition-stack return cookie; same-ISA returns remain normal.
- Invalid frontend IDs, non-canonical targets, frontend alignment violations,
  invalid ABI slots, transition-stack overflow/underflow, and trap-restore
  conflicts are rejected before mutating architectural frontend/PC state.
- Foreign barriers/fences are explicit x86-TSO no-ops, and foreign memory
  operations cannot expose weak reordering in the shared x86 address space.

Opcode ownership, reserved-bit policy, CPUID leaves, spill-image layout,
canonical event frames, error precedence, zero-kernel OS contract, and
conformance evidence are now carried by `docs/poly-isa-v2-draft.md`. This
boundary deliberately says nothing about FPGA fabric, Verilog structure, or
timing closure. Those are implementation and productization tasks. The ISA
readiness requirement is that the hardware contract is explicit, discoverable,
fixed-latency where required, and free of OS/libc/runtime policy.
