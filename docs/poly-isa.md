# Poly ISA Quick Reference

Poly runs existing x86_64, AArch64, and RISC-V64 userspace code in one x86_64
virtual address space. x86_64 remains the privileged/control ISA.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- x86_64 still owns privilege, paging, interrupts, faults, VM control, atomics,
  and TSO memory ordering.
- AArch64 and RISC-V64 are additional user-mode frontends, not coprocessors.
- Foreign frontends fetch native instructions directly from `RIP`; there is no
  per-instruction `#UD` envelope.
- Foreign architectural state is XSAVE-style process/thread state.

## Poly Controls

- `PENTER frontend`: enter a frontend from trusted runtime code.
- `PSWITCH frontend, target`: non-returning cross-frontend branch.
- `PCALL frontend, target, sig`: cross-frontend call using ABI signature slot.
- `PTRAPRET`: return from a precise Poly trap.
- `PLANDING`: mark or validate indirect cross-frontend targets.
- Same-ISA returns are native; cross-frontend returns use native return
  instructions plus transition-stack cookies.

## Hardware Boundary

Hardware handles frontend switching, call/return cookies, precise trap packets,
XSAVE state, and fixed-latency register-only ABI remapping.

Software handles syscalls, libcalls, linking, stack arguments, aggregates,
variadics, incompatible vectors, and memory-shaped ABI translation.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved HINT subspace
- RISC-V: custom-0 opcode family

Full rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
