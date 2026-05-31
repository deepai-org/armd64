# Poly ISA

Poly extends x86_64 so existing x86_64, AArch64, and RISC-V64 user-mode code can
share one virtual address space. Rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Model

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.
- x86_64 remains the system ISA: privilege, paging, faults, interrupts, VM
  control, syscalls, atomics, and TSO memory ordering stay x86-owned.
- AArch64 and RISC-V64 are user-mode decode frontends that fetch native
  instructions from `RIP`.
- Poly controls are real decoded instructions, not `#UD` envelopes.
- Non-aliased foreign registers are explicit per-thread XSAVE-style state.

## Instructions

- `PENTER f`: enter frontend `f` at fall-through.
- `PSWITCH f, target`: tail-switch to frontend `f` and PC.
- `PCALL f, target, sig`: cross-frontend call using ABI signature slot `sig`.
- `PTRAPRET`: return from the user-space Poly trap monitor.
- `PLANDING`: validate an indirect cross-frontend landing pad.

## Call Boundary

`PCALL` is fixed-latency and register-only. Signature slots may rename integer,
FP, and fixed-vector argument registers. Hardware does not parse ELF, libc,
syscalls, stacks, aggregates, or user-memory call descriptors; runtime thunks
handle those cases.

- `P0..P7`: x86_64 `RAX,RDX,RCX,RDI,RSI,R8,R9,R10`; AArch64 `x0..x7`;
  RISC-V64 `a0..a7`.
- `F0..F7`: x86_64 `XMM0..XMM7`; AArch64 `v0..v7`; RISC-V64 `fa0..fa7`.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
