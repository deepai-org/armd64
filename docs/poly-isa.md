# Poly ISA Quick Reference

Poly extends an x86_64 userspace process with direct AArch64 and RISC-V64
frontends. The goal is compatibility with existing native ABI code and shared
libraries, not a new compiler-only ABI and not one trap per foreign instruction.

Design rationale lives in `docs/poly-isa-design-directions.md`.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## Difference From x86_64

- x86_64 stays the system ISA: boot, privilege, paging, faults, interrupts,
  VM control, atomics, and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends sharing the same process address
  space, page tables, permissions, and stack memory.
- AArch64 fetches aligned 32-bit instructions directly. RISC-V fetches 16-bit
  RVC and 32-bit instructions directly.
- ISA transitions use decoded Poly control instructions, not `#UD` envelopes.
- Foreign register state is explicit per-thread XSAVE-style state.
- Hardware handles frontend switches, register-only ABI signatures, precise
  traps, and XSAVE state. Runtime software handles stack arguments, aggregates,
  variadics, lazy binding, syscall policy, and other ABI reshaping.

## Controls

| Operation | Role |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PLANDING` | Validate indirect cross-frontend targets when enabled. |
| `PTRAPRET` | Resume after a precise Poly trap. |

## Temporary Encodings

Current Bochs/test encodings are placeholders, not vendor allocations.

| ID | Frontend | Fetch | Test encoding |
| --- | --- | --- | --- |
| `0` | x86_64 | variable length | `0f 3a fc <subop>` |
| `1` | AArch64 | 32-bit aligned | `0xd503201f | (subop << 5)` |
| `2` | RISC-V64 | 16-bit RVC plus 32-bit | `0x0000700b | (subop << 25)` |

Constants live in `tools/include/polycpuid.h`.
