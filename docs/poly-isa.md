# Poly ISA

Poly adds raw AArch64 and RISC-V64 user-mode frontends to an x86_64 machine.
x86_64 remains the system ISA: boot, privilege, paging, interrupts, precise
faults, atomics, and the effective TSO memory model are still x86_64-owned.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Delta

- Foreign code fetches normal 32-bit AArch64 or RISC-V64 instructions from the same virtual address space.
- Foreign modes inherit x86_64 page permissions, traps, stack memory, atomics, and TSO ordering.
- Mode switches are decoded Poly control ops, not `#UD` traps or per-instruction envelopes.
- The target is existing precompiled SysV x86_64, AAPCS64, and RISC-V psABI code.
- Current explicit import/export state layout version: `3`.

## Control

- `PENTER frontend`: enter a foreign frontend.
- `PSWITCH frontend,target`: switch frontend and branch without return state.
- `PCALL frontend,target,sig`: switch frontend, branch, save return state, and apply cached register-only ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.

## Boundary

Hardware handles fixed-latency frontend switching, native-return recovery,
XSAVE-style state, trap packets, and register-only ABI signature remapping.
Software handles stack arguments, aggregates, variadics, lazy binding, vector
layout mismatches, syscall policy, and other memory-side ABI work.

Prototype encodings are isolated: x86_64 Poly control page, AArch64 reserved
`HINT`, RISC-V `custom-0`, XSAVE component `20`.

Detailed rationale: `docs/poly-isa-design-directions.md`.
