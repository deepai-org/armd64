# Poly ISA

Poly lets existing x86_64, AArch64, and RISC-V64 user-mode code share one
x86_64 virtual address space. It targets real ABI compatibility: x86_64 SysV,
AArch64 AAPCS64, and RISC-V psABI.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, syscalls, VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode decode frontends.
- AArch64 fetches aligned 32-bit instructions.
- RISC-V64 fetches 16/32-bit instructions, including RVC.
- Mode changes use decoded control instructions, not `#UD` envelopes.
- Foreign architectural state is per-thread XSAVE-style state.
- Recoverable foreign traps produce OS-neutral trap packets.

## Interop

- Fast calls use register-only ABI signature slots where possible.
- Stack arguments, aggregates, variadics, lazy binding, libc policy, syscall
  policy, and other memory-shaped ABI work stay in software thunks or monitors.
- The ISA supplies fast frontend switching and register-state mechanics; it does
  not parse user-memory call descriptors or rewrite stack layouts in hardware.

## Controls

Prototype Bochs encodings stand in for dedicated silicon opcodes:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Core subops: `PENTER` `0x03`, `PSWITCH` `0x04`, `PLANDING` `0x05`,
`PCALL` `0x2d`, `PCALL_SLOT` `0x30..0x3c`, `PTRAPRET` `0x62`.
Setup/query subops live in `0x65..0x6e`.

Control-flow targets must be canonical and aligned for the target frontend.
Monitor packet addresses must be canonical and qword-aligned.

See `README.md` for run commands and `docs/poly-isa-design-directions.md` for
the longer hardware/ABI rationale.
