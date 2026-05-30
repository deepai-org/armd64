# Poly ISA

Poly keeps x86_64 as the system ISA. AArch64 and RISC-V64 are alternate
user-mode frontends in the same address space.

Full design notes: `docs/poly-isa-design-directions.md`.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused targets: `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`, `make boot-poly-full-arch-traps`.

## ISA Delta

- x86_64 owns boot, paging, privilege, interrupts, faults, VM, atomics, and TSO.
- AArch64/RISC-V64 fetch normal aligned 32-bit instructions from the shared PC.
- Mode changes use decoded Poly control instructions, not `#UD` envelopes.
- Fast cross-ISA calls use register alias signature slots.
- Stack arguments, aggregates, variadics, lazy binding, and libc/syscall policy
  stay in software thunks or a user-mode Poly monitor.
- Foreign architectural state is explicit XSAVE-style state.

## Temporary Encodings

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- x86_64 temporary space: `0f 3a fc <subop>`
- AArch64 temporary space: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 temporary space: `0x0000700b | ((subop & 0x7f) << 25)`
- Control ops: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`
- CPUID base leaf: `0x40000000`
- XSAVE component: `20`
- Poly state layout: `8`
