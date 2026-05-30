# Poly ISA

Poly adds raw AArch64 and RISC-V64 userspace frontends to x86_64. x86_64
remains the system ISA; compatibility targets existing SysV x86_64, AAPCS64,
and RISC-V psABI code.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused boots: `boot-poly-probe-arch-traps`, `boot-poly-bench-arch-traps`,
`boot-poly-thread-arch-traps`.

## x86_64 Delta

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign modes fetch native 32-bit instructions from x86_64 virtual memory.
- x86_64 page permissions and TSO ordering apply in every mode.
- Poly controls are decoded instructions, not `#UD` envelopes.
- Poly state is XSAVE-style component `20`.
- Syscalls, imports, breakpoints, and illegal instructions produce userspace
  monitor trap packets.

## Instructions

- `PENTER frontend`: enter a foreign frontend.
- `PSWITCH frontend,target`: switch frontend and branch.
- `PCALL frontend,target,sig`: switch, branch, save cross-return state, and
  apply register-only ABI signature `sig`.
- `PTRAPRET`: resume from a monitor trap.
- `PLANDING`: mark a legal indirect landing site.

## Hardware Boundary

Hardware handles frontend switching, cross-return cookies, XSAVE state,
register-only ABI remapping, landing checks, and precise trap packets.
Runtime code handles stack arguments, aggregates, variadics, lazy binding,
syscalls, libcalls, and memory-side ABI conversion.

## Prototype

Encodings: x86_64 `0f 3a fc <subop>`, AArch64
`0xd503201f | ((subop & 0x7f) << 5)`, RISC-V64
`0x0000700b | ((subop & 0x7f) << 25)`. CPUID base: `0x40000000`.

Design rationale: `docs/poly-isa-design-directions.md`.
