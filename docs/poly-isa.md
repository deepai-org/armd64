# Poly ISA Quick Reference

Poly runs precompiled x86_64, AArch64, and RISC-V64 user code in one x86_64 process and address space. Design rationale lives in `docs/poly-isa-design-directions.md`.

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA: privilege, paging, interrupts, faults, real syscalls, atomics, VM control, and TSO memory ordering.
- AArch64/RISC-V64 are direct-fetch user frontends. No per-instruction `#UD` envelopes.
- Foreign register state is explicit per-thread XSAVE-style state.
- Hardware does frontend switching, register remapping, return-cookie recovery, and precise trap packets. Software does loaders, libc, syscall policy, and complex ABI thunks.

## Instructions

| Instruction | Meaning |
| --- | --- |
| `PENTER frontend` | Enter a frontend at the current PC. |
| `PSWITCH frontend, target` | Tail-switch to another frontend. |
| `PCALL frontend, target, sig` | Cross-ISA call using register signature slot `sig`. |
| `PTRAPRET` | Resume from a Ring 3 trap packet. |
| `PLANDING` | Mark/validate an indirect cross-ISA target. |

Same-ISA branches and returns stay native. Cross-ISA returns use the callee's ordinary return instruction plus a hardware transition stack and reserved return cookie.

## ABI And Traps

- Fast `PCALL` is register-only. Signature slots remap names without moving data or reading memory.
- Null exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Native signatures cover SysV, AAPCS64, RISC-V psABI, FP/vector, and hidden structure-return register cases.
- Thunks handle stack arguments, memory-shaped aggregates, variadics, lazy binding, syscalls, libcalls, and incompatible vectors.
- Foreign `svc`/`ecall`, breakpoints, illegal/unsupported instructions, and unresolved imports produce OS-neutral Ring 3 trap packets.

## Prototype Encodings

| Frontend | Encoding space |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` Poly Control Opcode Page |
| AArch64 | reserved `HINT`, `0xd503201f | (subop << 5)` |
| RISC-V64 | `custom-0`, `0x0000700b | (subop << 25)` |
