# Poly ISA Quick Reference

Poly adds raw AArch64 and RISC-V64 user-mode frontends to an x86_64 system
CPU. x86_64 still owns boot, privilege, paging, faults, interrupts, atomics,
and the global TSO memory model.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- Foreign code fetches real 32-bit AArch64 or RISC-V instructions from the
  same virtual address space as x86_64.
- Foreign frontends are user-mode only and use x86_64 page permissions,
  stack memory, traps, and TSO ordering.
- Mode switches are decoded Poly operations, not `#UD` exceptions and not
  per-instruction envelopes.
- The compatibility target is ordinary precompiled SysV x86_64, AAPCS64, and
  RISC-V psABI code, including cross-ISA libraries.

## Operations

- `PENTER frontend`: enter a frontend from runtime or system code.
- `PSWITCH frontend,target`: switch frontend and branch without return state.
- `PCALL frontend,target,sig`: switch frontend, branch, save return state, and
  apply cached register-only ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.

Prototype encodings are temporary: x86_64 uses the decoded Poly control opcode
page, AArch64 uses reserved `HINT`, RISC-V uses `custom-0`, and explicit Poly
state is modeled as XSAVE component `20`.

## ABI Boundary

Hardware does fixed-latency frontend switching, native return recovery,
explicit state save/restore, trap packet delivery, and register-only ABI
signature remapping.

Software handles stack arguments, aggregates, variadics, lazy binding,
incompatible vector layouts, and any other memory-side ABI reshaping.

Trap and import packets preserve the first eight native foreign ABI argument registers
so runtimes can handle full `x0..x7` and `a0..a7` call boundaries.

## State And Traps

Foreign-only registers, ABI signature slots, trap packets, transition-stack
state, and monitor controls are explicit XSAVE-style architectural state. The
current explicit state import layout version is `3`.

Foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports,
and recoverable faults produce OS-neutral trap packets. Hardware reports the
event; runtime or OS policy decides how to handle it.

Design rationale: `docs/poly-isa-design-directions.md`.
