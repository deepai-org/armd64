# Poly ISA Quick Reference

Poly extends x86_64 so precompiled AArch64 and RISC-V64 userspace code can run
in the same virtual address space. x86_64 remains the system ISA.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
make boot-poly-probe-arch-traps
make boot-poly-bench-arch-traps
```

## Differences From x86_64

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- x86_64 still owns privilege, paging, interrupts, faults, atomics, stacks, and
  TSO ordering.
- Foreign frontends fetch raw 32-bit instructions from x86_64 virtual memory.
- Mode switches are decoded control instructions, not `#UD` envelopes.
- Foreign state is XSAVE-style architectural state, not hidden emulator state.

## Control Operations

- `PENTER frontend`: enter a foreign frontend.
- `PSWITCH frontend,target`: switch frontend and branch.
- `PCALL frontend,target,sig`: switch, branch, save cross-return state, and
  apply register-only ABI signature `sig`.
- `PTRAPRET`: resume from a precise Poly trap.

## ABI Boundary

The target is existing SysV x86_64, AAPCS64, and RISC-V psABI code. There is no
separate `PolyFast` application ABI.

Hardware handles frontend switching, cross-return recovery, XSAVE state, trap
packets, and register-only ABI remapping. Software handles stack arguments,
aggregates, variadics, lazy binding, syscall policy, and memory-side ABI work.

- x86_64: Poly Control Opcode Page, currently `0f 3a fc <subop>`.
- AArch64: reserved `HINT` subspace.
- RISC-V64: `custom-0` opcode family.
- State: XSAVE component `20`.

Design rationale and future directions: `docs/poly-isa-design-directions.md`.
