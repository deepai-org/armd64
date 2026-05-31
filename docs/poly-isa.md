# Poly ISA

Poly is an x86_64 CPU extension for running precompiled AArch64 and RISC-V64
user code in one process and virtual address space. The compatibility target is
ordinary native ABI code: SysV x86_64, AAPCS64, and RISC-V psABI.

This is the short ISA reference. Rationale and open questions live in
`docs/poly-isa-design-directions.md`.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts, faults,
  atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode frontends that fetch real native
  instructions from the same virtual address space.
- Cross-ISA control flow uses decoded Poly control instructions, not `#UD`
  envelopes.
- Non-x86 state is per-thread architectural state saved through an XSAVE-style
  component.
- Hardware does not implement Linux, libc, libgcc, dynamic linking, stack
  repacking, or user-memory call descriptors.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Control Instructions

- `PENTER frontend`: enter a frontend at the current PC.
- `PSWITCH frontend, target`: branch to another frontend without return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature slot
  `sig`.
- `PTRAPRET`: resume after a precise Ring 3 Poly trap.
- `PLANDING`: mark or validate an indirect cross-ISA landing point.

Same-ISA code uses ordinary native branches, calls, and returns.

## ABI, Returns, And Traps

Fast `PCALL` is register-only: the selected ABI signature aliases source
argument/result registers to target argument/result registers with fixed latency.
Software thunks handle stack arguments, aggregates, variadics, hidden structure
returns, lazy binding, syscall translation, libcalls, and incompatible vectors.

The null signature exposes this low-level exchange window:
`RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.

Cross-ISA calls return through ordinary native return instructions. Hardware
uses a return cookie plus a transition stack to recover the caller frontend, PC,
SP, and flags.

Foreign `svc`/`ecall`, breakpoints, illegal or unsupported instructions,
unresolved imports, and other recoverable exits produce OS-neutral trap packets
for a Ring 3 Poly monitor. The kernel still owns hard page faults, scheduling,
interrupts, signals, and real syscalls issued by the monitor.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>` Poly Control Opcode Page.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.
