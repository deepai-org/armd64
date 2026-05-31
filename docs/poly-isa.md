# Poly ISA

Poly lets one x86_64 process execute precompiled x86_64, AArch64, and RISC-V64 code in one virtual address space. Detailed rationale: `docs/poly-isa-design-directions.md`.

## Contract

- x86_64 is the system ISA: privilege, paging, faults, interrupts, atomics, syscalls, VM control, and TSO stay x86-owned.
- AArch64/RISC-V64 are user frontends that fetch real 32-bit native instructions directly from `RIP`; no per-instruction `#UD` envelopes.
- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign state is per-thread XSAVE-style architectural state. Recoverable exits become Ring 3 trap packets; hard faults and interrupts stay OS-owned.

## Controls

| Operation | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend at the current PC. |
| `PSWITCH frontend, target` | Tail-switch to another frontend. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PTRAPRET` | Resume from a Ring 3 trap packet. |
| `PLANDING` | Validate an indirect cross-ISA target. |

Cross-ISA returns use ordinary native returns plus a hardware transition stack and reserved return cookie. Same-ISA returns stay native.

## ABI

- Fast `PCALL` is register-only: signature slots rename registers without memory reads or data moves.
- Low-level exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Software thunks handle stack arguments, aggregates, variadics, lazy binding, syscalls, libcalls, and incompatible vectors.

## Encodings

| Frontend | Encoding space |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` Poly Control Opcode Page |
| AArch64 | reserved `HINT`, `0xd503201f | (subop << 5)` |
| RISC-V64 | `custom-0`, `0x0000700b | (subop << 25)` |
