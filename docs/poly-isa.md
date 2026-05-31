# Poly ISA Quick Reference

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends in the same
virtual address space.

## Running

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Model

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains responsible for boot, privilege, paging, faults, interrupts,
  atomics, and the memory model.
- AArch64/RISC-V fetch raw 32-bit instructions from `RIP`; there are no
  per-instruction `#UD` envelopes.
- Foreign registers are XSAVE-style architectural state, not hidden emulator
  state.
- Hardware switches frontends and maps common register ABI signatures; software
  handles stacks, aggregates, variadics, linking, syscalls, and libcalls.

## Instructions

- `PENTER frontend`: enter a frontend.
- `PSWITCH frontend, target`: tail-branch across frontends.
- `PCALL frontend, target, sig`: call across frontends with ABI signature `sig`.
- `PTRAPRET`: return from a user-mode Poly trap packet.
- `PLANDING`: validate an indirect cross-frontend landing pad.

Prototype encodings use x86 `0f 3a fc <subop>`, AArch64 reserved HINT, and RISC-V
custom-0. Real hardware should allocate normal decoded opcodes, not `#UD`.

Rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
