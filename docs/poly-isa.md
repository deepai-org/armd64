# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system CPU.
The goal is compatibility with existing precompiled objects and cross-ISA
library calls, not a new compiler-only ABI.

## Run It

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused boots: `boot-poly-probe-arch-traps`, `boot-poly-call-arch-traps`,
`boot-poly-thread-arch-traps`, `boot-poly-neutral-arch-traps`,
`boot-poly-full-arch-traps`.

## Architectural Delta From x86_64

- x86_64 remains the system frontend for boot, privilege, paging, faults,
  interrupts, and the shared TSO memory model.
- Frontend IDs are `0=x86_64`, `1=AArch64`, and `2=RISC-V64`.
- Foreign frontends fetch native 32-bit instructions directly from guest memory.
  There are no per-instruction `#UD` envelopes.
- Cross-ISA control is expressed as `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`,
  and `PLANDING`.
- `PCALL` records return state; ordinary native returns cross back through
  return cookies.
- Fast calls use register-only ABI signature slots/RAT remapping. Stack
  arguments, aggregates, variadics, syscalls, libcalls, and layout conversion
  stay in software runtime/loader thunks.
- Trap packets are OS-neutral and report frontend, status, PC, and the first
  eight native foreign ABI argument registers.
- Non-x86 architectural state is XSAVE-style Poly state. The prototype uses
  component `20`; the state import layout version is `8`.

## Temporary Bochs Encodings

These are prototype encodings, not final silicon allocations:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`
- CPUID base: `0x40000000`

Long-form rationale: `docs/poly-isa-design-directions.md`.
