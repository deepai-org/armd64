# Poly ISA Quick Reference

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Useful targets: `make boot-poly-arch-traps`, `make boot-poly-neutral-arch-traps`,
`make boot-poly-call-arch-traps`, `make boot-poly-full-arch-traps`.

## Model

Poly keeps x86_64 as the system ISA and adds AArch64/RISC-V64 user-mode
frontends inside the same virtual address space.

- x86_64 owns boot, privilege, paging, interrupts, faults, atomics, VM, and TSO.
- AArch64/RISC-V64 fetch normal aligned 32-bit instructions from shared `RIP`.
- Poly transitions are decoded control instructions, not `#UD` trap envelopes.
- Foreign state is explicit XSAVE-style architectural state.
- Runtime-visible trap packets handle syscalls, breakpoints, illegal
  instructions, imports, and policy exits without OS-specific hardware.
- Trap packets carry the first eight native foreign ABI argument registers.

Fast cross-ISA calls use register alias/signature slots. Calls needing stack
repacking, aggregate ABI rules, or variadic handling use software thunks.

## Encodings

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

Operations: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`.

- CPUID base leaf `0x40000000`; XSAVE component `20`; state layout `8`
- Explicit state import layout version is `8`.
- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Full design: `docs/poly-isa-design-directions.md`.
