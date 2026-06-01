# Poly ISA Quick Reference

Poly is an x86_64 extension prototype for running precompiled x86_64,
AArch64, and RISC-V64 code in one process and one virtual address space.
This file is intentionally short. Design rationale lives in
`docs/poly-isa-design-directions.md`.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- x86_64 is still the system ISA: boot, privilege, paging, interrupts,
  exceptions, atomics, and TSO ordering stay x86_64-defined.
- AArch64 and RISC-V64 are user-mode frontends sharing the x86_64 address
  space.
- Foreign modes use direct instruction fetch, not one `#UD` envelope per
  instruction.
- Extra foreign registers are per-thread XSAVE-style architectural state.
- Hardware switches frontends and aliases register arguments; software handles
  syscalls, libcalls, loader policy, lazy binding, and stack/aggregate ABI
  repacking.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Control | Meaning |
| --- | --- |
| `PENTER frontend` | Enter another frontend. |
| `PSWITCH frontend, target` | Cross-ISA tail branch. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PLANDING` | Validate an indirect cross-ISA landing target. |
| `PTRAPRET` | Resume from a Poly trap packet. |

## ABI Model

- Native ABIs stay native: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Register-only calls may use hardware-style ABI signature slots.
- Stack arguments, variadics, aggregate repacking, vector mismatches, and lazy
  binding use runtime/loader thunks.
- Cross-ISA calls return through ordinary native returns.
