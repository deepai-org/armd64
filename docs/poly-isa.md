# Poly ISA

Poly adds hardware-visible frontend switches so existing x86_64, AArch64, and
RISC-V64 userspace code can run in one x86_64 virtual address space. Design
rationale is in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Model

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA: privilege, paging, interrupts, hard faults,
  atomics, syscalls, and TSO use x86_64 rules.
- AArch64 and RISC-V64 are user-mode fetch/decode frontends over the same VA
  space. They are not coprocessors and are not entered through `#UD`.
- Foreign architectural state is per-thread XSAVE-style state, not CR3-scoped
  hidden emulator state.
- Recoverable foreign exits produce precise Ring 3 trap packets for a userspace
  Poly monitor. Hardware does not emulate OS syscalls or libcalls.

## Controls

- `PENTER frontend`: enter a frontend at the next instruction.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: cross-call using ABI signature slot `sig`.
- `PTRAPRET`: resume after a userspace Poly trap handler.
- `PLANDING`: validate an indirect cross-ISA landing pad.

## Fast ABI Path

Fast `PCALL` is register-only. Signature slots may remap argument/result
register names in rename hardware. They must not parse descriptors, touch user
memory, repack stacks, classify aggregates, translate syscalls, or call libc.

Baseline exchange window:

`RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`

Everything outside that window is software ABI work: stack arguments, variadic
calls, by-value aggregates, lazy binding, syscall policy, and incompatible
vector ABIs.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | (subop << 5)` in reserved `HINT` space
- RISC-V64: `0x0000700b | (subop << 25)` in `custom-0`
