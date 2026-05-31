# Poly ISA

Poly is an x86_64 extension for running precompiled AArch64 and RISC-V64 user
code in the same process and virtual address space. The compatibility target is
normal SysV x86_64, AAPCS64, and RISC-V psABI objects.

## Architectural Contract

- x86_64 remains the system ISA for boot, kernel entry, paging, exceptions,
  interrupts, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode decode frontends over that x86 machine.
- Foreign code is fetched directly: AArch64 at 4-byte alignment, RISC-V at
  2-byte alignment for RVC. There are no per-instruction `#UD` envelopes.
- Non-x86 architectural state is explicit per-thread XSAVE-style state.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Control

- `PENTER frontend`: enter a foreign frontend at the current PC.
- `PSWITCH frontend, target`: cross-ISA branch with no return.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: return from a Ring 3 Poly trap monitor.
- `PLANDING`: mark or validate an indirect cross-ISA landing point.

Same-ISA code uses ordinary native calls, branches, and returns.

## ABI Boundary

Hardware provides fixed-latency frontend switching, return-cookie handling, and
register aliasing. It must not parse user-memory descriptors or rewrite stack
layouts.

`PCALL` records caller frontend, PC, SP, and flags in Poly state, then places a
reserved return cookie in the callee return location. Ordinary native return
instructions cross back by resolving that cookie.

ABI signature slots define register mappings. Silicon may apply them with
rename/RAT updates. Software thunks handle stack arguments, aggregates,
variadics, hidden structure returns, lazy binding, syscall translation, and
libcalls.

Foreign `svc`/`ecall`, breakpoints, illegal or unsupported instructions,
unresolved imports, and recoverable exits produce OS-neutral trap packets for a
Ring 3 Poly monitor. The kernel only sees normal architectural faults or
interrupts.

## Fixed Register Window

This window is for low-level thunks. Native ABI interop should use signature
slots when ABI register orders differ.

`P0..P7` map to x86_64 `RAX,RDX,RCX,RDI,RSI,R8,R9,R10`, AArch64 `x0..x7`, and
RISC-V64 `a0..a7`.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>` Poly Control Opcode Page.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.

Hot `PCALL` forms encode the signature slot as an immediate. Design rationale
and open hardware questions live in `docs/poly-isa-design-directions.md`.
