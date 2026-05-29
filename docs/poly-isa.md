# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 CPU. x86_64
stays the system ISA for boot, paging, privilege, interrupts, atomics, and the
global TSO memory model.

## Frontends

| ID | Frontend | Fetch model |
| -- | -------- | ----------- |
| `0` | x86_64 | normal variable-length x86 decode |
| `1` | AArch64 | direct 32-bit fetch from `RIP` |
| `2` | RISC-V64 | direct RV64/RVC fetch from `RIP` |

## What Changes From x86_64

- Foreign instructions are fetched directly; there are no per-instruction
  `#UD` envelopes or legacy single-instruction wrappers.
- All frontends share one virtual address space, TLB, page-fault path,
  permission model, and x86-style TSO ordering.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI. Poly is not a new compiler-only ABI.
- Register-only calls use cached ABI signature slots. Stack arguments,
  aggregates, variadics, lazy binding, and other ABI reshaping stay in
  loader/runtime thunks.
- Trap packets are OS-neutral. Foreign `svc`/`ecall`, illegal instructions,
  unresolved imports, and breakpoints report the source frontend, PC, status,
  and the first eight native foreign ABI argument registers.
- Foreign register state is explicit XSAVE-style architectural state. Hidden
  CR3/TLS-keyed emulator banks are not the hardware contract.

## Control Surface

- Generic operations: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`.
- x86_64 prototype controls: decoded Poly control opcode page.
- AArch64 prototype controls: reserved `HINT` subspace.
- RISC-V64 prototype controls: `custom-0` subspace.
- Poly XSAVE component: `20`.

## Pointers

- Run instructions and current prototype status: `README.md`.
- Design rationale and hardware direction: `docs/poly-isa-design-directions.md`.
- Shared constants: `tools/include/polycpuid.h`.
