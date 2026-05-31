# Poly ISA Quick Reference

Poly adds hardware-visible x86_64, AArch64, and RISC-V64 frontends inside one
x86_64 virtual address space. x86_64 stays responsible for privilege, paging,
faults, interrupts, syscalls, VM control, atomics, and TSO memory ordering.

## Frontends

| ID | Frontend |
| -- | -- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |

Foreign frontends fetch native 32-bit instructions directly from `RIP`. There
are no single-instruction `#UD` envelopes. Non-aliased foreign registers are
per-thread XSAVE-style architectural state.

## Controls

| Control | Purpose |
| -- | -- |
| `PENTER frontend` | Enter a frontend at the fall-through PC. |
| `PSWITCH frontend, target` | Tail-switch to another frontend and PC. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PTRAPRET` | Return from the user-space Poly trap monitor. |
| `PLANDING` | Validate an indirect cross-ISA landing pad. |

`PCALL` is fixed-latency and register-only. Software thunks handle stack
arguments, memory-shaped aggregates, variadics, loader policy, libcalls, and
syscall translation.

## Register Window

| Poly | x86_64 | AArch64 | RISC-V64 |
| -- | -- | -- | -- |
| `P0..P7` | `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` | `x0..x7` | `a0..a7` |
| `F0..F7` | `XMM0..XMM7` | `v0..v7` | `fa0..fa7` |

## Prototype Encodings

| ISA | Encoding |
| -- | -- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | reserved `HINT`: `0xd503201f | (subop << 5)` |
| RISC-V64 | `custom-0`: `0x0000700b | (subop << 25)` |

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
