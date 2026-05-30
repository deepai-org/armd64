# Poly ISA

Poly adds raw AArch64 and RISC-V64 userspace frontends to an x86_64 system
CPU. The compatibility target is existing native ABI code: SysV x86_64,
AAPCS64, and RISC-V psABI.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Other useful targets: `boot-poly-probe-arch-traps`,
`boot-poly-call-arch-traps`, `boot-poly-thread-arch-traps`,
`boot-poly-full-arch-traps`.

## Contract

| Area | Poly behavior |
| --- | --- |
| System ISA | x86_64 owns boot, privilege, paging, interrupts, faults, and TSO ordering. |
| Frontends | `0=x86_64`, `1=AArch64`, `2=RISC-V64`. |
| Fetch | Foreign frontends fetch native 32-bit instructions from the shared x86_64 virtual address space. |
| Controls | `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`; no per-instruction `#UD` envelopes. |
| Calls | `PCALL` changes frontend, branches, records cross-return state, and may apply a register-only ABI signature. |
| Returns | Native return instructions cross back through hardware return cookies. |
| State | Foreign architectural state is explicit XSAVE-style Poly state, prototype component `20`. |
| Traps | Syscalls, imports, breakpoints, illegal instructions, and faults produce OS-neutral trap packets. |
| Runtime work | Stack arguments, aggregates, variadics, lazy binding, syscalls, libcalls, and memory-side ABI conversion stay in software. |

## Prototype Encodings

These are Bochs prototype encodings, not final silicon opcode assignments:
`0f 3a fc <subop>` on x86_64, `0xd503201f | ((subop & 0x7f) << 5)` on
AArch64, and `0x0000700b | ((subop & 0x7f) << 25)` on RISC-V64. The prototype
CPUID base leaf is `0x40000000`.

Design rationale: `docs/poly-isa-design-directions.md`.
