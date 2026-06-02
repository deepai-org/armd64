# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system ISA in
one virtual address space. It targets existing native ABI objects and shared
libraries, not a new Poly-only compiler ABI.

See `README.md` for commands and `docs/poly-isa-design-directions.md` for
hardware/ABI rationale.

## What Differs From x86_64

- x86_64 owns boot, privilege, paging, faults, interrupts, atomics, syscalls,
  VM control, and global TSO ordering.
- AArch64 uses aligned 32-bit user-mode fetch; RISC-V64 uses 16/32-bit
  user-mode fetch including RVC.
- Frontend switches are decoded control instructions, not `#UD` envelopes.
- Cross-ISA calls target x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Hardware handles frontend switching, register-only ABI signature slots,
  per-thread XSAVE-style foreign state, and OS-neutral trap packets.
- Software handles stack arguments, aggregates, variadics, syscall policy, lazy
  binding, libc/libgcc/libatomic helpers, and other user-memory reshaping.

## Prototype Controls

Temporary Bochs encodings model dedicated silicon controls.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops: `0x03` `PENTER`, `0x04` `PSWITCH`, `0x05` `PLANDING`,
`0x2d` `PCALL`, `0x30..0x3c` `PCALL_SLOT`, `0x62` `PTRAPRET`,
`0x65..0x6e` setup/query controls.

Control targets must be canonical and valid for the target frontend's fetch
alignment. Monitor packet addresses must be canonical and qword-aligned.
