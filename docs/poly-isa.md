# Poly ISA Quick Reference

Poly lets one x86_64 process run precompiled AArch64 and RISC-V64 user code in
the same virtual address space. The target is native ABI compatibility for real
objects and libraries, not a new compiler-only ABI.

## Model

- x86_64 owns privilege, paging, interrupts, faults, VM control, atomics, and TSO.
- AArch64 and RISC-V64 are ring-3 frontends over the same x86_64 address space.
- ABI targets are SysV x86_64, AAPCS64, and RISC-V psABI.
- Non-x86 architectural state is explicit per-thread XSAVE-style state.
- Hardware emits OS-neutral trap packets; software owns syscall, libc, linker,
  libgcc/libatomic, stack, and aggregate policy.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64

| Op | Purpose |
| --- | --- |
| `PENTER mode` | Enter a frontend at the current PC. |
| `PSWITCH mode,target` | Cross-ISA branch. |
| `PCALL mode,target,sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PTRAPRET` | Return from a Poly trap monitor. |
| `PLANDING` | Mark or validate indirect cross-ISA landing points. |

Prototype encodings are decoded instructions, not `#UD` envelopes: x86_64
`0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V64 `custom-0`.

Run commands are in `README.md`. Hardware/ABI rationale is in
`docs/poly-isa-design-directions.md`.
