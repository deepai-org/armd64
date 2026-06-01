# Poly ISA

Poly is an x86_64 extension for running existing x86_64, AArch64, and RISC-V64
userspace code in one process address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
```

For a shorter cross-ISA loader smoke test:

```sh
make boot-poly-exec-cross-arch-traps
```

## How It Differs From x86_64

- x86_64 remains the system ISA for boot, kernel entry, paging, faults,
  interrupts, VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode decode frontends over the same virtual
  memory, stack memory, page permissions, and thread model.
- Foreign instructions are fetched directly by their native frontend. They are
  not wrapped in per-instruction `#UD` envelopes.
- Cross-ISA calls target real native ABIs, not a new compiler-only ABI.
- Fast register-only ABI cases can use hardware signature slots. Stack
  arguments, aggregates, variadics, lazy binding, syscall policy, and helper
  calls remain runtime/software work.
- Foreign register and Poly control state are per-thread XSAVE-style
  architectural state, not hidden CR3-scoped emulator state.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults produce
  precise OS-neutral trap records for runtime or OS policy.

## Operations

- `PENTER frontend`: enter a frontend from trusted runtime/system code.
- `PSWITCH frontend, target`: branch to another frontend without return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature slot
  `sig`.
- `PLANDING`: validate indirect cross-ISA targets when landing-pad policy is
  enabled.
- `PTRAPRET`: resume after a precise Poly trap.

Temporary Bochs/test encodings are defined in `tools/include/polycpuid.h`.
Long-form design rationale lives in `docs/poly-isa-design-directions.md`.
