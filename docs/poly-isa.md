# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system ISA.
The target is existing precompiled code and cross-ISA library linking, not a
new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
```

Use `make boot-poly-exec-cross-arch-traps` for a faster loader smoke test.

## Contract

- x86_64 remains the system ISA for boot, kernel entry, paging, interrupts,
  faults, VM control, atomics, and global TSO ordering.
- Foreign modes are raw 32-bit instruction frontends. They do not use
  per-instruction `#UD` envelopes.
- Cross-ISA calls preserve native ABI compatibility. Register-only calls may
  use hardware ABI signature slots; stack/aggregate/variadic cases use runtime
  thunks.
- Non-aliased foreign registers are per-thread XSAVE-style architectural state,
  never CR3-scoped hidden emulator state.
- Foreign traps produce precise OS-neutral trap records that runtime or OS
  policy can handle.

## ISA Operations

- `PENTER frontend`: enter a foreign frontend from trusted x86 code.
- `PSWITCH frontend, target`: non-returning cross-ISA branch.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PLANDING`: validate an indirect Poly landing target.
- `PTRAPRET`: resume after a precise Poly trap.

Temporary Bochs encodings are in `tools/include/polycpuid.h`. Design rationale
is in `docs/poly-isa-design-directions.md`.
