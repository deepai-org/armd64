# Poly ISA Quick Reference

Poly extends x86_64 so precompiled AArch64 and RISC-V64 userspace code can run
in one virtual address space. x86_64 remains the system ISA.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused tests: `make boot-poly-probe-arch-traps`,
`make boot-poly-bench-arch-traps`, `make boot-poly-thread-arch-traps`.

## x86_64 Differences

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign frontends fetch native 32-bit instructions from x86_64 virtual memory.
- Mode switches are decoded control operations, not `#UD` traps.
- Foreign state is XSAVE-style architectural state.
- Memory uses x86_64 virtual memory, permissions, and TSO ordering.
- Syscalls, imports, breakpoints, and illegal instructions produce precise
  userspace monitor trap packets. The CPU does not implement OS policy.

## Control Operations

- `PENTER frontend`: enter a foreign frontend.
- `PSWITCH frontend,target`: switch frontend and branch.
- `PCALL frontend,target,sig`: switch, branch, save cross-return state, and
  apply register ABI signature `sig`.
- `PTRAPRET`: return from a monitor trap and resume at `resume_pc`.
- `PLANDING`: mark a legal frontend landing site.

## ABI Boundary

The target is existing SysV x86_64, AAPCS64, and RISC-V psABI code. There is no
separate application ABI.

Hardware handles frontend switching, cross-return recovery, XSAVE state,
precise trap packets, and register-only ABI remapping. Software handles stack
arguments, aggregates, variadics, lazy binding, syscall translation, and any
memory-side ABI conversion.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`.
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`.
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`.
- Poly CPUID base: `0x40000000`.
- Poly XSAVE component: `20`.

Design rationale: `docs/poly-isa-design-directions.md`.
