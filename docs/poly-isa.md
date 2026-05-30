# Poly ISA

Poly is an x86_64 extension for running existing x86_64, AArch64, and RISC-V64
userspace code in one process. x86_64 stays the system ISA: privilege, paging,
faults, interrupts, atomics, VM control, and global TSO ordering remain x86-owned.

## Run The Prototype

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Delta

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign frontends fetch native aligned 32-bit instructions from `RIP`.
- Cross-frontend control is decoded, not exception based: `PENTER`, `PSWITCH`,
  `PCALL`, `PTRAPRET`, and `PLANDING`.
- Hot native-ABI calls use register-only ABI signature slots.
- Stack arguments, aggregates, variadics, lazy binding, syscalls, and libc policy
  are handled by software thunks or a user monitor, not by hardware.
- Poly state is XSAVE-style architectural state: foreign registers, trap packets,
  transition stack, ABI signatures, monitor addresses, and landing policy.
- Prototype x86 encodings use temporary `0f 3a fc <op>` forms.

## More Detail

- Design rationale: `docs/poly-isa-design-directions.md`
- Constants and encodings: `tools/include/polycpuid.h`
