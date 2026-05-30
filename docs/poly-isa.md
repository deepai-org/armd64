# Poly ISA

Poly is an x86_64 ISA extension that can execute AArch64 and RISC-V64 userspace
code in the same process. It is not a new OS ABI: x86_64 remains authoritative
for privilege, paging, interrupts, faults, atomics, VM control, and memory
ordering.

## Run

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Architectural Delta

- Modes are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Foreign modes fetch native aligned 32-bit instructions from `RIP`.
- Frontend control uses decoded Poly instructions: `PENTER`, `PSWITCH`, `PCALL`,
  `PTRAPRET`, and `PLANDING`.
- Fast native ABI calls use hardware ABI signature slots for register mapping.
- Stack arguments, aggregates, variadics, lazy binding, and libc/syscall policy
  stay in software thunks or the user monitor.
- Extra state is XSAVE-style: foreign registers, trap packets, ABI signatures,
  transition state, monitor addresses, and landing policy.
- Prototype x86 encodings currently use temporary `0f 3a fc <op>` forms.

## References

- Full design: `docs/poly-isa-design-directions.md`
- Constants and opcodes: `tools/include/polycpuid.h`
