# Poly ISA

Poly lets existing x86_64, AArch64, and RISC-V64 user-mode code share one x86_64 virtual address space. The target is native ABI compatibility with SysV x86_64, AAPCS64, and the RISC-V psABI.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts, syscalls, VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends over the same address space and memory subsystem.
- Mode changes use decoded Poly control instructions, never per-instruction `#UD` envelopes.
- Foreign registers are per-thread XSAVE-style architectural state; recoverable foreign traps produce OS-neutral user-monitor packets.

## Hardware Boundary

- Hardware handles frontend switching, state save/restore, trap packets, native return cookies, and optional register-only ABI signature slots.
- Hardware does not parse descriptors, rewrite stacks, marshal structs, implement libcalls, or define syscall policy. Software thunks and monitors own those policies.

## Prototype Encodings

Bochs uses temporary encodings as stand-ins for future dedicated opcodes:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Core subops: `PENTER` `0x03`, `PSWITCH` `0x04`, `PLANDING` `0x05`, `PCALL` `0x2d`, `PCALL_SLOT` `0x30..0x3c`, `PTRAPRET` `0x62`. Setup/query subops use `0x65..0x6e`.

Run commands are in `README.md`. Hardware and ABI rationale is in `docs/poly-isa-design-directions.md`.
