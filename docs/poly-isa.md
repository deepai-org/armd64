# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends in the same
process address space. The goal is compatibility with existing precompiled
code, not a new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
```

Faster loader smoke test:

```sh
make boot-poly-exec-cross-arch-traps
```

## How It Differs From x86_64

- x86_64 is still the system ISA: boot, kernel entry, paging, interrupts,
  faults, VM control, atomics, and global TSO ordering stay x86-owned.
- AArch64 and RISC-V64 are user-mode frontend modes. They fetch and decode raw
  32-bit native instructions directly; there are no per-instruction `#UD`
  envelopes.
- Cross-ISA calls target real native ABIs. Hardware can accelerate
  register-only calls through ABI signature slots; stack arguments, aggregates,
  variadics, lazy binding, and helper calls remain software/runtime work.
- Extra foreign architectural state is per-thread XSAVE-style state, not hidden
  emulator state and not CR3-only process state.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults produce
  precise OS-neutral trap records for runtime or OS policy.

## Operations

- `PENTER frontend`: trusted entry into a frontend.
- `PSWITCH frontend, target`: non-returning cross-frontend branch.
- `PCALL frontend, target, sig`: cross-frontend call using ABI signature slot
  `sig`.
- `PLANDING`: optional indirect target validation.
- `PTRAPRET`: resume after a precise Poly trap.

## More Detail

Temporary Bochs encodings live in `tools/include/polycpuid.h`. Detailed design
rationale lives in `docs/poly-isa-design-directions.md`.
