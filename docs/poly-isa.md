# Poly ISA

Poly lets one x86_64 process link and execute precompiled x86_64, AArch64, and
RISC-V64 user-mode code in one virtual address space.

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns privilege, paging, interrupts, faults, atomics, VM control,
  syscalls, and TSO memory ordering.
- Foreign frontends fetch native 32-bit instructions directly from `RIP`.
- Poly controls are decoded opcodes, not `#UD` traps or instruction envelopes.
- Non-aliased foreign state is per-thread XSAVE-style architectural state.
- Hardware is OS-neutral: it never parses Linux, libc, linker, syscall, stack,
  aggregate, or descriptor policy.

## Controls

- `PENTER frontend`: enter `frontend` at the fall-through PC.
- `PSWITCH frontend, target`: tail-switch to `frontend:target`.
- `PCALL frontend, target, sig`: call `frontend:target` with ABI signature slot
  `sig`.
- `PTRAPRET`: return from a user-space Poly trap monitor.
- `PLANDING`: validate an indirect cross-ISA landing pad.

## ABI Boundary

Fast `PCALL` is register-only. ABI signature slots may rename registers but must
not read memory, repack stacks, classify aggregates, translate syscalls, or call
helpers.

- Integer window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = AArch64 `x0..x7` = RISC-V
  `a0..a7`.
- FP window: `XMM0..XMM7` = AArch64 `v0..v7` = RISC-V `fa0..fa7`.
- Software thunks handle stack args, variadics, by-value aggregates,
  incompatible vectors, lazy binding, imports, and syscall policy.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.

Run commands: [../README.md](../README.md). Design rationale:
[poly-isa-design-directions.md](poly-isa-design-directions.md).
