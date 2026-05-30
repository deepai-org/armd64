# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 userspace frontends to an x86_64 process. The
x86_64 architecture remains authoritative for privilege, paging, interrupts,
faults, atomics, VM control, and the effective memory model.

## Run

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused gates: `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`.

## Difference From x86_64

- Frontend modes: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign modes fetch native aligned 32-bit instructions from `RIP`.
- Control instructions: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`.
- Register-only native ABI calls use cached ABI signature slots.
- Stack args, aggregates, variadics, lazy binding, libcalls, and syscall
  translation stay in software thunks or the user monitor.
- Extra architectural state is XSAVE-style: foreign registers, trap packets,
  ABI signatures, transition state, monitor addresses, and landing policy.
- Prototype x86 encodings use temporary `0f 3a fc <op>` forms.

Detailed rationale: `docs/poly-isa-design-directions.md`.
Constants: `tools/include/polycpuid.h`.
