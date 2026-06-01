# Poly ISA Reference

Poly runs existing x86_64, AArch64, and RISC-V64 user code in one virtual
address space. It is not a new source ABI. x86_64 remains the system ISA.

Design rationale: `docs/poly-isa-design-directions.md`.

## Execution Model

- Frontend `0`: x86_64, native variable-length fetch.
- Frontend `1`: AArch64, 4-byte aligned fetch.
- Frontend `2`: RISC-V64, 16/32-bit fetch including RVC.
- x86_64 owns boot, privilege, paging, interrupts, faults, VM control, and the
  global TSO memory model.
- AArch64 and RISC-V64 are user-mode peer frontends over the same virtual
  memory.

## Control Instructions

Poly operations are decoded instructions, not `#UD` envelopes.

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)
RISC-V   0x0000700b | ((subop & 0x7f) << 25)
```

Core subops:

- `0x03 PENTER`: trusted/runtime frontend entry.
- `0x04 PSWITCH`: tail-switch to another frontend.
- `0x05 PLANDING`: mark/check indirect cross-frontend landing targets.
- `0x2d PCALL`: cross-frontend call using a register-selected ABI signature.
- `0x30..0x3c PCALL_SLOT`: cross-frontend call using signature slot `0..12`.
- `0x62 PTRAPRET`: return from a Poly trap/monitor packet.
- `0x65..0x6e STATE`: state key, ABI signatures, monitor packet, landing policy.

## Calls And Returns

`PCALL` records caller frontend, return PC, SP, and flags; applies a
register-only ABI signature; installs a reserved native return cookie; then
branches to the target frontend.

Returns use ordinary native instructions: x86_64 `ret`, AArch64 `ret x30`, and
RISC-V64 `ret` / `jalr x0, ra, 0`. Returning to the reserved cookie restores
the recorded caller frontend and PC. Same-ISA calls and returns stay native.

## ABI Boundary

Hardware only remaps registers through fixed-latency ABI signature slots. This
covers common register-only calls and returns.

Hardware does not parse user-memory call descriptors, rewrite stacks, repack
aggregates, translate variadic calls, implement libc/libgcc helpers, or encode
OS policy. Loader/runtime thunks handle those cases.

## State And Traps

Foreign GPR/FP state, frontend state, transition-stack state, ABI signatures,
and trap controls are explicit per-thread XSAVE-style state.

Recoverable foreign syscalls, breakpoints, unresolved imports, unsupported
instructions, and frontend exits produce precise Poly trap records for a
runtime or OS handler. The kernel still owns hard page faults, interrupts,
scheduling, and real syscalls.

## Prototype Scope

Bochs validates decode, direct frontend switching, return-cookie behavior, trap
packets, register/FP bridging, and native ABI interop. Bochs timing is not a
hardware performance claim; few-cycle switching is the hardware/FPGA target.
