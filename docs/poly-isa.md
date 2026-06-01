# Poly ISA

Poly extends x86_64 so one process can run existing x86_64, AArch64, and
RISC-V64 native ABI code.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
```

Use `make boot-poly-exec-cross-arch-traps` for a shorter cross-ISA smoke test.

## Contract

- x86_64 remains the system ISA: boot, kernel entry, paging, faults,
  interrupts, atomics, VM control, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode decode frontends over the same process
  address space, stack memory, page permissions, and thread model.
- Poly transitions are decoded ISA operations, not `#UD` exception envelopes.
- Foreign registers and Poly control state are per-thread XSAVE-style state.
- Hardware handles frontend switching, register-only ABI signatures, precise
  trap packets, native return-cookie recovery, and XSAVE state.
- Runtime software handles stack arguments, aggregates, variadics, lazy
  binding, syscall policy, helper calls, and other memory-shaped ABI work.

## Operations

- `PENTER frontend`: enter a frontend from trusted runtime/system code.
- `PSWITCH frontend, target`: branch to another frontend without return.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PLANDING`: validate indirect cross-ISA targets when enabled.
- `PTRAPRET`: resume after a precise Poly trap.

Temporary Bochs/test encodings live in `tools/include/polycpuid.h`. Design
rationale lives in `docs/poly-isa-design-directions.md`.
