# Poly ISA

Minimal reference. Rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Contract

Poly runs existing x86_64, AArch64, and RISC-V64 user code in one x86_64 virtual
address space. x86_64 remains the system ISA for privilege, paging, interrupts,
faults, syscalls, VM control, atomics, and global TSO ordering.

AArch64 and RISC-V64 are user frontends. They fetch native 32-bit instructions
from `RIP`, not `#UD` envelopes. Extra foreign state is per-thread XSAVE-style
state. Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Instructions

- `PENTER frontend`: enter a frontend at the fall-through PC.
- `PSWITCH frontend, target`: tail-switch to another frontend and PC.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: return from the user-space Poly trap monitor.
- `PLANDING`: validate an indirect cross-ISA landing pad.

`PCALL` is fixed-latency and register-only. Signature slots may alias integer,
FP, and fixed-vector argument registers. Software thunks handle stack arguments,
aggregates, variadics, ELF relocation, libc, and syscall policy.

Default window: `P0..P7` maps x86_64 `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` to
AArch64 `x0..x7` and RISC-V64 `a0..a7`; `F0..F7` maps x86_64 `XMM0..XMM7` to
AArch64 `v0..v7` and RISC-V64 `fa0..fa7`.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
