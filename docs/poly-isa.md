# Poly ISA Quick Reference

Poly runs existing x86_64, AArch64, and RISC-V64 user code in one virtual
address space. x86_64 remains the system ISA.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused gates:

- `make boot-poly-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

## ISA Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, faults, interrupts, atomics, VM control,
  and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends that fetch normal aligned
  32-bit instructions from `RIP` in the same virtual address space.
- Frontend changes use decoded Poly control opcodes, not `#UD` trap envelopes.
- Poly state is XSAVE-style architectural state: foreign registers, trap
  packets, ABI signatures, transition state, and landing policy.
- Fast interop uses register-only ABI signature slots. Runtime thunks handle
  stack args, aggregates, variadics, lazy binding, libcalls, and syscall
  translation.

## Operations

Core operations are `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`.
The current x86 prototype uses `0f 3a fc <op>` as a temporary decoded control
space. AArch64 and RISC-V control encodings are generated from subops in
`tools/include/polycpuid.h`.

- Constants and prototype encodings: `tools/include/polycpuid.h`
- Hardware and ABI rationale: `docs/poly-isa-design-directions.md`
