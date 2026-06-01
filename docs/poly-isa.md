# Poly ISA

Poly is an x86_64 CPU extension prototype that adds AArch64 and RISC-V64
user-mode frontends. The target is compatibility with existing precompiled
native code from all three ISAs in one process and one virtual address space.

This file is the short operator/spec reference. Longer design rationale lives
in `docs/poly-isa-design-directions.md`.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode instruction frontends over the same x86_64
  address space.
- Foreign instructions are fetched directly, not wrapped in per-instruction
  `#UD` envelopes.
- Non-x86 register state is per-thread XSAVE-style architectural state.
- Syscalls, libcalls, lazy binding, stack repacking, and loader policy stay in
  software runtimes, not in hardware.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Control | Purpose |
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

## References

- Prototype constants and encodings: `tools/include/polycpuid.h`
- Full design direction: `docs/poly-isa-design-directions.md`
