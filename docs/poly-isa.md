# Poly ISA Quick Reference

Poly lets one x86_64 process execute real AArch64 and RISC-V64 user code in
the same virtual address space. It is for native ABI compatibility, not a new
compiler-only ABI.

## Contract

- x86_64 owns privilege, paging, interrupts, faults, VM control, atomics, and
  global TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends that fetch native instructions
  from x86_64 virtual memory.
- Target ABIs are SysV x86_64, AAPCS64, and RISC-V psABI.
- Foreign architectural state is explicit per-thread XSAVE-style state.
- Hardware emits OS-neutral trap packets; it does not implement Linux, libc,
  linker, libgcc, libatomic, or stack-repacking policy.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Op | Purpose |
| --- | --- |
| `PENTER mode` | Enter a frontend. |
| `PSWITCH mode,target` | Branch to another frontend. |
| `PCALL mode,target,sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PTRAPRET` | Resume after a Poly trap. |
| `PLANDING` | Mark or validate indirect cross-ISA landing points. |

Prototype controls are decoded instructions, not `#UD` envelopes: x86_64
`0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V `custom-0`.

Run commands are in `README.md`. Hardware/ABI rationale is in
`docs/poly-isa-design-directions.md`.
