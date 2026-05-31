# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine so precompiled code from different ISAs can share one process and virtual address space. Design rationale lives in `docs/poly-isa-design-directions.md`.

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, faults, interrupts, VM control, real syscalls, atomics, and the global TSO memory model.
- AArch64 and RISC-V64 are user frontends, not independent machines.
- Cross-ISA control flow uses decoded Poly opcodes, never `#UD` envelopes.
- Extra foreign registers are per-thread XSAVE-style architectural state.
- Hardware switches frontends, remaps registers, records returns, and emits precise traps. It does not implement libc, loaders, syscall policy, stack repacking, or user-memory call descriptors.

## Control Flow

| Instruction | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend at the current PC. |
| `PSWITCH frontend, target` | Tail-switch to another frontend. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PTRAPRET` | Resume from a Ring 3 Poly trap packet. |
| `PLANDING` | Validate an indirect cross-ISA landing point. |

Native same-ISA branches, calls, and returns stay native. Cross-ISA returns use ordinary return instructions plus a hardware return cookie and transition stack.

## ABI And Traps

- Fast `PCALL` is register-only. Signature slots remap architectural register names without moving data.
- Null signature: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Other signatures cover native argument/result registers and hidden structure-return pointers.
- Thunks handle stack arguments, memory-shaped aggregates, variadics, lazy binding, syscall translation, libcalls, and incompatible vector types.
- Foreign `svc`/`ecall`, breakpoints, illegal or unsupported instructions, and unresolved imports produce OS-neutral Ring 3 trap packets for the user monitor.
- Page faults, scheduling, hardware interrupts, and signals remain kernel-owned.

## Encodings

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` Poly Control Opcode Page |
| AArch64 | reserved `HINT`, `0xd503201f | (subop << 5)` |
| RISC-V64 | `custom-0`, `0x0000700b | (subop << 25)` |
