# Poly ISA

Poly runs existing x86_64, AArch64, and RISC-V64 user-mode code in one x86_64 virtual address space. The goal is real ABI compatibility with SysV x86_64, AAPCS64, and the RISC-V psABI, not a new compiler-only ABI.

## Contract

- x86_64 is the system ISA: boot, privilege, paging, interrupts, syscalls, VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are peer user-mode frontends over the same memory system. AArch64 fetches aligned 32-bit instructions; RISC-V64 fetches 16/32-bit instructions including RVC.
- Mode changes use decoded Poly control instructions, not per-instruction `#UD` envelopes.
- Foreign state is per-thread XSAVE-style architectural state. Recoverable foreign traps produce OS-neutral trap packets for a user-space monitor.

## Boundary

- Hardware provides frontend switching, per-thread state save/restore, trap packets, and optional register-only ABI signature slots.
- Hardware does not parse user-memory descriptors, rewrite stacks, marshal structs, implement libcalls, or define OS syscall policy.
- Software thunks and monitors handle stack arguments, aggregates, variadics, lazy binding, libc policy, and syscall policy.

## Prototype Encodings

Bochs uses temporary encodings that stand in for future dedicated silicon opcodes:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Core subops: `PENTER` `0x03`, `PSWITCH` `0x04`, `PLANDING` `0x05`, `PCALL` `0x2d`, `PCALL_SLOT` `0x30..0x3c`, `PTRAPRET` `0x62`. Setup/query subops use `0x65..0x6e`.

Run commands are in `README.md`; detailed hardware and ABI rationale is in
`docs/poly-isa-design-directions.md`.
