# Poly ISA

```sh
make image
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|NATIVE_CHECK|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

Short tests: `make boot`, `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`.

## Contract

- x86_64 remains the system ISA: boot, paging, privilege, interrupts, faults,
  syscalls, and TSO.
- Poly adds user-mode AArch64 and RISC-V64 frontends for existing precompiled
  objects and cross-ISA shared libraries.
- Frontend IDs are `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign code is direct fetched from `RIP`: AArch64 32-bit aligned words;
  RISC-V64 16/32-bit instructions. No per-instruction `#UD` envelopes.
- All frontends share the x86_64 virtual address space, TLBs, permissions, and
  precise fault behavior.
- Cross-ISA calls target real ABIs: x86_64 SysV, AAPCS64, and RISC-V psABI.
- Hardware-facing transitions are register-only. Software thunks handle stack
  arguments, aggregates, variadics, dynamic linking, and incompatible vectors.
- Non-x86 state is explicit XSAVE-style Poly state, not hidden emulator state.

## Encodings

- x86_64 controls: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT`
- RISC-V64 controls: `custom-0`
- Poly XSAVE prototype component: `20`

Long design notes: `docs/poly-isa-design-directions.md`
Constants: `tools/include/polycpuid.h`
