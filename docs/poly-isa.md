# Poly ISA Quick Reference

Poly ISA lets one x86_64 process enter precompiled AArch64 or RISC-V64 userspace code. x86_64 remains the system ISA for paging, privilege, interrupts, faults, atomics, VM control, and TSO memory ordering.

## Run It

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused gates: `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`, `make boot-poly-full-arch-traps`.

## Difference From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign mode fetches native aligned 32-bit instructions from `RIP`.
- `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` are decoded controls, not `#UD` traps.
- Extra architectural state is XSAVE-style: foreign registers, trap packets, ABI signatures, transitions, and landing policy.
- Fast interop uses ABI signature slots for register-only native ABI calls; software thunks handle stack arguments, aggregates, variadics, lazy binding, libcalls, and syscall translation.
- Prototype x86 controls use temporary `0f 3a fc <op>` encodings; constants live in `tools/include/polycpuid.h`.

Detailed rationale: `docs/poly-isa-design-directions.md`.
