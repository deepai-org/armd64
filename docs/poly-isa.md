# Poly ISA

Poly adds raw AArch64 and RISC-V64 user-mode frontends to an x86_64 system.
x86_64 remains the system ISA: it owns boot, privilege, paging, interrupts,
fault delivery, atomics, and the effective TSO memory model.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Delta

- Foreign code fetches normal 32-bit AArch64 or RISC-V64 instructions from the same virtual address space as x86_64.
- Foreign execution is user-mode only and inherits x86_64 page permissions, precise traps, stack memory, and TSO ordering.
- Mode switches are decoded Poly control ops, not `#UD` envelopes and not one envelope per foreign instruction.
- The target is existing precompiled SysV x86_64, AAPCS64, and RISC-V psABI code.

## Control

- `PENTER frontend`: enter a foreign frontend.
- `PSWITCH frontend,target`: switch frontend and branch without return state.
- `PCALL frontend,target,sig`: switch frontend, branch, save return state, and apply cached register-only ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.

Prototype encodings are isolated: x86_64 uses the Poly control opcode page, AArch64 uses reserved `HINT`, RISC-V uses `custom-0`, and explicit Poly state is modeled as XSAVE component `20`.

## Boundary

Hardware handles fixed-latency frontend switching, native-return recovery, explicit state save/restore, trap-packet delivery, and register-only ABI signature remapping.

Hardware does not parse user-memory call descriptors, repack stack arguments, reshape structs, translate libcalls, or know OS syscall ABIs.

Software thunks handle stack arguments, aggregates, variadics, lazy binding, incompatible vector layouts, syscall policy, and other memory-side ABI work.

## State

Foreign-only registers, ABI signature slots, trap packets, transition-stack state, and monitor controls are explicit XSAVE-style architectural state. The current explicit import/export layout version is `3`.

Trap and import packets preserve the first eight native foreign ABI argument registers so runtimes can handle full `x0..x7` and `a0..a7` call boundaries.

Detailed rationale: `docs/poly-isa-design-directions.md`.
