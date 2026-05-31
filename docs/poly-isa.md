# Poly ISA

Poly adds hardware-visible AArch64 and RISC-V64 user-mode frontends to x86_64.
The target is existing precompiled-code compatibility and fast cross-ISA
linking, not a new compiler-only ABI.

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns privilege, paging, faults, interrupts, atomics, VM control,
  syscalls, and TSO ordering.
- Foreign frontends fetch native 32-bit instructions from the same virtual
  address space.
- Poly controls are decoded instructions, not `#UD` envelopes.
- Extra foreign architectural state is explicit per-thread XSAVE-style state.
- Hardware is OS-neutral: no Linux, libc, linker, or syscall-policy emulation.

## Controls

- `PENTER frontend`: enter a frontend at fall-through PC.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: cross-call using ABI signature slot `sig`.
- `PTRAPRET`: return from a userspace Poly trap monitor.
- `PLANDING`: mark or validate an indirect cross-ISA landing pad.

## ABI Boundary

Fast `PCALL` is register-only. Signature slots may remap argument and return
register names in rename hardware, but must not read memory, parse descriptors,
repack stacks, classify aggregates, translate syscalls, or invoke helpers.

Exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.

Software thunks handle stack arguments, variadics, by-value aggregates,
incompatible vectors, lazy binding, imports, and syscall policy.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | (subop << 5)` reserved `HINT`
- RISC-V64: `0x0000700b | (subop << 25)` `custom-0`

Run commands: [../README.md](../README.md). Rationale:
[poly-isa-design-directions.md](poly-isa-design-directions.md).
