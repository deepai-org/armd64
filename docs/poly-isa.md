# Poly ISA

Poly extends an x86_64 CPU with AArch64 and RISC-V64 user-mode frontends for
running existing precompiled objects in one process. It is not a new compiler
target or a replacement ABI. Design rationale lives in
`docs/poly-isa-design-directions.md`.

## How It Differs From x86_64

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, kernel
  entry, VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode decode frontends. They fetch their native
  32-bit instruction streams from the same virtual address space.
- Cross-ISA calls preserve native ABI meaning: SysV x86_64, AAPCS64, and
  RISC-V psABI are compatibility targets.
- Extra foreign architectural state is explicit per-thread XSAVE-style state,
  not hidden CR3-scoped emulator state.

## Control Instructions

Temporary encodings are decoded controls, not `#UD` exception envelopes:

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Key subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`, `PCALL=0x2d`,
`PCALL_SLOT=0x30..0x3c`, `PTRAPRET=0x62`, state/control setup `0x65..0x6e`.

## Hardware Boundary

Hardware provides fixed-latency frontend switches, register-only ABI signature
remapping, landing validation, return cookies, transition-stack recovery, and
trap packets. Software handles stack arguments, aggregates, variadics, lazy
binding, helper libraries, and syscall/libc policy. Hardware must not parse
user-memory call descriptors, repack stacks, or embed OS/libc semantics.
