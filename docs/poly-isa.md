# Poly ISA

Poly is an x86_64 CPU extension for running precompiled AArch64 and RISC-V64
user code in one process and virtual address space. The target is binary
interop with normal SysV x86_64, AAPCS64, and RISC-V psABI objects.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode frontends over that machine.
- Foreign instructions are fetched directly; there are no per-instruction
  `#UD` envelopes.
- AArch64 fetch is 4-byte aligned; RISC-V fetch is 2-byte aligned for RVC.
- Non-x86 state is explicit per-thread XSAVE-style architectural state.

Frontend IDs: `0` = x86_64, `1` = AArch64, `2` = RISC-V64.

## Control

| Operation | Meaning |
| --- | --- |
| `PENTER frontend` | Enter another frontend at the current PC. |
| `PSWITCH frontend, target` | Cross-ISA branch with no return. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PTRAPRET` | Resume from a Ring 3 Poly trap monitor. |
| `PLANDING` | Mark or validate an indirect cross-ISA landing point. |

These are decoded control instructions. Same-ISA code uses normal native calls,
branches, and returns.

## ABI Boundary

Hardware owns only fixed-latency frontend switching, return-cookie handling,
and register aliasing. Runtime/loader thunks own memory-shaped ABI work:
stack arguments, aggregates, variadics, hidden structure returns, lazy binding,
syscall translation, and libcalls.

`PCALL` selects an ABI signature slot. Silicon can apply the slot by
register-renaming/RAT updates instead of executing register moves. Invalid
slots trap before changing frontend or PC.

Cross-ISA calls return through ordinary native return instructions. `PCALL`
records caller frontend, PC, SP, and flags in Poly state, then installs a
reserved return cookie in the callee return location.

Foreign `svc`/`ecall`, breakpoints, illegal instructions, unsupported
instructions, unresolved imports, and recoverable exits produce OS-neutral trap
packets for a Ring 3 Poly monitor.

## Register Window

Low-level thunks can use this fixed register window. Native ABI calls that need
different source/target mappings should use programmed signature slots.

| P-reg | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `P0` | `RAX` | `x0` | `a0` |
| `P1` | `RDX` | `x1` | `a1` |
| `P2` | `RCX` | `x2` | `a2` |
| `P3` | `RDI` | `x3` | `a3` |
| `P4` | `RSI` | `x4` | `a4` |
| `P5` | `R8` | `x5` | `a5` |
| `P6` | `R9` | `x6` | `a6` |
| `P7` | `R10` | `x7` | `a7` |

## Prototype Encodings

| Frontend | Encoding family |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` Poly Control Opcode Page |
| AArch64 | reserved `HINT`: `0xd503201f | (subop << 5)` |
| RISC-V64 | `custom-0`: `0x0000700b | (subop << 25)` |

Hot signature-slot forms use immediate subopcodes so calls do not need a
temporary register move. Hardware rationale and open design direction live in
`docs/poly-isa-design-directions.md`.
