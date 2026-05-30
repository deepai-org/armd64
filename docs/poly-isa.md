# Poly ISA

Poly extends x86_64 with raw AArch64 and RISC-V64 userspace frontends.
x86_64 remains the system ISA; the target is existing SysV x86_64, AAPCS64,
and RISC-V psABI code, not a new application ABI.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused boots: `boot-poly-probe-arch-traps`, `boot-poly-bench-arch-traps`,
`boot-poly-thread-arch-traps`.

## How It Differs From x86_64

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign frontends fetch native 32-bit instructions from x86_64 virtual memory.
- All modes share x86_64 page permissions and TSO ordering.
- Poly controls are decoded instructions, not `#UD` envelopes.
- Poly state is explicit XSAVE-style architectural state, component `20`.
- Syscalls, imports, breakpoints, and illegal instructions produce userspace
  monitor trap packets. Hardware does not implement OS policy.

## Control Instructions

- `PENTER frontend`: enter AArch64 or RISC-V64.
- `PSWITCH frontend,target`: switch and branch.
- `PCALL frontend,target,sig`: switch, branch, save cross-return state, and use
  cached register-only ABI signature `sig`.
- `PTRAPRET`: resume from a monitor trap.
- `PLANDING`: mark a legal indirect landing site.

## Boundary

Hardware handles frontend switching, cross-return cookies, XSAVE state,
register-only ABI remapping, landing checks, and precise trap packets. Runtime
code handles stack arguments, aggregates, variadics, lazy binding, syscalls,
libcalls, and other memory-side ABI conversion.

## Prototype

Encodings: x86_64 `0f 3a fc <subop>`, AArch64
`0xd503201f | ((subop & 0x7f) << 5)`, RISC-V64
`0x0000700b | ((subop & 0x7f) << 25)`. CPUID base: `0x40000000`.

Design rationale: `docs/poly-isa-design-directions.md`.
