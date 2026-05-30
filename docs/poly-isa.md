# Poly ISA

Poly extends x86_64 with raw AArch64 and RISC-V64 userspace execution in the
same virtual address space. x86_64 remains the system ISA. The compatibility
target is existing SysV x86_64, AAPCS64, and RISC-V psABI objects.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused tests:

- `make boot-poly-probe-arch-traps`
- `make boot-poly-bench-arch-traps`
- `make boot-poly-thread-arch-traps`

## What Changes From x86_64

| Area | Poly behavior |
| --- | --- |
| Frontends | `0=x86_64`, `1=AArch64`, `2=RISC-V64` |
| Fetch | Foreign modes fetch native 32-bit instructions from x86_64 virtual memory |
| Memory | x86_64 paging, protection, and TSO ordering apply in every mode |
| Controls | `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`; no `#UD` envelopes |
| Calls | `PCALL` switches frontend, branches, saves cross-return state, and applies a register-only ABI signature |
| State | XSAVE-style Poly component, prototype component `20` |
| Traps | Syscalls, imports, breakpoints, and illegal instructions produce userspace monitor trap packets |

Hardware owns frontend switching, cross-return cookies, XSAVE state,
register-only ABI remapping, landing checks, and precise trap packets. Runtime
code owns stack arguments, aggregates, variadics, lazy binding, syscalls,
libcalls, and memory-side ABI conversion.

## Prototype Encodings

- x86_64 control prefix: `0f 3a fc <subop>`
- AArch64 control word: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control word: `0x0000700b | ((subop & 0x7f) << 25)`
- CPUID base leaf: `0x40000000`

Design details live in `docs/poly-isa-design-directions.md`.
