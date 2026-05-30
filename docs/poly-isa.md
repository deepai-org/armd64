# Poly ISA Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system CPU.
The goal is compatibility with existing precompiled native objects in one
virtual address space. Rationale lives in `docs/poly-isa-design-directions.md`.

## Architectural Contract

- x86_64 owns boot, privilege, paging, interrupts, faults, and the global TSO
  memory model.
- AArch64 and RISC-V64 fetch and execute native 32-bit user instructions
  directly from the shared RIP/PC stream.
- Per-instruction `#UD` envelopes are not part of the ISA.
- Poly state is explicit XSAVE-style architectural state.
- The CPU does not implement Linux, libc, dynamic linker policy, or user-memory
  call descriptors. Those remain runtime or monitor responsibilities.

## Frontend Operations

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

- `PENTER frontend`: enter a frontend from runtime or system code.
- `PSWITCH frontend, target`: switch frontends without recording a return.
- `PCALL frontend, target, sig`: switch frontends and record a native return.
- `PTRAPRET`: resume execution after a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing target.

## Cross-ISA Calls

- Native ABI compatibility is the default: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Register-only calls use ABI signature slots. Hardware remaps architectural
  register names and does not touch stack or aggregate memory.
- `PCALL` records caller frontend, PC, SP, and flags, then installs a reserved
  return cookie in the callee's native return location.
- Normal native returns cross back by returning to the cookie.
- Stack arguments, aggregates, variadics, hidden returns, incompatible vectors,
  lazy binding, syscalls, and libc helpers go through software thunks or a
  user-space Poly monitor.

## Bochs Prototype

These encodings are temporary prototype allocations, not final silicon opcodes.

- CPUID base: `0x40000000`
- XSAVE component: `20`
- Poly state layout: `8`
- x86_64 control page: `0f 3a fc <subop>`
- AArch64 control page: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control page: `0x0000700b | ((subop & 0x7f) << 25)`
