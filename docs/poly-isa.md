# Poly ISA

Poly lets existing x86_64, AArch64, and RISC-V64 user-mode code execute in one
x86_64 virtual address space. The target is real ABI compatibility with SysV
x86_64, AAPCS64, and the RISC-V psABI, not a new compiler-only ABI.

## Architectural Contract

- x86_64 is the system ISA: boot, privilege, paging, faults, interrupts,
  syscalls, VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode decode frontends over the same address
  space and memory subsystem. AArch64 fetches aligned 32-bit instructions;
  RISC-V64 fetches 16/32-bit instructions, including RVC.
- Mode changes are decoded control instructions, not `#UD` exception envelopes.
- Foreign architectural state is per-thread XSAVE-style state; recoverable
  foreign traps produce OS-neutral trap packets.

## Hardware Boundary

- Hardware supplies fast frontend switching, per-thread state, trap packets, and
  optional register-only ABI signature slots.
- Hardware does not parse user-memory call descriptors, rewrite stack layouts,
  marshal structs, implement libcalls, or translate OS syscall policy.
- Software thunks/monitors handle memory-shaped ABI work: stack arguments,
  aggregates, variadics, lazy binding, libc policy, and syscall policy.

## Temporary Encodings

These Bochs prototype encodings stand in for future dedicated silicon opcodes:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Core subops are `PENTER` `0x03`, `PSWITCH` `0x04`, `PLANDING` `0x05`,
`PCALL` `0x2d`, `PCALL_SLOT` `0x30..0x3c`, and `PTRAPRET` `0x62`.
Setup/query subops use `0x65..0x6e`. Control-flow targets must be canonical
and frontend-aligned. Monitor packet addresses must be canonical and
qword-aligned.

Run commands are in `README.md`. Longer hardware and ABI rationale lives in
`docs/poly-isa-design-directions.md`.
