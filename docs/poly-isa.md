# Poly ISA

Poly lets one x86_64 process execute precompiled x86_64, AArch64, and RISC-V64 code in one virtual address space. Rationale is in `docs/poly-isa-design-directions.md`.

## Contract

- x86_64 remains the system ISA for privilege, paging, interrupts, hard faults, syscalls, VM control, atomics, and the TSO memory model.
- AArch64/RISC-V64 are user frontends that fetch real 32-bit native instructions directly; there are no per-instruction `#UD` envelopes.
- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign state is per-thread XSAVE-style state. Recoverable exits go to Ring 3 trap packets; hard faults and interrupts stay OS-owned.

## Controls

- `PENTER frontend`: enter a frontend at the current PC.
- `PSWITCH frontend, target`: tail-switch to another frontend.
- `PCALL frontend, target, sig`: call through ABI signature slot `sig`.
- `PTRAPRET`: resume after a Ring 3 trap packet.
- `PLANDING`: mark or validate an indirect cross-ISA target.

Cross-ISA calls return through ordinary native returns plus a hardware transition stack and reserved return cookie. Same-ISA returns are unchanged.

## ABI

- Fast `PCALL` is register-only: signature slots rename registers and never parse user memory or repack stacks.
- Exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- FP signatures cover common scalar/vector register-only cases.
- Software thunks handle stack arguments, aggregates, variadics, lazy binding, syscalls, libcalls, and incompatible vector ABIs.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>` Poly control opcode page.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
