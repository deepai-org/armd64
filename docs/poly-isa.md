# Poly ISA

Poly extends x86_64 so one process can execute existing precompiled x86_64,
AArch64, and RISC-V64 code in a shared virtual address space.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## Differences From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode instruction frontends over the same
  process address space.
- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Fetch follows the active frontend: x86_64 variable-length, AArch64 32-bit
  aligned, RISC-V64 16-bit RVC plus 32-bit instructions.
- Cross-frontend control uses decoded Poly instructions: `PENTER`, `PSWITCH`,
  `PCALL`, `PTRAPRET`, and `PLANDING`; no `#UD` instruction envelopes.
- Non-x86 state is explicit per-thread XSAVE-style architectural state.
- Fast calls use hardware register-alias signature slots. Stack arguments,
  aggregates, variadics, and loader policy stay in software thunks.
- Foreign traps are delivered as OS-neutral packets to a user-mode Poly monitor.

## Hardware Boundary

- No Linux, libc, libgcc, libatomic, or dynamic-linker policy in hardware.
- No user-memory call descriptor parsing in control instructions.
- No stack repacking, aggregate marshalling, or variadic argument handling in
  hardware.

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
