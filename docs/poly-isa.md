# Poly ISA Quick Reference

Poly lets existing x86_64, AArch64, and RISC-V64 native ABI code run and call
across ISAs inside one x86_64 process.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## How It Differs From x86_64

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, faults,
  VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are direct-fetch user frontends:
  AArch64 fetches aligned 32-bit instructions, and RISC-V64 fetches RVC/32-bit.
- All frontends share one virtual address space, page permissions, stack memory,
  and process resource model.
- Foreign architectural state is per-thread XSAVE-style state. It is not keyed
  only by CR3 and is not private emulator bookkeeping.
- Poly transitions are decoded control instructions, not `#UD` envelopes.
- Hardware handles frontend switches, register-only ABI signatures, precise
  traps, native return-cookie recovery, and XSAVE state.
- Runtime software handles stack arguments, aggregates, variadics, lazy binding,
  syscall policy, helper calls, and other memory-shaped ABI work.

## Control Operations

| Operation | Role |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PLANDING` | Validate indirect cross-ISA targets when enabled. |
| `PTRAPRET` | Resume after a precise Poly trap. |

Current Bochs/test encodings are temporary. Constants live in
`tools/include/polycpuid.h`.

Deeper rationale lives in `docs/poly-isa-design-directions.md`.
