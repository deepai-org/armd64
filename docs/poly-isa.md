# Poly ISA Quick Reference

Poly extends x86_64 with AArch64 and RISC-V64 user-mode frontends for
precompiled code in one process and virtual address space. Design rationale is
in `docs/poly-isa-design-directions.md`.

## Contract

- Frontends are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- x86_64 owns boot, privilege, paging, faults, interrupts, VM control, atomics, real syscalls, and the global TSO memory model.
- AArch64 and RISC-V64 are user frontends, not independent machines.
- Cross-ISA control flow uses decoded Poly instructions, not `#UD` envelopes.
- Non-aliased foreign registers are per-thread XSAVE-style architectural state.
- Hardware does not implement libc, dynamic linking, stack repacking, syscall policy, or user-memory call descriptors.

## Control Flow

- `PENTER frontend`: enter a frontend at the current PC.
- `PSWITCH frontend, target`: switch frontends without a return edge.
- `PCALL frontend, target, sig`: cross-ISA call through ABI signature slot `sig`.
- `PTRAPRET`: resume after a precise Ring 3 Poly trap.
- `PLANDING`: validate an indirect cross-ISA landing point.
- Same-ISA branches, calls, and returns stay native; cross-ISA returns use native return instructions plus a hardware return cookie and transition stack.

## ABI

- Fast `PCALL` is register-only.
- ABI signatures select fixed register maps without moving data.
- Null map: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Register-only signatures cover ordinary argument/result registers and hidden
  structure-return pointers.
- Software thunks handle stack arguments, memory-shaped aggregates, variadics,
  structure-return stack reshaping, lazy binding, syscall translation, libcalls,
  and incompatible vectors.

## Traps

- Foreign `svc`/`ecall`, breakpoints, illegal/unsupported instructions, and unresolved imports produce OS-neutral Ring 3 trap packets for a user monitor.
- Page faults, scheduling, interrupts, and signals remain kernel-owned.

## Encodings

- x86_64: `0f 3a fc <subop>` Poly Control Opcode Page.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
