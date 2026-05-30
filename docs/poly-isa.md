# Poly ISA Quick Reference

Poly is an x86_64 extension that can execute raw AArch64 and RISC-V64
userspace code in the same virtual address space. x86_64 stays the system ISA;
the compatibility target is existing SysV x86_64, AAPCS64, and RISC-V psABI
objects.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused boots: `boot-poly-probe-arch-traps`, `boot-poly-bench-arch-traps`,
`boot-poly-thread-arch-traps`.

## What Changes From x86_64

- Frontends are `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign modes fetch native 32-bit instructions from x86_64 virtual memory.
- x86_64 paging, protection, and TSO ordering apply in every mode.
- `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` are decoded control
  instructions, not `#UD` envelopes.
- `PCALL` switches frontend, branches, saves cross-return state, and applies a
  register-only ABI signature.
- Poly state is an XSAVE-style component, currently prototype component `20`.
- Syscalls, imports, breakpoints, and illegal instructions become userspace
  monitor trap packets.

Hardware owns frontend switching, cross-return cookies, XSAVE state,
register-only ABI remapping, landing checks, and precise trap packets. Runtime
code owns stack arguments, aggregates, variadics, lazy binding, syscalls,
libcalls, and memory-side ABI conversion.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`
- CPUID base: `0x40000000`

Design details live in `docs/poly-isa-design-directions.md`.
