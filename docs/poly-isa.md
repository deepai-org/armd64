# Poly ISA Reference

Poly is a multi-frontend CPU extension for running existing x86_64, AArch64,
and RISC-V64 user code in one virtual address space. It is not a new source
ABI. The compatibility target is ordinary precompiled native ABI code, with
loader/runtime thunks only where memory-shaped ABI differences require them.

For hardware rationale and open design direction, see
`docs/poly-isa-design-directions.md`.

## Execution Model

Frontend IDs:

| ID | Frontend | Fetch rule |
| --- | --- | --- |
| `0` | x86_64 | Native variable-length x86 fetch |
| `1` | AArch64 | Aligned 32-bit instruction fetch |
| `2` | RISC-V64 | 16/32-bit instruction fetch, including RVC |

x86_64 is the system ISA. It owns boot, privilege, paging, interrupts, faults,
VM control, atomics, and the global TSO memory model. AArch64 and RISC-V64 are
user-mode frontends over the same x86_64 virtual address space.

## Control Encodings

Poly control operations are decoded instructions, not `#UD` exception
envelopes:

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)
RISC-V   0x0000700b | ((subop & 0x7f) << 25)
```

Core subops:

| Subop | Operation | Meaning |
| --- | --- | --- |
| `0x03` | `PENTER` | Enter a frontend from trusted runtime/system code |
| `0x04` | `PSWITCH` | Branch to another frontend without return |
| `0x05` | `PLANDING` | Validate an indirect cross-frontend landing target |
| `0x2d` | `PCALL` | Cross-frontend call using a signature selected in a register |
| `0x30..0x3c` | `PCALL_SLOT` | Cross-frontend call using immediate signature slot `0..12` |
| `0x62` | `PTRAPRET` | Return from a precise Poly trap/monitor packet |
| `0x65..0x6e` | `STATE` | State key, ABI signature, monitor packet, and landing policy controls |

## Calls And Returns

`PCALL` records caller frontend, PC, SP, and flags, applies a register-only ABI
signature slot, installs a reserved native return cookie, and branches to the
callee frontend.

Returns use the callee's ordinary native return instruction:

| Frontend | Return instruction |
| --- | --- |
| x86_64 | `ret` |
| AArch64 | `ret x30` |
| RISC-V64 | `ret` / `jalr x0, ra, 0` |

A return to the reserved cookie restores the caller frontend and PC. Same-ISA
returns remain normal native returns.

## ABI Boundary

Hardware handles only fixed-latency register aliasing through ABI signature
slots. This is intended to map common register-only calls and returns without
moving data through memory.

Hardware must not parse user-memory call descriptors, repack stacks or
aggregates, translate variadic calls, implement libc/libgcc helpers, or encode
OS policy. Those cases stay in loader/runtime thunks.

## State And Traps

Foreign architectural state is explicit per-thread XSAVE-style state, not
hidden CR3-scoped emulator state. The state includes foreign GPR/FP state,
frontend state, transition-stack state, ABI signature slots, and trap/monitor
control addresses.

Recoverable foreign syscalls, breakpoints, unresolved imports, unsupported
instructions, and frontend exits produce precise trap records for a runtime or
OS handler. The kernel still owns hard page faults, interrupts, scheduling, and
real syscalls.

## Current Prototype Scope

Bochs is the functional prototype. It validates decode, direct frontend
switching, native return-cookie behavior, trap packets, register/FP bridging,
and native ABI interop paths. Bochs timing is not a hardware performance claim;
few-cycle switching is the hardware/FPGA design target.
