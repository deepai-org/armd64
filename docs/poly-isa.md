# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine so
existing native code can share one virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## x86_64 Differences

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 still owns privilege, paging, faults, interrupts, VM control, and TSO.
- Foreign modes fetch native 32-bit instructions from `RIP`, not `#UD`
  envelopes.
- Foreign registers are XSAVE-style per-thread state.
- Hardware only switches frontends, tracks return cookies, emits trap packets,
  and aliases registers.
- Software handles linking, syscalls/libcalls, stack arguments, aggregates,
  variadics, and incompatible vectors.

## Control Instructions

- `PENTER frontend`: enter a frontend from trusted runtime code.
- `PSWITCH frontend, target`: non-returning cross-frontend branch.
- `PCALL frontend, target, sig`: cross-frontend call with ABI signature slot.
- `PTRAPRET`: return from a precise Poly trap.
- `PLANDING`: mark or validate indirect cross-frontend targets.

Same-ISA returns are native. Cross-frontend returns use native return
instructions plus hardware transition-stack cookies.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved HINT subspace
- RISC-V64: custom-0 opcode family

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
