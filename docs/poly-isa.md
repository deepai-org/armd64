# Poly ISA Reference

Poly is an x86_64 extension for running existing x86_64, AArch64, and RISC-V64
user-mode code in one process address space. This file is the short ISA
contract; rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Execution Model

- x86_64 remains the system ISA for privilege, paging, faults, interrupts,
  atomics, VM control, syscalls, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode frontends that fetch native 32-bit
  instructions directly from `RIP`.
- Poly controls are decoded instructions, not `#UD` envelopes.
- Foreign architectural state is explicit per-thread XSAVE-style state.
- Hardware is OS-neutral: no ELF, libc, syscall-number, stack-layout,
  aggregate-layout, or user-memory descriptor parsing.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.

## Control Operations

| Operation | Meaning |
| --- | --- |
| `PENTER frontend` | Enter a frontend at the fall-through PC. |
| `PSWITCH frontend, target` | Tail-switch to another frontend and PC. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PTRAPRET` | Return from the user-space Poly trap monitor. |
| `PLANDING` | Validate an indirect cross-ISA landing pad. |

## Call Boundary

`PCALL` is fixed-latency and register-only. Signature slots may rename
architectural registers, but they must not touch memory, repack stacks,
classify aggregates, translate syscalls, or call helpers.

The null signature exposes this low-level exchange window:

| Poly | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `P0..P7` | `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` | `x0..x7` | `a0..a7` |
| `F0..F7` | `XMM0..XMM7` | `v0..v7` | `fa0..fa7` |

Software thunks handle stack arguments, variadics, by-value aggregates,
incompatible vectors, lazy binding, imports, and syscall policy.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`

Run commands are documented in [../README.md](../README.md).
