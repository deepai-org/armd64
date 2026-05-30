# Poly ISA

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Other useful targets: `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`, `make boot-poly-full-arch-traps`.

## What Changes From x86_64

Poly keeps x86_64 as the system ISA. AArch64 and RISC-V64 are added as
user-mode frontends in the same virtual address space.

- Boot, paging, privilege, interrupts, faults, atomics, VM, and TSO remain x86_64.
- AArch64/RISC-V64 fetch normal aligned 32-bit instructions from shared `RIP`.
- Mode changes use real decoded control instructions, not `#UD` envelopes.
- Foreign architectural state is explicit XSAVE-style state.
- Syscalls, imports, illegal instructions, and breakpoints exit through
  runtime-visible trap packets, not OS-specific CPU policy.
- Fast cross-ISA calls use register alias/signature slots.
- Complex ABI cases use software thunks for stack, aggregate, and variadic rules.

## Current Encodings

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

Control operations: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`.

- x86_64 temporary space: `0f 3a fc <subop>`
- AArch64 temporary space: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 temporary space: `0x0000700b | ((subop & 0x7f) << 25)`
- CPUID base leaf: `0x40000000`
- XSAVE component: `20`
- Poly state layout: `8`

Full design: `docs/poly-isa-design-directions.md`.
