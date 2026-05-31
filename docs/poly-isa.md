# Poly ISA

Quick reference for the Poly CPU extension. See
[poly-isa-design-directions.md](poly-isa-design-directions.md) for rationale.

## Model

Poly lets existing x86_64, AArch64, and RISC-V64 user code share one x86_64
virtual address space. x86_64 remains the system ISA for privilege, paging,
faults, interrupts, VM control, syscalls, atomics, and global TSO ordering.

AArch64 and RISC-V64 are user-mode frontends that fetch native 32-bit
instructions from `RIP`. Poly control flow uses decoded instructions, not `#UD`
envelopes. Extra foreign registers are per-thread XSAVE-style state.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Operations

- `PENTER frontend`: enter a frontend at fall-through.
- `PSWITCH frontend, target`: tail-switch to another frontend and PC.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: return from the user-space Poly trap monitor.
- `PLANDING`: validate an indirect cross-ISA landing pad.

`PCALL` is fixed-latency and register-only. Signature slots may rename integer,
FP, and fixed-vector argument registers. Runtime thunks handle stacks,
aggregates, variadics, ELF, libc, and syscalls.

Null-signature window: `P0..P7` maps x86_64 `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` to
AArch64 `x0..x7` and RISC-V64 `a0..a7`; `F0..F7` maps x86_64 `XMM0..XMM7` to
AArch64 `v0..v7` and RISC-V64 `fa0..fa7`.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
