# Poly ISA Quick Reference

Poly keeps x86_64 as the system ISA and adds AArch64 and RISC-V64 user-mode frontends in the same virtual address space.

Design rationale lives in `docs/poly-isa-design-directions.md`; opcode and CPUID constants live in `tools/include/polycpuid.h`.

## Run

```sh
make image
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## x86_64 Differences

- x86_64 remains the system ISA for boot, privilege, paging, interrupts, and faults.
- User-mode frontends are `0=x86_64`, `1=AArch64`, and `2=RISC-V64`.
- `PENTER`, `PSWITCH`, `PCALL`, and `PIRET` switch fetch/decode frontends without an exception path.
- Foreign instructions fetch from `RIP`: AArch64 is 4-byte aligned; RISC-V64 is 2-byte aligned.
- Prototype controls use x86 `0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V `custom-0`.
- Non-x86 frontend state is explicit XSAVE-style state: component `20`, layout version `8`, size `4096`.
- Hardware stays OS-neutral: no Linux, libc, import, or stack-layout emulation.

## Compatibility

- Targets existing SysV x86_64, AAPCS64, and RISC-V psABI objects.
- Fast calls use cached register-signature slots for register-only ABI mapping.
- Software thunks handle stack arguments, aggregates, variadics, PLT/GOT, lazy binding, and incompatible vector layouts.
