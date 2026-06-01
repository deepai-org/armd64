# Poly ISA

Quick reference for the prototype. See `README.md` for run commands and
`docs/poly-isa-design-directions.md` for hardware/ABI rationale.

## Contract

- Goal: run existing precompiled x86_64, AArch64, and RISC-V64 user code in one
  virtual address space.
- x86_64 remains the system ISA: boot, privilege, paging, faults, interrupts,
  syscalls, atomics, VM control, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode decode frontends, not new compiler ABIs.
- Cross-ISA calls target native ABIs: x86_64 SysV, AAPCS64, and RISC-V psABI.
- Foreign state is explicit per-thread XSAVE-style architectural state.
- Hardware switches frontends and aliases register lanes; software handles
  stack arguments, aggregates, variadics, lazy binding, libc, and syscall
  policy.

## Prototype Controls

Controls are decoded instructions, not `#UD` envelopes.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Important subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`,
`PCALL=0x2d`, `PCALL_SLOT=0x30..0x3c`, `PTRAPRET=0x62`, setup `0x65..0x6e`.
