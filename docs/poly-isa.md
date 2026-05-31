# Poly ISA

Poly lets one x86_64 process execute precompiled x86_64, AArch64, and RISC-V64
user-mode code in one virtual address space. The goal is native ABI
compatibility and fast cross-ISA linking, not a new compiler-only ABI.

## System Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  atomics, VM control, syscalls, and TSO memory ordering.
- AArch64 and RISC-V fetch native instructions directly from the same virtual
  address space.
- Poly controls are decoded instructions, not `#UD` envelopes.
- Extra foreign architectural state is per-thread XSAVE-style state.
- Hardware stays OS-neutral: the CPU does not parse Linux, libc, linker,
  syscall, or descriptor policy.

## Control Instructions

- `PENTER frontend`: enter a frontend at the fall-through PC.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: cross-call using ABI signature slot `sig`.
- `PTRAPRET`: return from a userspace Poly trap monitor.
- `PLANDING`: mark or validate an indirect cross-ISA landing pad.

## ABI Boundary

Fast `PCALL` is register-only. ABI signature slots may rename registers, but
must not read memory, parse descriptors, repack stacks, classify aggregates,
translate syscalls, or call helpers.

Exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = AArch64 `x0..x7` = RISC-V
`a0..a7`; `XMM0..XMM7` = AArch64 `v0..v7` = RISC-V `fa0..fa7`.

Software thunks handle stack arguments, variadics, by-value aggregates,
incompatible vectors, lazy binding, imports, and syscall policy.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.

Run commands: [../README.md](../README.md). Rationale:
[poly-isa-design-directions.md](poly-isa-design-directions.md).
