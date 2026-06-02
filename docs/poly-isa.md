# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode decode frontends to an x86_64 machine. The target is compatibility with existing SysV x86_64, AAPCS64, and RISC-V psABI code, not a new compiler-only ABI.

## Architecture

- x86_64 owns system behavior: boot, privilege, paging, interrupts, syscalls, VM control, atomics, and TSO ordering.
- AArch64/RISC-V64 are user frontends over the same virtual address space and switch via decoded control instructions, not `#UD` envelopes.
- Foreign register state is per-thread XSAVE-style state; recoverable foreign traps are OS-neutral monitor packets.
- Hardware handles frontend switch, return cookies, trap delivery, state save/restore, and optional register-only ABI signatures.
- Software handles syscall/libcall policy, dynamic linking, stack/aggregate marshalling, and anything requiring memory inspection.

## Bochs Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Core subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`, `PCALL=0x2d`, `PCALL_SLOT=0x30..0x3c`, `PTRAPRET=0x62`, setup/query `0x65..0x6e`.

Run commands: `README.md`. Hardware/ABI rationale: `docs/poly-isa-design-directions.md`.
