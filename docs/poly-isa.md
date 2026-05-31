# Poly ISA Quick Reference

Poly lets native x86_64, AArch64, and RISC-V64 user objects run in one x86_64
virtual address space. x86_64 remains the system ISA.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Model

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, faults, interrupts, atomics, and TSO.
- AArch64/RISC-V fetch native 32-bit instructions directly from `RIP`.
- All frontends share one virtual address space and protection model.
- Foreign registers are XSAVE-style architectural state, not hidden CR3 state.

## Control Flow

- `PENTER`: enter a frontend
- `PSWITCH`: tail-branch to another frontend
- `PCALL`: cross-frontend call with a register-only ABI signature slot
- `PTRAPRET`: resume after a trap packet
- `PLANDING`: validate an indirect cross-frontend landing pad

Native same-ISA returns stay ordinary. Cross-frontend returns use a hardware
transition stack and return cookies.

## Boundary

- Hardware: frontend switching, call/return state, trap packets, register-only
  ABI remapping.
- Software: linking policy, syscalls, libcalls, stack arguments, aggregates,
  variadics, lazy binding, and memory-shaped ABI work.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved HINT subspace
- RISC-V: custom-0 family

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
