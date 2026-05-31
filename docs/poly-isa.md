# Poly ISA Reference

Poly lets existing x86_64, AArch64, and RISC-V64 userspace code run in one
x86_64 virtual address space. Rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns privilege, paging, interrupts, hard faults, atomics, syscalls,
  and TSO.
- AArch64/RISC-V64 are user frontends that directly fetch native instructions.
- Mode changes are decoded control transfers, not `#UD` traps.
- Foreign register state is per-thread XSAVE-style architectural state.
- Recoverable foreign exits produce precise Ring 3 trap packets.

## Controls

- `PENTER frontend`: enter a frontend.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: cross-call using ABI signature slot `sig`.
- `PTRAPRET`: resume from a Poly trap packet.
- `PLANDING`: validate an indirect cross-ISA landing pad.

## ABI Boundary

Fast `PCALL` is register-only. Hardware may remap argument/result registers
through ABI signature slots, ideally as register-rename/RAT updates. It must not
parse user-memory descriptors, repack stacks, translate syscalls, emulate libc,
or inspect aggregate layouts.

Baseline exchange window:

`RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`

Stack arguments, by-value aggregates, variadics, lazy binding, syscall/libcall
policy, and incompatible vector ABIs are software responsibilities.

## Bochs Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | (subop << 5)` in reserved `HINT` space
- RISC-V64: `0x0000700b | (subop << 25)` in `custom-0`
