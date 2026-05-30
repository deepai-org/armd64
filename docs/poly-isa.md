# Poly ISA

Poly is a multi-frontend CPU extension. x86_64 remains the system ISA;
AArch64 and RISC-V64 run as user-mode frontends in the same virtual address
space.

This file is the quick reference. The design rationale lives in
`docs/poly-isa-design-directions.md`.

## Run The Prototype

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused targets:

- `make boot-poly-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

## What Changes From x86_64

- x86_64 still owns boot, paging, privilege, interrupts, faults, VM control,
  atomics, and global TSO ordering.
- AArch64 and RISC-V64 fetch ordinary aligned 32-bit instructions from the
  shared address space.
- Mode changes are decoded Poly control instructions, not exception-driven
  `#UD` envelopes.
- Fast cross-ISA calls use register-only ABI signature slots.
- Stack arguments, aggregates, variadics, lazy binding, libc policy, and
  syscall translation are software responsibilities.
- Foreign architectural state is explicit XSAVE-style state.

## Prototype Constants

Frontend IDs:

- `0`: x86_64
- `1`: AArch64
- `2`: RISC-V64

| Item | Value |
| --- | --- |
| x86_64 temporary control space | `0f 3a fc <subop>` |
| AArch64 temporary control space | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 temporary control space | `0x0000700b | ((subop & 0x7f) << 25)` |
| Control ops | `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING` |
| CPUID base leaf | `0x40000000` |
| XSAVE component | `20` |
| Poly state layout | `8` |
