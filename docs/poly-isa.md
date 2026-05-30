# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system CPU.
x86_64 still owns boot, privilege, paging, interrupts, faults, atomics, and TSO.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- Foreign code fetches real 32-bit instructions from the same virtual address
  space as x86_64.
- Foreign frontends are user-mode only and share x86_64 page permissions, stack
  memory, and memory ordering.
- Mode switches are decoded Poly operations, not `#UD` exceptions.
- The target is ordinary precompiled SysV x86_64, AAPCS64, and RISC-V psABI
  code.
- Hardware handles frontend switching, return state, explicit Poly state,
  traps, and register-only ABI signatures.
- Software handles stack arguments, aggregates, variadics, lazy binding, and
  complex ABI reshaping.

## Operations

- `PENTER frontend`: enter a frontend from runtime/system code.
- `PSWITCH frontend,target`: switch frontend and branch without return state.
- `PCALL frontend,target,sig`: switch frontend, branch, save return state, and
  apply cached register ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.

Prototype encodings are temporary: x86_64 `0f 3a fc <subop>`, AArch64 reserved
`HINT`, RISC-V `custom-0`, Poly state XSAVE component `20`.

## ABI

`PCALL` does not parse user-memory descriptors or repack stack layouts. Its
optional signature only remaps register names, such as SysV
`RDI,RSI,RDX,RCX,R8,R9` to AAPCS64 `x0..x5` or RISC-V `a0..a5`.

The first eight native foreign argument registers are preserved in trap/import
packets so runtimes can handle full `x0..x7` / `a0..a7` boundaries.

## State And Traps

Foreign-only registers, ABI signature slots, trap packets, transition-stack
state, and monitor controls are explicit XSAVE-style architectural state.
Current explicit state import layout version: `3`.

Foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports,
and recoverable faults produce OS-neutral trap packets. Hardware reports the
event; runtime or OS policy decides how to handle it.

Detailed rationale: `docs/poly-isa-design-directions.md`.
