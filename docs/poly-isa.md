# Poly ISA

Poly extends x86_64 so precompiled x86_64, AArch64, and RISC-V64 userspace code
can run in one process. x86_64 remains the system ISA; AArch64 and RISC-V64 are
additional user-mode frontends. Design rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Running

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.
- x86_64 owns boot, privilege, paging, interrupts, syscalls, atomics, and TSO.
- Foreign frontends fetch native aligned 32-bit instructions from `RIP`.
- All frontends share x86 virtual memory and page tables.
- Extra foreign registers are XSAVE-style architectural state, not hidden banks.
- Fast interop is frontend switching plus register remapping.
- Stack reshaping, aggregate marshalling, variadics, symbol policy, and libc
  policy are runtime work.

## Control Instructions

- `PENTER frontend`: trusted entry into a frontend.
- `PSWITCH frontend, target`: cross-frontend tail branch.
- `PCALL frontend, target, sig`: cross-frontend call using ABI signature `sig`.
- `PTRAPRET`: resume after a precise trap.
- `PLANDING`: validate an indirect cross-frontend target when enabled.

Prototype encodings live in `tools/include/polycpuid.h`; final hardware should
use real decoded opcodes, not `#UD` traps.

## Calls And Traps

- Target existing precompiled code, not a new source ABI.
- Register-only calls use cached ABI signatures; complex calls use thunks.
- Native return instructions stay valid; cross-ISA returns use a hardware
  transition stack plus reserved return cookies.
- Recoverable foreign syscalls, traps, unresolved imports, and unsupported
  instructions produce OS-neutral trap packets for a Ring 3 Poly monitor.

The kernel still owns hard page faults, interrupts, scheduling, signals, and
real syscalls issued by the monitor.

## Non-Goals

- Legacy single-instruction `#UD` envelopes.
- Hidden register banks keyed only by `CR3`.
- Hardware parsing of user-memory call descriptors.
- Hardware stack repacking, struct marshalling, libcalls, or Linux-specific
  syscall emulation.
