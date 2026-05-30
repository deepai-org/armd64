# Poly ISA Reference

Poly runs existing AArch64 and RISC-V64 userspace code beside x86_64 in one
virtual address space. Design rationale: `docs/poly-isa-design-directions.md`.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode peer frontends that fetch native 32-bit
  instructions directly from RIP/PC.
- There are no per-instruction `#UD` envelopes.
- Poly state is architectural XSAVE-style state, not hidden emulator state.
- Hardware does not implement Linux, libc, linker policy, or user-memory call
  descriptors. Those are runtime/monitor jobs.

## Control Operations

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter a frontend from runtime/system code.
- `PSWITCH frontend, target`: non-returning cross-frontend branch.
- `PCALL frontend, target, sig`: cross-frontend call using signature slot `sig`.
- `PTRAPRET`: resume after a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing pad.

## ABI

- Compatibility targets real native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Fast paths use register-only ABI signature slots. Hardware remaps register
  names; it never reads or rewrites stack data.
- `PCALL` records caller frontend, PC, SP, and flags, installs a reserved return
  cookie in the callee's native return location, and switches frontend.
- Ordinary native returns cross back by returning to the cookie.
- Stack arguments, aggregates, variadics, hidden returns, incompatible vectors,
  lazy binding, syscalls, and libc helpers use software thunks or the user-space
  Poly monitor.

## Bochs Prototype Encodings

Temporary only; not final silicon allocations.

- CPUID base `0x40000000`; XSAVE component `20`; state layout `8`.
- x86_64 control: `0f 3a fc <subop>`.
- AArch64 control: `0xd503201f | ((subop & 0x7f) << 5)`.
- RISC-V64 control: `0x0000700b | ((subop & 0x7f) << 25)`.
