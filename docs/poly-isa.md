# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode decode frontends to an x86_64 machine.
x86_64 remains the system ISA for paging, faults, interrupts, syscalls,
scheduling, and OS-visible state. The goal is compatibility with existing
precompiled cross-ISA libraries, not a new compiler-only ABI.

## Run

```bash
make image
make boot-poly
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Full validation: `make boot-poly-full-real-xsave-arch-traps`

## Architectural Rules

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign code is direct-fetched as native 32-bit instructions. There are no
  per-instruction `#UD` envelopes.
- Cross-ISA calls target native ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Register-only calls can use hardware ABI signature slots; stack arguments,
  aggregates, and variadics use software thunks.
- Non-shared foreign state is explicit XSAVE-style state, not hidden CR3 state.
- Foreign syscalls/traps exit through OS-neutral trap packets or runtime code.
- Memory ordering inherits the x86_64 machine contract.

## Prototype Control Opcodes

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Temporary Bochs encodings; these model real frontend controls, not `ud2` traps.

## More Detail

- [README](../README.md)
- [Design directions](poly-isa-design-directions.md)
- [CPUID ABI](../tools/include/polycpuid.h)
- [Bochs implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
