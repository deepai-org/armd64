# Poly ISA

Poly is an x86_64 extension for running existing x86_64, AArch64, and RISC-V64
user-mode code in one virtual address space. This is the quick contract;
rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.
- x86_64 remains the system ISA: privilege, paging, faults, interrupts, VM
  control, syscalls, atomics, and TSO ordering stay x86-owned.
- AArch64 and RISC-V64 are user-mode frontends that fetch native instructions
  from `RIP`; Poly controls are decoded instructions, not `#UD` envelopes.
- Foreign state is explicit per-thread XSAVE-style architectural state.
- Hardware does not parse ELF, libc, syscall numbers, stacks, aggregates, or
  user-memory call descriptors.

## Controls

- `PENTER f`: enter frontend `f` at fall-through.
- `PSWITCH f, target`: tail-switch to frontend `f` and PC.
- `PCALL f, target, sig`: cross-frontend call using ABI signature slot `sig`.
- `PTRAPRET`: return from the user-space Poly trap monitor.
- `PLANDING`: validate an indirect cross-frontend landing pad.

## ABI Boundary

`PCALL` is fixed-latency and register-only. Signature slots may rename integer,
FP, and fixed-vector argument registers, but never read memory, repack stacks,
classify aggregates, translate syscalls, or call helpers. Loader/runtime thunks
handle those complex cases. Null signature windows:

- `P0..P7`: x86_64 `RAX,RDX,RCX,RDI,RSI,R8,R9,R10`, AArch64 `x0..x7`,
  RISC-V64 `a0..a7`.
- `F0..F7`: x86_64 `XMM0..XMM7`, AArch64 `v0..v7`, RISC-V64 `fa0..fa7`.

## Encodings

Prototype control encodings are x86_64 `0f 3a fc <subop>`, AArch64 reserved
`HINT` `0xd503201f | (subop << 5)`, and RISC-V64 `custom-0`
`0x0000700b | (subop << 25)`.
