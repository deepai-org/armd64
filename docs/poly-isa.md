# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine. The
goal is to run existing precompiled code from all three ISAs in one process and
virtual address space, using hardware-style mode switches rather than
per-instruction traps.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes

- x86_64 remains the system ISA: boot, paging, privilege, interrupts, atomics,
  and memory ordering stay x86_64-defined.
- AArch64 and RISC-V64 execute as user-mode frontends over the same address
  space.
- Foreign fetch is direct: AArch64 uses aligned 32-bit instructions; RISC-V64
  uses 16/32-bit instructions with RVC.
- Extra foreign registers are per-thread XSAVE-style state, not CR3-global
  emulator state.
- Syscalls, libcalls, stack repacking, lazy binding, and loader policy are
  runtime/loader work, not hardware-parsed call descriptors.

## ISA Controls

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- `PENTER frontend`: enter another frontend.
- `PSWITCH frontend, target`: cross-ISA tail branch.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PLANDING`: validate an indirect cross-ISA landing target.
- `PTRAPRET`: resume from a Poly trap packet.

## ABI

- Native ABIs are x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Register-only calls use ABI signature slots for hardware-style register alias
  remapping.
- Complex calls use runtime thunks: stack arguments, variadics, aggregate
  repacking, vector mismatches, and lazy binding.
- Cross-ISA calls return through ordinary native returns.

## References

- Prototype constants and encodings: `tools/include/polycpuid.h`
- Design rationale and future hardware direction: `docs/poly-isa-design-directions.md`
