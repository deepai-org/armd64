# Poly ISA Quick Reference

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends in the same
process address space. The goal is compatibility with existing precompiled
code, not a new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
```

Fast loader smoke test:

```sh
make boot-poly-exec-cross-arch-traps
```

## Contract

- x86_64 remains the system ISA for boot, kernel entry, paging, interrupts,
  faults, VM control, atomics, and global TSO ordering.
- AArch64/RISC-V64 execute as direct native 32-bit fetch/decode frontends, not
  per-instruction `#UD` envelopes.
- Cross-ISA calls target real native ABIs. Register-only cases may use hardware
  ABI signature slots; stack arguments, aggregates, variadics, lazy binding,
  and helper calls stay in software thunks or the user runtime.
- Poly architectural state is per-thread XSAVE-style state. It is not hidden
  emulator state and is not keyed only by CR3.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults produce
  precise OS-neutral trap records for runtime or OS policy.

## Operations

- `PENTER frontend`: trusted entry into a frontend.
- `PSWITCH frontend, target`: non-returning cross-frontend branch.
- `PCALL frontend, target, sig`: cross-frontend call using ABI signature slot
  `sig`.
- `PLANDING`: optional indirect target validation.
- `PTRAPRET`: resume after a precise Poly trap.

Temporary Bochs encodings live in `tools/include/polycpuid.h`. Detailed design
rationale lives in `docs/poly-isa-design-directions.md`.
