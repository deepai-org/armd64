# Poly ISA

Poly is an x86_64 CPU extension for running existing precompiled x86_64,
AArch64, and RISC-V64 code in one user virtual address space.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## ISA Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode frontends that fetch real native
  instructions from the same address space.
- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Fetch width is frontend-specific: x86_64 is variable-width, AArch64 is
  32-bit aligned, and RISC-V64 supports 16-bit RVC plus 32-bit instructions.
- Non-x86 architectural state is explicit XSAVE-style per-thread state, not
  hidden emulator state.
- Cross-frontend controls are decoded instructions: `PENTER`, `PSWITCH`,
  `PCALL`, `PTRAPRET`, and `PLANDING`. They are not `#UD` envelopes.
- Fast calls use fixed register alias signature slots. Complex ABI cases use
  loader/runtime thunks.
- Foreign traps produce OS-neutral packets for a user-mode Poly monitor.

## What Hardware Does Not Do

- No Linux, libc, libgcc, libatomic, or dynamic-linker policy in hardware.
- No user-memory call descriptor parsing in control instructions.
- No stack repacking, aggregate marshalling, or variadic argument handling in
  hardware.

Detailed rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
