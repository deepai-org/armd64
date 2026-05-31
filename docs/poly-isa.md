# Poly ISA

Poly is a hardware-visible multi-frontend extension for running existing
x86_64, AArch64, and RISC-V64 userspace code in one x86_64 virtual address
space. It targets native ABI compatibility, not a new compiler-only ABI.

## Architectural Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  atomics, VM control, syscalls, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode fetch/decode frontends over the same
  virtual address space, fetching native instructions directly.
- Control operations are decoded instructions, not `#UD` envelopes.
- Foreign state is explicit per-thread XSAVE-style state.
- Hardware does not emulate Linux, libc, dynamic linking, or syscall policy.

## Controls

- `PENTER frontend`: enter a frontend at the next instruction.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: cross-call using ABI signature slot `sig`.
- `PTRAPRET`: resume after a Poly trap handler.
- `PLANDING`: validate an indirect cross-ISA landing pad.

## ABI Boundary

Fast `PCALL` is register-only. Signature slots may remap argument/result
register names in rename hardware, but must not read user memory, parse
descriptors, repack stacks, classify aggregates, translate syscalls, or call
helpers.

Baseline exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` =
`a0..a7`.

Software thunks handle stack arguments, variadics, by-value aggregates,
incompatible vectors, lazy binding, imports, and syscall policy.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved `HINT` encoding, `0xd503201f | (subop << 5)`
- RISC-V64: `custom-0` encoding, `0x0000700b | (subop << 25)`

Run commands are in [../README.md](../README.md). Design rationale is in
[poly-isa-design-directions.md](poly-isa-design-directions.md).
