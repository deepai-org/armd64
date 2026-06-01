# Poly ISA Quick Reference

Small reference only. Use `README.md` for running it and
`docs/poly-isa-design-directions.md` for rationale.

## Contract

- Run existing precompiled x86_64, AArch64, and RISC-V64 user code in one VA.
- Keep x86_64 as the system ISA: boot, privilege, paging, faults, interrupts,
  syscalls, atomics, VM control, and TSO ordering.
- Treat AArch64 and RISC-V64 as user-mode decode frontends.
- Use native ABIs at boundaries: x86_64 SysV, AAPCS64, and RISC-V psABI.
- Keep foreign state explicit, per-thread, and XSAVE-style.
- Hardware switches frontends and aliases register lanes; software handles
  stack args, aggregates, variadics, lazy binding, libc, and syscall policy.

## Control Encodings

Decoded controls replace `#UD` envelopes.

| ISA | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Key subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`,
`PCALL=0x2d`, `PCALL_SLOT=0x30..0x3c`, `PTRAPRET=0x62`, setup `0x65..0x6e`.
