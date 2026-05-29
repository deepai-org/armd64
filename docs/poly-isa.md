# Poly ISA

Poly adds peer instruction frontends to an x86_64 system CPU. x86_64 still owns
boot, privilege, paging, interrupts, faults, syscalls, virtual memory, and TSO
ordering. AArch64 and RISC-V64 execute as raw userspace frontends in the same
address space.

## Contract

- Foreign code is fetched directly from `RIP`: AArch64 as aligned 32-bit words,
  RISC-V64 as 16/32-bit instructions. There are no per-instruction `#UD`
  envelopes.
- All frontends share the x86_64 virtual address space, TLBs, page permissions,
  and precise fault model.
- Cross-ISA calls target existing ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI. This is not a new compiler-only ABI.
- Fast hardware transitions are branch-like and register-only. Stack arguments,
  aggregates, variadics, dynamic linking, and incompatible vectors stay in
  loader/runtime thunks.
- Non-x86 registers are explicit XSAVE-style Poly architectural state, not
  hidden CR3-scoped emulator state.

## Frontend IDs

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |
| `3..255` | Reserved |

## Prototype Encodings

- x86_64 controls: decoded `0f 3a fc <subop>` Poly control page.
- AArch64 controls: reserved `HINT` subspace.
- RISC-V64 controls: `custom-0` subspace.
- Poly XSAVE prototype component: `20`.

See `docs/poly-isa-design-directions.md` for rationale and
`tools/include/polycpuid.h` for ABI-visible constants.
