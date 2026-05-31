# Poly ISA

Poly lets one x86_64 process link and execute precompiled x86_64, AArch64, and RISC-V64 code in one address space. See [poly-isa-design-directions.md](poly-isa-design-directions.md) for rationale.

## Contract

- x86_64 remains the system ISA for privilege, paging, interrupts, hard faults, syscalls, VM control, atomics, and memory ordering.
- AArch64/RISC-V64 are direct 32-bit user-mode fetch/decode frontends. There are no per-instruction `#UD` envelopes.
- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign registers are per-thread XSAVE-style state. Recoverable traps use Ring 3 trap packets; hard faults and interrupts stay OS-owned.

## Instructions

- `PENTER frontend`: enter a frontend at the current PC.
- `PSWITCH frontend, target`: tail-switch frontends.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: resume after a Ring 3 trap packet.
- `PLANDING`: mark or validate an indirect cross-ISA landing pad.

## ABI

- Fast `PCALL` only renames registers; it never parses user memory or stack layouts.
- Exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Native returns cross ISAs through a hardware transition stack and reserved return cookie.
- Software thunks handle stack args, aggregates, variadics, lazy binding, syscalls, libcalls, and incompatible vector ABIs.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | (subop << 5)` reserved `HINT`
- RISC-V64: `0x0000700b | (subop << 25)` `custom-0`
