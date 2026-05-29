# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 CPU. x86_64
remains the system ISA for boot, paging, privilege, interrupts, atomics, and
the global TSO memory model.

## Frontends

| ID | Frontend | Fetch |
| -- | -------- | ----- |
| `0` | x86_64 | normal variable-length x86 decode |
| `1` | AArch64 | direct 32-bit fetch from `RIP` |
| `2` | RISC-V64 | direct RV64/RVC fetch from `RIP` |

## x86_64 Differences

- Foreign code is fetched directly. There are no per-instruction `#UD`
  envelopes.
- All frontends share one virtual address space, TLB, page-fault path,
  permission model, and TSO memory model.
- Foreign register state is architectural XSAVE-style state.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Register-only calls use cached ABI signature slots. Complex ABI cases use
  loader/runtime thunks.
- Traps are OS-neutral packets, not Linux syscall or libc emulation.

## Controls

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved `HINT` space
- RISC-V64: `custom-0` space
- Poly XSAVE component: `20`
- `PENTER frontend`
- `PSWITCH frontend, target`
- `PCALL frontend, target, sig`
- `PTRAPRET`

Design rationale: `docs/poly-isa-design-directions.md`.
Constants: `tools/include/polycpuid.h`.
