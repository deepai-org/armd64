# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine. The goal is compatibility with existing SysV x86_64, AAPCS64, and RISC-V psABI code, not a new compiler-only ABI.

Architectural contract:

- x86_64 owns boot, privilege, paging, interrupts, syscalls, VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 execute as user-mode frontends over the same virtual address space.
- Mode switches are decoded control instructions, not per-instruction `#UD` envelopes.
- Foreign register state is per-thread XSAVE-style architectural state.
- Recoverable foreign traps are delivered as OS-neutral user-monitor packets.
- Hardware handles frontend switching, native return cookies, trap packet delivery, state save/restore, and optional register-only ABI signature slots.
- Software handles syscall policy, libcalls, stack/aggregate marshalling, dynamic linking, and any ABI work that needs memory access.

Temporary Bochs encodings:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Core subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`, `PCALL=0x2d`, `PCALL_SLOT=0x30..0x3c`, `PTRAPRET=0x62`, setup/query `0x65..0x6e`.

Run commands are in `README.md`; hardware and ABI rationale is in `docs/poly-isa-design-directions.md`.
