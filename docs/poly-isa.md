# Poly ISA Quick Reference

Poly runs precompiled x86_64, AArch64, and RISC-V64 code in one x86_64 process.
Design rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Model

- x86_64 owns privilege, paging, interrupts, hard faults, atomics, syscalls, and TSO.
- AArch64/RISC-V64 are user frontends fetching real instructions from the same address space.
- Mode changes are decoded control transfers, not `#UD` traps.
- Extra foreign state is per-thread XSAVE-style state.
- Recoverable foreign exits become Ring 3 trap packets; hardware has no OS/libc policy.

## Instructions

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- `PENTER frontend`: enter a frontend.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: cross-call using ABI signature slot `sig`.
- `PTRAPRET`: resume from a Poly trap packet.
- `PLANDING`: validate an indirect cross-ISA landing pad.

## ABI Contract

- Fast `PCALL` is register-only: no descriptor parsing, stack repacking, memory reads, syscall translation, or libcall emulation in hardware.
- ABI signature slots describe argument/result aliases and should compile into register-rename/RAT updates.
- Baseline window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Stack args, aggregates, variadics, lazy binding, syscall/libcall policy, and incompatible vector ABIs use software thunks or the Ring 3 monitor.

## Prototype Encodings

Bochs-only temporary encodings: x86_64 `0f 3a fc <subop>`; AArch64 `0xd503201f | (subop << 5)` in reserved `HINT`; RISC-V64 `0x0000700b | (subop << 25)` in `custom-0`.
