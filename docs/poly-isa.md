# Poly ISA

Poly keeps x86_64 as the system ISA and adds user-mode AArch64 and RISC-V64
frontends in the same virtual address space.

The goal is compatibility with existing precompiled code and cross-ISA shared
libraries, not a new compiler-only ABI.

## Run It

```sh
make image
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|NATIVE_CHECK|Kernel panic|Segmentation fault|BUG:" \
  out/serial.log out/bochs*.log
```

Useful shorter targets: `make boot`, `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`.

## What Changes From x86_64

- x86_64 still owns boot, paging, privilege, interrupts, faults, syscalls, and
  TSO memory ordering.
- Frontend IDs are `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- AArch64 fetches aligned 4-byte instructions from `RIP`.
- RISC-V64 fetches 2-byte or 4-byte instructions from `RIP`.
- All frontends share x86_64 virtual addresses, TLBs, page permissions, and
  faults.
- `PENTER`, `PSWITCH`, `PCALL`, and `PIRET` are decoded control-flow operations,
  not `#UD` envelopes.
- Non-x86 state is explicit XSAVE-style architectural state, currently component
  `20` in the prototype.
- Hardware accelerates register-only handoff. Software thunks handle stack args,
  aggregates, variadics, PLT/GOT, lazy binding, and incompatible vectors.

## Prototype Encodings

- x86_64 controls: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT`
- RISC-V64 controls: `custom-0`

## References

- Design rationale: `docs/poly-isa-design-directions.md`
- Public constants: `tools/include/polycpuid.h`
