# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system ISA.
x86_64 remains the system ISA: boot, privilege, paging, interrupts, syscalls,
atomics, virtual memory, and the memory-ordering contract are still x86-defined.

## Frontends

| ID | Frontend | Fetch model |
| -- | -------- | ----------- |
| `0` | x86_64 | normal x86_64 variable-length decode |
| `1` | AArch64 | direct 32-bit fetch from `RIP` |
| `2` | RISC-V64 | direct RV64/RVC fetch from `RIP` |
| `3..255` | reserved | invalid |

Foreign instructions are not `#UD` envelopes. They execute through real decode
frontends sharing the same virtual address space, TLB, permissions, precise
fault model, and x86 TSO memory contract.

## Architectural Contract

- Extra foreign register state is explicit Poly XSAVE-style state.
- Hidden emulator banks keyed by CR3/TLS are not architectural.
- Native ABIs stay native: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Fast register-only calls use cached ABI signature slots for register aliasing.
- Stack arguments, aggregates, variadics, lazy binding, and incompatible vectors
  are handled by loader/runtime thunks.
- Foreign traps produce OS-neutral trap records; hardware does not emulate libc
  or Linux syscall policy.

## Current Prototype

- x86_64 controls: `0f 3a fc <subop>`.
- AArch64 controls: reserved `HINT` subspace.
- RISC-V64 controls: `custom-0` subspace.
- Poly XSAVE component: `20`.

Full rationale: `docs/poly-isa-design-directions.md`
Shared constants: `tools/include/polycpuid.h`
