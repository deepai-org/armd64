# Poly ISA

Poly extends x86_64 with AArch64 and RISC-V64 user-mode frontends for running
existing precompiled code from all three ISAs in one process. x86_64 remains the
system ISA: privilege, paging, faults, interrupts, atomics, VM control, and TSO.

## Run

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign frontends fetch native aligned 32-bit instructions from `RIP`.
- Control ops are decoded instructions, not `#UD` envelopes: `PENTER`,
  `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`.
- Fast calls use register-only ABI signature slots; hardware renames registers
  but never parses descriptors or repacks memory.
- Stack args, aggregates, variadics, lazy binding, syscalls, and libc policy are
  software responsibilities.
- Poly state is XSAVE-style architectural state.
- Temporary prototype x86 encodings use `0f 3a fc <op>`.

## References

- Design directions: `docs/poly-isa-design-directions.md`
- Constants and encodings: `tools/include/polycpuid.h`
