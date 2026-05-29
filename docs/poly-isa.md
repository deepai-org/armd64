# Poly ISA Quick Reference

Poly keeps x86_64 as the system ISA and adds AArch64/RISC-V64 user-mode
frontends for existing precompiled objects and cross-ISA shared libraries.

## Run

```sh
make image
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|NATIVE_CHECK|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

Short tests: `make boot`, `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`.

## Difference From x86_64

- x86_64 owns boot, paging, privilege, interrupts, faults, syscalls, and TSO.
- Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign code fetches raw instructions from `RIP`: AArch64 aligned 32-bit,
  RISC-V64 16/32-bit. There are no per-instruction x86 `#UD` envelopes.
- All frontends share the x86_64 virtual address space, TLBs, permissions, and
  precise fault behavior.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AAPCS64, and RISC-V
  psABI. Poly is not a new compiler-only ABI.
- Hardware-facing transitions are register-only; software thunks handle stack
  arguments, aggregates, variadics, dynamic linking, and incompatible vectors.
- Non-x86 architectural state is explicit XSAVE-style Poly state, not hidden
  CR3-scoped emulator state.

## Prototype Encodings

- x86_64 controls: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT`
- RISC-V64 controls: `custom-0`
- Poly XSAVE prototype component: `20`

Design rationale: `docs/poly-isa-design-directions.md`.
Constants: `tools/include/polycpuid.h`.
