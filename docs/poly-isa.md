# Poly ISA

Poly keeps x86_64 as the system ISA and adds AArch64 and RISC-V64
user-mode frontends in the same virtual address space.

Detailed rationale: `docs/poly-isa-design-directions.md`

Constants and test contract: `tools/include/polycpuid.h`

## Run It

```sh
make image
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" \
  out/serial.log out/bochs*.log
```

## How It Differs From x86_64

- x86_64 still owns boot, paging, privilege, interrupts, faults, and OS entry.
- User frontends are `0=x86_64`, `1=AArch64`, and `2=RISC-V64`.
- `PENTER`, `PSWITCH`, `PCALL`, and `PIRET` switch the fetch/decode frontend.
- Frontend switches are normal control-flow operations, not `#UD` traps.
- AArch64 fetches 4-byte instructions from `RIP`.
- RISC-V64 fetches 2-byte or 4-byte instructions from `RIP`.
- Prototype x86 controls use `0f 3a fc <subop>`.
- Prototype AArch64 controls use reserved `HINT` encodings.
- Prototype RISC-V controls use `custom-0` encodings.
- Non-x86 architectural state is exposed as XSAVE-style state component `20`.

## Compatibility Model

- The goal is compatibility with existing SysV x86_64, AAPCS64, and RISC-V psABI objects.
- Register-only calls can use cached ABI signature slots for fast register remapping.
- Complex calls still use software thunks.
- Thunks handle stack arguments, aggregates, variadics, PLT/GOT, lazy binding, and incompatible vector layouts.

## Hardware Boundary

- Hardware provides frontend switching, explicit architectural state, trap packets, and register-signature slots.
- Hardware does not know Linux, libc, ELF imports, symbol binding, or stack object layouts.
- OS-neutral runtime code handles syscall/libcall policy in user space where possible.
