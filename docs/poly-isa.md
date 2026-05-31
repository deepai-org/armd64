# Poly ISA

Poly is an x86_64 extension for running precompiled AArch64 and RISC-V64 user
code in the same process and virtual address space. Deeper rationale lives in
`docs/poly-isa-design-directions.md`.

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, faults, interrupts, VM control, atomics,
  real syscalls, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode frontends, not independent machines.
- Cross-ISA control flow uses decoded Poly instructions, not `#UD` envelopes.
- Non-aliased foreign registers are per-thread XSAVE-style architectural state.
- Hardware does not implement libc, dynamic linking, stack repacking, syscall policy,
  or user-memory call descriptors.

## Control Flow

- `PENTER frontend`: enter a frontend at the current PC.
- `PSWITCH frontend, target`: switch frontends without a return edge.
- `PCALL frontend, target, sig`: cross-ISA call through ABI signature slot `sig`.
- `PTRAPRET`: resume after a precise Ring 3 Poly trap.
- `PLANDING`: validate an indirect cross-ISA landing point.
- Same-ISA branches, calls, and returns stay native.
- Cross-ISA returns use native return instructions plus a hardware return cookie
  and transition stack.

## ABI

- Fast `PCALL` is register-only.
- ABI signatures alias `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` to `x0..x7` and `a0..a7`
  without moving data.
- Software thunks handle stack arguments, aggregates, variadics, structure returns,
  lazy binding, syscall translation, libcalls, and incompatible vectors.

## Traps And Encodings

- Foreign `svc`/`ecall`, breakpoints, illegal/unsupported instructions, and unresolved
  imports produce OS-neutral Ring 3 trap packets for a user-space monitor.
- Page faults, scheduling, interrupts, and signals remain kernel-owned.

- x86_64: `0f 3a fc <subop>` Poly Control Opcode Page.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
