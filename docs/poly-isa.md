# Poly ISA

Poly is an x86_64 extension for running existing x86_64, AArch64, and RISC-V64
user-mode code in one virtual address space. This is the short contract; design
rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.
- x86_64 stays responsible for privilege, paging, faults, interrupts, VM
  control, syscalls, atomics, and TSO ordering.
- AArch64 and RISC-V64 are user frontends fetched directly from `RIP`.
  AArch64 is fixed 32-bit; RISC-V may use 16-bit compressed instructions.
- Poly controls are decoded instructions, not `#UD` envelopes.
- Foreign architectural state is explicit per-thread XSAVE-style state.
- Hardware must not parse ELF, libc, syscall numbers, stacks, aggregates, or
  user-memory call descriptors.

## Controls

`PENTER` enters a frontend at fall-through. `PSWITCH` tail-switches to a
frontend and PC. `PCALL` calls a frontend and PC using ABI signature slot `sig`.
`PTRAPRET` returns from the user-space Poly trap monitor. `PLANDING` validates
an indirect cross-ISA landing pad.

## Calls

`PCALL` is fixed-latency and register-only. Signature slots may rename
architectural registers, but they never read memory, repack stacks, classify
aggregates, translate syscalls, or call helpers.

Null-signature exchange window: `P0..P7` maps to x86_64
`RAX,RDX,RCX,RDI,RSI,R8,R9,R10`, AArch64 `x0..x7`, and RISC-V64 `a0..a7`.
`F0..F7` maps to x86_64 `XMM0..XMM7`, AArch64 `v0..v7`, and RISC-V64
`fa0..fa7`.

Software thunks handle everything memory-shaped: stack arguments, variadics,
by-value aggregates, incompatible vectors, lazy binding, imports, and syscall
policy.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`

Run commands are in [../README.md](../README.md).
