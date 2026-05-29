# Poly ISA

Poly keeps x86_64 as the system ISA and adds user-mode AArch64/RISC-V64
frontends for existing precompiled code and cross-ISA shared libraries.

## Run

```sh
make image
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|NATIVE_CHECK|Kernel panic|Segmentation fault|BUG:" \
  out/serial.log out/bochs*.log
```

Short smoke tests: `make boot`, `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`.

## x86_64 Differences

- x86_64 owns boot, paging, privilege, interrupts, faults, syscalls, and TSO.
- Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign code fetches from `RIP` in the same virtual address space.
- Fetch width is native: AArch64 aligned 32-bit, RISC-V64 16/32-bit.
- All frontends share x86_64 TLBs, page permissions, and fault behavior.
- `PENTER`, `PSWITCH`, `PCALL`, and `PIRET` are decoded control-flow operations,
  not `#UD` envelopes.
- Non-x86 architectural state is XSAVE-style state, prototype component `20`.
- Hardware handles register-only transitions; software thunks handle stack args,
  aggregates, variadics, PLT/GOT, lazy binding, and incompatible vectors.

## Prototype Encodings

- x86_64 controls: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT`
- RISC-V64 controls: `custom-0`

Design rationale lives in `docs/poly-isa-design-directions.md`; public constants
live in `tools/include/polycpuid.h`.
