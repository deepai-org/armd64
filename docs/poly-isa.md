# Poly ISA Quick Reference

Poly lets an x86_64 CPU run existing precompiled AArch64 and RISC-V64 user code
in the same process. It is not a new compiler ABI.

## Contract

- x86_64 owns boot, privilege, paging, faults, interrupts, syscalls, atomics,
  VM control, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends fetching native instructions
  from the same virtual address space.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AAPCS64, RISC-V psABI.
- Foreign architectural state is explicit per-thread XSAVE-style state.
- Hardware switches frontends and aliases registers.
- Software handles stack arguments, aggregates, variadics, lazy binding, libc,
  and syscall policy.

## Encodings

Temporary controls are decoded instructions, not `#UD` exception envelopes:

| Frontend | Control encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Key subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`, `PCALL=0x2d`,
`PCALL_SLOT=0x30..0x3c`, `PTRAPRET=0x62`, state/control setup `0x65..0x6e`.

See `README.md` for running the prototype and
`docs/poly-isa-design-directions.md` for hardware/ABI rationale.
