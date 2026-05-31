# Poly ISA

Poly is an x86_64 extension for running precompiled AArch64 and RISC-V64 user
code in the same process and address space. It targets native ABI compatibility,
not a new compiler-only ABI.

## Contract

- x86_64 owns privilege, paging, faults, interrupts, VM control, atomics, and
  the process memory model.
- AArch64 and RISC-V64 are user-mode frontends over the same virtual machine.
- Target ABIs are SysV x86_64, AAPCS64, and RISC-V psABI.
- Non-x86 architectural state is explicit per-thread XSAVE-style state.
- Hardware only switches frontends, maps register ABI signatures, and reports
  OS-neutral trap packets.
- Software handles loading, ABI thunks, syscalls, libcalls, stack and aggregate
  layout, lazy binding, and OS-specific policy.

## Frontend Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER mode`: enter a frontend at the current PC.
- `PSWITCH mode,target`: branch to another frontend.
- `PCALL mode,target,sig`: cross-frontend call using ABI signature slot `sig`.
- `PTRAPRET`: return from a user-mode Poly trap monitor.
- `PLANDING`: validate an indirect cross-ISA landing point.

Prototype encodings are real decoded instructions, not `#UD` traps: x86_64
`0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V64 `custom-0`.

See `README.md` for how to run the prototype and
`docs/poly-isa-design-directions.md` for hardware rationale.
