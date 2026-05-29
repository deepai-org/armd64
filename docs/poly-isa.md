# Poly ISA Quick Reference

Poly lets an x86_64 system CPU directly fetch AArch64 and RISC-V64 userspace
code in the same x86_64 virtual address space.

## What Changes From x86_64

- x86_64 still owns boot, privilege, paging, interrupts, faults, syscalls,
  virtual memory, atomics, and TSO ordering.
- AArch64 and RISC-V64 are userspace frontends fetched directly from `RIP`, not
  per-instruction `#UD` envelopes.
- Frontend ids are x86_64=`0`, AArch64=`1`, RISC-V64=`2`; `3..255` reserved.
- AArch64 fetches aligned 32-bit words; RISC-V64 fetches standard 16/32-bit
  instructions.
- All frontends share x86_64 virtual memory, TLBs, permissions, and precise
  fault behavior.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast register-only calls use ABI signature slots and exchange registers.
  Stack args, aggregates, variadics, loader policy, and incompatible vectors use
  software thunks.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults emit
  OS-neutral trap records.
- Non-x86 state is explicit XSAVE-style Poly state, not hidden CR3 state.

## Prototype Encodings

- x86_64 controls: `0f 3a fc <subop>`.
- AArch64 controls: reserved `HINT` subspace.
- RISC-V64 controls: `custom-0` subspace.
- Poly XSAVE component: `20`.

Design rationale: `docs/poly-isa-design-directions.md`.
ABI constants: `tools/include/polycpuid.h`.
