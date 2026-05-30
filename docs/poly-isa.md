# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 userspace frontends to an x86_64 process. It is
not a new OS ABI. x86_64 stays authoritative for privilege, paging, interrupts,
faults, atomics, VM control, and memory ordering.

## Run The Prototype

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- Frontend modes: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign modes fetch native aligned 32-bit instructions at `RIP`.
- `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` switch frontends.
- Fast native ABI calls use hardware ABI signature slots for register mapping.
- Complex ABI work stays in software thunks or the user monitor.
- Extra state is XSAVE-style: foreign registers, trap packets, ABI signatures,
  transition state, monitor addresses, and landing policy.
- Prototype x86 encodings use temporary `0f 3a fc <op>` forms.

More detail: `docs/poly-isa-design-directions.md`.
Constants: `tools/include/polycpuid.h`.
