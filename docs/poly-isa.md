# Poly ISA

Poly is an x86_64 extension that adds user-mode AArch64 and RISC-V64 frontends.
x86_64 remains the system ISA: boot, paging, interrupts, privilege transitions,
atomics, and the global TSO memory model all stay x86-owned.

| ID | Frontend | Fetch path |
| -- | -------- | ---------- |
| `0` | x86_64 | normal x86 variable-length decode |
| `1` | AArch64 | direct 32-bit fetch from `RIP` |
| `2` | RISC-V64 | direct RV64/RVC fetch from `RIP` |

## How It Differs From x86_64

- Foreign instructions are fetched directly; there are no per-instruction
  `#UD` envelopes.
- All frontends share one x86_64 address space, TLB, page-fault path,
  permission model, and TSO memory model.
- Foreign register state is architectural XSAVE-style state, not hidden
  emulator state.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Fast register-only crossings use cached ABI signature slots. Calls needing
  stack or aggregate reshaping use loader/runtime thunks.
- Poly traps are OS-neutral records; hardware does not implement Linux syscalls
  or libc policy.

## Control Surface

- x86_64 prototype controls: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT` subspace
- RISC-V64 controls: `custom-0` subspace
- Poly XSAVE component: `20`
- `PENTER frontend`: enter a frontend.
- `PSWITCH frontend, target`: switch frontend without a return edge.
- `PCALL frontend, target, sig`: cross-ISA call through a native return cookie
  and hardware transition stack.
- `PTRAPRET`: resume after a precise poly trap.

Design rationale: `docs/poly-isa-design-directions.md`.
Constants: `tools/include/polycpuid.h`.
