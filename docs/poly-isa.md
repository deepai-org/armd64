# Poly ISA

Poly keeps x86_64 as the system ISA and adds raw AArch64 and RISC-V64
user-mode frontends in the same virtual address space. The goal is existing
native ABI code and shared libraries, not a new compiler-only ABI.

## Run It

```sh
make image
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|NATIVE_CHECK|Kernel panic|Segmentation fault|BUG:" \
  out/serial.log out/bochs*.log
```

Other useful targets: `make boot`, `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`.

## ISA Delta

- Frontends are `0=x86_64`, `1=AArch64`, and `2=RISC-V64`.
- x86_64 owns boot, paging, privilege, faults, interrupts, OS entry, and TSO.
- Foreign code shares x86_64 virtual memory and page permissions.
- AArch64 fetches direct 4-byte instructions from `RIP`; RISC-V64 fetches
  direct 2-byte or 4-byte instructions from `RIP`.
- `PENTER`, `PSWITCH`, `PCALL`, and `PIRET` switch frontends as decoded
  control-flow operations, not `#UD` traps.
- Prototype controls are x86 `0f 3a fc <subop>`, AArch64 reserved `HINT`, and
  RISC-V `custom-0`.
- Non-x86 state is explicit XSAVE-style architectural state, component `20`.

## ABI Boundary

- Target ABIs are SysV x86_64, AAPCS64, and RISC-V psABI.
- Register-only calls can use cached ABI signature slots for integer/FP remap.
- Software thunks handle stack args, aggregates, variadics, PLT/GOT, lazy
  binding, and incompatible vector layouts.
- Hardware provides frontend switches, explicit state, trap packets, landing
  policy, and register-signature slots. It does not know Linux, libc, ELF
  imports, symbols, or stack object layouts.

Detailed design notes: `docs/poly-isa-design-directions.md`

Constants: `tools/include/polycpuid.h`
