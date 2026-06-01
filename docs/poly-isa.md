# Poly ISA

Poly is a hardware-oriented frontend extension for running existing x86_64,
AArch64, and RISC-V64 user code in one x86_64 process. It is not a new compiler
ABI. Rationale: `docs/poly-isa-design-directions.md`.

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, interrupts, kernel entry, atomics, VM
  control, and global TSO ordering.
- Foreign frontends are user-mode only and fetch native 32-bit instructions.
- Supported ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Extra foreign state is explicit per-thread XSAVE-style state.

## Controls

Temporary decoded controls, not `#UD` traps:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`
- Subops: `0x03` `PENTER`, `0x04` `PSWITCH`, `0x05` `PLANDING`,
  `0x2d` `PCALL`, `0x30..0x3c` `PCALL_SLOT 0..12`, `0x62` `PTRAPRET`,
  `0x65..0x6e` state/control setup.

## Hardware Boundary

- Hardware: fixed-latency switches, register-only ABI signatures, landing
  validation, return cookies, transition-stack recovery, and trap packets.
- Software: stack args, aggregates, variadics, lazy binding, helper libraries,
  and syscall/libc policy.
- Excluded from hardware: user-memory call descriptors, stack repacking, and
  OS/libc semantics.
