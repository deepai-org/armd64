# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends so existing
native objects from all three ISAs can run in one process and address space.

## Contract

- x86_64 remains the system ISA for privilege, paging, faults, interrupts, VM
  control, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are peer user frontends that fetch native instructions
  from the same x86_64 virtual address space.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI. This is not a compiler-only `PolyFast` ABI.
- Hardware accelerates register-only calls with ABI signature slots that rename
  or alias registers. Software thunks handle stack arguments, aggregates,
  variadics, lazy binding, and incompatible vector layouts.
- Foreign state is explicit per-thread XSAVE-style state.
- Foreign syscalls and recoverable traps produce OS-neutral trap packets for a
  user runtime. Hardware does not implement OS, libc, linker, libgcc, or
  libatomic policy.

## Differences From x86_64

- The decoder can switch between x86_64, AArch64, and RISC-V64.
- AArch64 fetch is 4-byte aligned; RISC-V fetch is 2-byte aligned for RVC.
- Cross-ISA calls return through native returns using a hardware transition
  stack and reserved return cookies.
- Recoverable foreign exits produce precise trap packets, not hidden OS/libc
  emulation.

## Control Ops

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

Ops: `PENTER` enters a frontend, `PSWITCH` branches across frontends, `PCALL`
calls across frontends using an ABI signature slot, `PTRAPRET` resumes from a
trap monitor, and `PLANDING` validates indirect landing points when enabled.

Prototype encodings: x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`, and
RISC-V `custom-0`. They are decoded control instructions, not `#UD` envelopes.

See `docs/poly-isa-design-directions.md` for rationale and `README.md` for
build/test commands.
