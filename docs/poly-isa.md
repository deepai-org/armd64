# Poly ISA

Poly extends x86_64 so one process can run precompiled AArch64 and RISC-V64
user code in the same virtual address space. The goal is native object/library
compatibility, not a new compiler-only ABI.

## Model

- x86_64 owns privilege, paging, faults, interrupts, VM control, atomics, and
  the process memory model.
- AArch64 and RISC-V64 are user-mode frontends over the same machine.
- Target ABIs are SysV x86_64, AAPCS64, and RISC-V psABI.
- Foreign architectural state is explicit per-thread XSAVE-style state.
- Hardware switches frontends and emits OS-neutral trap packets.
- Software owns loading, ABI thunks, syscalls, libcalls, stack/aggregate layout,
  and OS-specific behavior.

## Controls

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

- `PENTER mode`: enter a frontend at the current PC.
- `PSWITCH mode,target`: branch to another frontend.
- `PCALL mode,target,sig`: call another frontend using ABI signature slot `sig`.
- `PTRAPRET`: return from a user-mode Poly trap monitor.
- `PLANDING`: validate indirect cross-ISA landing points.

Prototype encodings are decoded instructions, not `#UD` envelopes: x86_64
`0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V64 `custom-0`.

See `README.md` for commands and `docs/poly-isa-design-directions.md` for
rationale.
