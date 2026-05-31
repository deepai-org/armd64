# Poly ISA

Poly is an x86_64 extension for running existing x86_64, AArch64, and RISC-V64
user-mode code in one virtual address space. This is the short contract;
rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.
- x86_64 owns privilege, paging, faults, interrupts, VM control, syscalls,
  atomics, and TSO ordering. Foreign frontends are user-mode only.
- AArch64 and RISC-V64 fetch directly from `RIP`; Poly controls are decoded
  instructions, not `#UD` envelopes.
- Foreign state is explicit per-thread XSAVE-style architectural state.
- Hardware must not parse ELF, libc, syscall numbers, stacks, aggregates, or
  user-memory call descriptors.

## Controls

- `PENTER frontend`: enter a frontend at fall-through.
- `PSWITCH frontend, target`: tail-switch to a frontend and PC.
- `PCALL frontend, target, sig`: cross-frontend call through ABI signature slot
  `sig`.
- `PTRAPRET`: return from the user-space Poly trap monitor.
- `PLANDING`: validate an indirect cross-frontend landing pad.

## ABI Boundary

`PCALL` is fixed-latency and register-only. ABI signature slots may rename
architectural registers, including FP/vector argument registers, but never read
memory, repack stacks, classify aggregates, translate syscalls, or call helpers.
Complex calls use loader/runtime thunks. Null signature windows:

- `P0..P7`: x86_64 `RAX,RDX,RCX,RDI,RSI,R8,R9,R10`, AArch64 `x0..x7`,
  RISC-V64 `a0..a7`.
- `F0..F7`: x86_64 `XMM0..XMM7`, AArch64 `v0..v7`, RISC-V64 `fa0..fa7`.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`
