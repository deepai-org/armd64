# Poly ISA Quick Reference

Poly is an x86_64 extension that can execute raw AArch64 and RISC-V64
user-mode code in the same virtual address space. x86_64 remains the system ISA
for boot, privilege, paging, interrupts, faults, atomics, and memory ordering.

## Running It

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- Adds two 32-bit fixed-width user frontends: AArch64 and RISC-V64.
- Uses decoded Poly control ops for mode changes; there are no `#UD` envelopes.
- Keeps x86_64 paging, permissions, faults, traps, atomics, stack memory, and TSO.
- Targets existing SysV x86_64, AAPCS64, and RISC-V psABI binaries/libraries.

## Control

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- `PENTER frontend`: enter a foreign frontend.
- `PSWITCH frontend,target`: switch frontend and branch.
- `PCALL frontend,target,sig`: switch frontend, branch, save return state, and apply register-only ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.

## Hardware Boundary

Hardware handles frontend switching, native-return recovery, XSAVE-style state,
trap packets, and register-only ABI remapping.

Software handles stack arguments, aggregates, variadics, lazy binding, vector
layout mismatches, syscall policy, and other memory-side ABI work. Prototype
encodings are isolated to the x86_64 Poly control page, AArch64 reserved
`HINT`, RISC-V `custom-0`, and XSAVE component `20`.

Detailed rationale: `docs/poly-isa-design-directions.md`.
