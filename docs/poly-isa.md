# Poly ISA

Short reference for running precompiled x86_64, AArch64, and RISC-V64 userspace in one x86_64 virtual address space.

Design rationale: `docs/poly-isa-design-directions.md`. Constants: `tools/include/polycpuid.h`.

## Run

```sh
make image
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## x86_64 Delta

- x86_64 owns boot, privilege, paging, interrupts, and faults.
- Frontend IDs are `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- `PENTER`, `PSWITCH`, `PCALL`, and `PIRET` redirect fetch/decode between frontends.
- Foreign code is raw direct-fetch code at `RIP`; no per-instruction `#UD` envelopes.
- AArch64 fetch is 4-byte aligned; RISC-V fetch is 2-byte aligned.
- Prototype control encodings are x86 `0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V `custom-0`.
- ABI compatibility targets native SysV x86_64, AAPCS64, and RISC-V psABI objects.
- Fast cross-ISA calls use cached register-signature slots; thunks handle stack args, aggregates, variadics, PLT/GOT, lazy binding, and incompatible vectors.
- Extra frontend state is explicit XSAVE-style state: component `20`, layout version `8`, size `4096`.
- The ISA is OS-neutral: hardware does not emulate Linux, libc, imports, or stack layouts.
