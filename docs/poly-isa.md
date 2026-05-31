# Poly ISA

Poly runs existing x86_64, AArch64, and RISC-V64 user code in one x86_64
virtual address space. x86_64 remains the system ISA for privilege, paging,
interrupts, faults, syscalls, VM control, atomics, and TSO ordering.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64. Foreign frontends fetch
native instructions directly from `RIP`; there are no per-instruction `#UD`
envelopes. Extra foreign state is per-thread XSAVE-style architectural state.

Controls:

- `PENTER frontend`: enter a frontend at the fall-through PC.
- `PSWITCH frontend, target`: tail-switch to another frontend and PC.
- `PCALL frontend, target, sig`: cross-ISA call through ABI signature slot `sig`.
- `PTRAPRET`: return from the user-space Poly trap monitor.
- `PLANDING`: validate an indirect cross-ISA landing pad.

`PCALL` is fixed-latency and register-only. Software thunks handle stack
arguments, memory-shaped aggregates, variadics, loader policy, libc, and syscall
translation.

Register window: `P0..P7` maps x86_64 `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` to
AArch64 `x0..x7` and RISC-V64 `a0..a7`; `F0..F7` maps x86_64 `XMM0..XMM7` to
AArch64 `v0..v7` and RISC-V64 `fa0..fa7`.

Prototype opcode pages: x86_64 `0f 3a fc <subop>`; AArch64 reserved `HINT`
`0xd503201f | (subop << 5)`; RISC-V64 `custom-0`
`0x0000700b | (subop << 25)`.

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
