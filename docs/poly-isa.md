# Poly ISA

Short hardware contract for running precompiled x86_64, AArch64, and RISC-V64 code in one x86_64 process. Rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Model

- x86_64 is the system ISA: privilege, paging, interrupts, hard faults, syscalls, VM control, atomics, and global TSO stay x86-owned.
- AArch64 and RISC-V64 are user-mode peer frontends that fetch real native 32-bit instructions from the same address space.
- Poly controls are decoded instructions, not `#UD` envelopes.
- Poly state is explicit per-thread XSAVE-style state. Recoverable exits use Ring 3 trap packets; the OS still owns hard faults, scheduling, and syscalls.

## Frontends And Controls

Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter a frontend at the current PC.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: resume after a Ring 3 Poly trap packet.
- `PLANDING`: mark or validate an indirect cross-ISA landing pad.

## ABI Boundary

- Fast `PCALL` is fixed-latency register aliasing only; it never reads user memory, parses descriptors, or repacks stack layouts.
- Signature slots describe native ABI register mappings. The null exchange window is `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Native returns cross ISAs through a hardware transition stack and reserved return cookie.
- Software thunks handle stack args, aggregates, variadics, lazy binding, syscalls, libcalls, and incompatible vector ABIs.

## Prototype Encodings

Prototype only; final architectural allocations are still open.

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | (subop << 5)` in reserved `HINT` space
- RISC-V64: `0x0000700b | (subop << 25)` in `custom-0` space
