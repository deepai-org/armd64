# Poly ISA

Poly is an x86_64 extension that can execute raw AArch64 and RISC-V64
user-mode code in one virtual address space. x86_64 remains the system ISA.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
make boot-poly-probe-arch-traps
make boot-poly-bench-arch-traps
```

## ISA Contract

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign frontends fetch raw 32-bit fixed-width instructions from the x86_64 address space.
- x86_64 still owns paging, faults, traps, atomics, stack memory, interrupts, privilege, and TSO.
- Mode changes use decoded Poly controls, not architectural `#UD` envelopes.
- The target is existing SysV x86_64, AAPCS64, and RISC-V psABI objects.

## Control

- `PENTER frontend`: enter a foreign frontend.
- `PSWITCH frontend,target`: switch frontend and branch.
- `PCALL frontend,target,sig`: switch frontend, branch, save return state, and apply register-only ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.

## ABI Boundary

Hardware handles frontend switching, native-return recovery, XSAVE-style foreign state, precise trap packets, and register-only ABI remapping.

Software handles stack arguments, aggregates, variadics, lazy binding, vector layout mismatches, syscall policy, and memory-side ABI work.

Prototype encodings are isolated to the x86_64 Poly control page, AArch64 reserved `HINT`, RISC-V `custom-0`, and XSAVE component `20`.

Design rationale: `docs/poly-isa-design-directions.md`.
