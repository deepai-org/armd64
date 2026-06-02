# Poly ISA

Poly lets existing x86_64, AArch64, and RISC-V64 user-mode code run in one
x86_64 virtual address space. It is compatibility-focused: the target ABIs are
x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI, not a new Poly-only ABI.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, syscalls, VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode decode frontends only.
- AArch64 fetch is aligned 32-bit; RISC-V64 fetch is 16/32-bit with RVC.
- Frontend changes use decoded control instructions, not `#UD` envelopes.
- Fast cross-ISA calls may use register-only ABI signature slots.
- Stack arguments, aggregates, variadics, lazy binding, libc/syscall policy,
  and other memory-shaped ABI work stay in software thunks or monitors.
- Foreign state is per-thread XSAVE-style architectural state.
- Recoverable foreign traps produce OS-neutral trap packets.

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
