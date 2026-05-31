# Poly ISA

Poly is an x86_64 CPU extension for running precompiled AArch64 and RISC-V64
user code in the same process and virtual address space.

Compatibility targets ordinary native ABIs: SysV x86_64, AAPCS64, and RISC-V
psABI. Design rationale and open hardware questions live in
`docs/poly-isa-design-directions.md`.

## Contract

- x86_64 owns boot, privilege, paging, faults, interrupts, atomics, VM control,
  and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode frontends that fetch real native
  instructions from the same address space.
- There are no per-instruction `#UD` envelopes.
- Non-x86 architectural state is explicit per-thread XSAVE-style state.
- Hardware does not implement Linux, libc, dynamic linking, stack repacking, or
  user-memory call descriptors.

## Operations

Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter a foreign frontend at the current PC.
- `PSWITCH frontend, target`: cross-ISA branch with no return.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: return from a Ring 3 Poly trap monitor.
- `PLANDING`: mark or validate an indirect cross-ISA landing point.

Same-ISA code uses ordinary native calls, branches, and returns. Cross-ISA calls
also return through ordinary native return instructions via a hardware return
cookie and transition stack.

## ABI

Hardware handles fixed-latency frontend switching, return-cookie handling, and
register-only ABI signature slots. Software thunks handle stack arguments,
aggregates, variadics, hidden structure returns, lazy binding, syscall
translation, and libcalls.

The null signature exposes `P0..P7` for low-level thunks:
`RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.

Foreign `svc`/`ecall`, breakpoints, illegal or unsupported instructions, and
recoverable exits produce OS-neutral trap packets for a Ring 3 Poly monitor.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>` Poly Control Opcode Page.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
