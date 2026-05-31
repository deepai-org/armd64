# Poly ISA

Hardware contract for running precompiled x86_64, AArch64, and RISC-V64 code in one x86_64 process. Rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Model

- x86_64 owns privilege, paging, interrupts, hard faults, syscalls, VM control, atomics, and global TSO.
- AArch64 and RISC-V64 are user frontends fetching native instructions from the same virtual address space.
- Poly controls are decoded instructions, not `#UD` envelopes.
- Extra state is per-thread XSAVE-style state; recoverable exits use Ring 3 trap packets.

## Controls

Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter at current PC.
- `PSWITCH frontend, target`: tail-branch across frontends.
- `PCALL frontend, target, sig`: call across frontends using ABI signature slot `sig`.
- `PTRAPRET`: resume after a Ring 3 trap packet.
- `PLANDING`: validate indirect cross-ISA landing pads.

## ABI

- Fast `PCALL` aliases registers only; it never reads user memory or repacks stacks.
- Signature slots map native ABI argument/result registers, ideally by RAT remapping.
- Exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Cross-ISA returns use a hardware transition stack and reserved return cookies.
- Thunks handle stack args, aggregates, variadics, lazy binding, syscalls, libcalls, and incompatible vector ABIs.

## Prototype Encodings

Temporary Bochs encodings; final silicon allocations are open.

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | (subop << 5)` in reserved `HINT` space
- RISC-V64: `0x0000700b | (subop << 25)` in `custom-0` space
