# Poly ISA Quick Reference

Poly extends an x86_64 machine with AArch64 and RISC-V64 user-mode frontends in the same virtual address space.

Full rationale: `docs/poly-isa-design-directions.md`. Shared constants: `tools/include/polycpuid.h`.

## Run

```sh
make image
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## Contract

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, and faults stay x86_64-defined.
- Frontends are `0=x86_64`, `1=AArch64`, and `2=RISC-V64`.
- `PENTER`, `PSWITCH`, `PCALL`, and `PIRET` change fetch/decode frontend without using exceptions.
- Foreign instructions are fetched directly from `RIP`; AArch64 is 4-byte aligned, RISC-V is 2-byte aligned.
- Prototype encodings are x86 `0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V `custom-0`.
- Compatibility targets existing SysV x86_64, AAPCS64, and RISC-V psABI objects.
- Fast calls use cached register-signature slots for register-only ABI mapping.
- Software thunks handle stack arguments, aggregates, variadics, PLT/GOT, lazy binding, and incompatible vector layouts.
- Extra frontend state is explicit XSAVE-style state: component `20`, layout version `8`, size `4096`.
- The ISA is OS-neutral: hardware does not emulate Linux, libc, imports, or stack layouts.
