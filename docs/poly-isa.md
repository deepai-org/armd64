# Poly ISA

Poly is an x86_64 extension for running precompiled AArch64 and RISC-V64 user
code in one process and virtual address space. Target ABIs: SysV x86_64,
AAPCS64, and RISC-V psABI. Rationale: `docs/poly-isa-design-directions.md`.

## Contract

- x86_64 owns boot, privilege, paging, faults, interrupts, VM control, atomics,
  and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode instruction frontends.
- Cross-ISA control flow uses decoded Poly instructions, not `#UD` envelopes.
- Foreign register state is per-thread state saved through an XSAVE-style
  component.
- Hardware does not implement libc, libgcc, dynamic linking, stack repacking,
  syscall policy, or user-memory call descriptors.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Instructions

- `PENTER frontend`: enter a frontend at the current PC.
- `PSWITCH frontend, target`: switch frontends without creating a return edge.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: resume after a precise Ring 3 Poly trap.
- `PLANDING`: validate an indirect cross-ISA landing point.

Same-ISA branches, calls, and returns stay native.

## ABI

Fast `PCALL` is register-only. A signature aliases source argument/result
registers to target argument/result registers with fixed latency:

`RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`

Software thunks handle stack arguments, aggregates, variadics, hidden structure
returns, lazy binding, syscall translation, libcalls, and incompatible vectors.

Cross-ISA calls return through ordinary native return instructions. Hardware
uses a return cookie and transition stack to restore caller frontend, PC, SP,
and flags.

Foreign `svc`/`ecall`, breakpoints, illegal/unsupported instructions, and
unresolved imports produce OS-neutral Ring 3 trap packets. The kernel still owns
page faults, scheduling, interrupts, signals, and real syscalls issued by the
monitor.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>` Poly Control Opcode Page.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
