# Poly ISA Quick Reference

Poly is an x86_64 CPU extension for running existing x86_64, AArch64, and
RISC-V64 user-mode binaries in one process address space.

## Difference From x86_64

- x86_64 remains the system ISA: privilege, paging, interrupts, faults,
  atomics, VM control, syscalls, and TSO memory ordering stay x86-owned.
- AArch64 and RISC-V64 are user-mode frontends. They fetch native 32-bit
  instructions directly from `RIP`; there are no per-instruction envelopes.
- Poly controls are decoded instructions, not `#UD` traps.
- Extra foreign state is explicit per-thread XSAVE-style architectural state.
- Hardware stays OS-neutral. It does not parse libc, Linux, linker policy,
  stack layouts, aggregates, syscall numbers, or user-memory descriptors.

## Frontends

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |
| `3..255` | reserved |

## Control Operations

| Operation | Meaning |
| --- | --- |
| `PENTER frontend` | Enter a frontend at the fall-through PC. |
| `PSWITCH frontend, target` | Tail-switch to another frontend and PC. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PTRAPRET` | Return from the user-space Poly trap monitor. |
| `PLANDING` | Validate an indirect cross-ISA landing pad. |

## Fast ABI Boundary

`PCALL` is fixed-latency and register-only. Signature slots may remap register
names, but must not read memory, repack stacks, classify aggregates, translate
syscalls, or call helpers.

Default exchange window:

| Poly | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `P0..P7` | `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` | `x0..x7` | `a0..a7` |
| `F0..F7` | `XMM0..XMM7` | `v0..v7` | `fa0..fa7` |

Software thunks handle stack arguments, variadics, by-value aggregates,
incompatible vectors, lazy binding, imports, and syscall policy.

## Temporary Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`

For commands, see [../README.md](../README.md). For rationale and future
directions, see [poly-isa-design-directions.md](poly-isa-design-directions.md).
