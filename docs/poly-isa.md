# Poly ISA

Poly adds AArch64 and RISC-V64 userspace frontends to an x86_64 system CPU.
x86_64 still owns boot, privilege, paging, interrupts, faults, syscalls,
virtual memory, and TSO ordering.

## Contract

- Frontends: x86_64=`0`, AArch64=`1`, RISC-V64=`2`; `3..255` are reserved.
- Foreign code is fetched from `RIP`: AArch64 as aligned 32-bit words, RISC-V64
  as 16/32-bit instructions.
- ISA switches use decoded control instructions, not per-instruction `#UD`
  traps or hidden emulator envelopes.
- All frontends share x86_64 virtual memory, TLBs, page permissions, and precise
  fault behavior.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast calls are branch-like and register-only using ABI signature slots.
  Stack arguments, aggregates, variadics, dynamic linking, and incompatible
  vectors remain software thunk work.
- Non-x86 registers are explicit XSAVE-style Poly architectural state, not
  hidden CR3-scoped state.

## Prototype Controls

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved `HINT` subspace
- RISC-V64: `custom-0` subspace
- Poly XSAVE component: `20`

Rationale: `docs/poly-isa-design-directions.md`; ABI constants:
`tools/include/polycpuid.h`.
