# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system CPU.
All code runs in the same x86_64 virtual address space; x86_64 remains the
system ISA.

## Contract

- x86_64 owns boot, privilege, paging, interrupts, faults, syscalls, atomics,
  and TSO memory ordering.
- AArch64 and RISC-V64 are direct-fetch user frontends, not `#UD` envelopes.
- Frontend ids are x86_64=`0`, AArch64=`1`, RISC-V64=`2`; `3..255` reserved.
- AArch64 fetches aligned 32-bit instructions from `RIP`.
- RISC-V64 fetches standard compressed and uncompressed instructions from `RIP`.
- All frontends share x86_64 virtual memory, TLBs, permissions, and precise
  fault behavior.
- Non-x86 architectural state is explicit Poly XSAVE-style state, not hidden
  CR3-scoped emulator state.

## Interop

- Calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Register-only calls use cached ABI signature slots to alias exchange
  registers.
- Stack arguments, aggregates, variadics, lazy binding, and incompatible vector
  layouts are handled by loader/runtime thunks.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults produce
  OS-neutral trap records for the runtime or OS policy layer.

## Prototype Encodings

- x86_64 controls: `0f 3a fc <subop>`.
- AArch64 controls: reserved `HINT` subspace.
- RISC-V64 controls: `custom-0` subspace.
- Poly XSAVE component: `20`.

Full design rationale: `docs/poly-isa-design-directions.md`.
Shared constants: `tools/include/polycpuid.h`.
