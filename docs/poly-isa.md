# Poly ISA

Poly is an x86_64 system ISA extension that adds user-mode AArch64 and RISC-V64
frontends. All modes share one x86_64 virtual address space; x86_64 still owns
boot, privilege, paging, interrupts, syscalls, atomics, and the memory model.

## Modes

- x86_64: frontend `0`, system/privileged ISA.
- AArch64: frontend `1`, direct 32-bit fetch from `RIP`.
- RISC-V64: frontend `2`, direct RVC/RV64 fetch from `RIP`.
- `3..255`: reserved.

Foreign modes are not `#UD` instruction envelopes. They are real decode modes
with shared TLBs, permissions, precise faults, and x86 TSO ordering.

## State

- Common address, stack, and fault model are x86_64-defined.
- Extra AArch64/RISC-V architectural state is explicit XSAVE-style Poly state.
- Hidden CR3-scoped emulator register banks are not part of the ISA contract.

## Interop

- Native ABIs are preserved: x86_64 SysV, AArch64 AAPCS64, RISC-V psABI.
- Register-only calls may use cached ABI signature slots for register aliasing.
- Stack args, aggregates, variadics, lazy binding, and incompatible vector
  layouts stay in loader/runtime thunks.
- Foreign traps (`svc`, `ecall`, breakpoints, illegal instructions, faults)
  produce OS-neutral trap records for the runtime or OS policy layer.

## Prototype Encodings

- x86_64 controls: `0f 3a fc <subop>`.
- AArch64 controls: reserved `HINT` subspace.
- RISC-V64 controls: `custom-0` subspace.
- Poly XSAVE component: `20`.

Design rationale: `docs/poly-isa-design-directions.md`
Shared constants: `tools/include/polycpuid.h`
