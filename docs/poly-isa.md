# Poly ISA Reference

Poly is an x86_64 CPU extension for running existing AArch64 and RISC-V64
userspace code in the same virtual address space as x86_64 code.

Rationale and open design work: `docs/poly-isa-design-directions.md`.

## Contract

- x86_64 remains the system ISA for privilege, paging, interrupts, faults, and
  the global TSO memory model.
- AArch64 and RISC-V64 are peer user-mode frontends that fetch native 32-bit
  instructions from the shared virtual address space.
- Poly state is explicit XSAVE-style architectural state, not hidden emulator
  state.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and unresolved
  imports produce OS-neutral trap packets.
- Hardware does not implement Linux syscall policy, libc helpers, dynamic
  linker policy, or user-memory call descriptors.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter a frontend.
- `PSWITCH frontend, target`: non-returning cross-frontend branch.
- `PCALL frontend, target, sig`: cross-frontend call with ABI signature slot
  `sig`.
- `PTRAPRET`: resume after a Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing pad.

## ABI

- The compatibility target is real native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Fast calls use register-only ABI signature slots. The CPU remaps register
  names; it does not read memory.
- `PCALL` records caller frontend, PC, SP, and flags, installs a reserved return
  cookie in the callee's native return location, and switches frontend.
- Ordinary native returns cross back by returning to the cookie.
- Software thunks handle stack arguments, aggregates, variadics, hidden
  structure returns, incompatible vectors, syscalls, libcalls, unresolved
  imports, and lazy binding.

## Bochs Prototype

Temporary encodings, not final silicon allocations:

- CPUID base `0x40000000`; XSAVE component `20`; state layout `8`.
- x86_64 control: `0f 3a fc <subop>`.
- AArch64 control: `0xd503201f | ((subop & 0x7f) << 5)`.
- RISC-V64 control: `0x0000700b | ((subop & 0x7f) << 25)`.
