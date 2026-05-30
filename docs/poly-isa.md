# Poly ISA Reference

Poly is an x86_64 CPU extension that lets existing AArch64 and RISC-V64
userspace code execute in the same virtual address space as x86_64 code.
For rationale and open design work, see `docs/poly-isa-design-directions.md`.

## Architecture

- x86_64 is the system ISA for boot, privilege, paging, interrupts, faults, and
  the baseline memory model.
- AArch64 and RISC-V64 are user-mode frontends that fetch native 32-bit
  instructions directly from `RIP`.
- Foreign frontends are not Linux emulation modes, coprocessors, or libcall
  accelerators.
- Poly state is explicit XSAVE-style architectural state, not hidden state
  keyed by `CR3`, PID, or TLS.
- Traps are OS-neutral packets. Hardware does not implement Linux syscalls,
  libc helpers, dynamic-linker policy, or user-memory call descriptors.

## Controls

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

- `PENTER frontend`: enter a frontend from runtime or system code.
- `PSWITCH frontend, target`: branch to another frontend without a return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature
  slot `sig`.
- `PTRAPRET`: resume from a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing pad.

`PCALL` saves caller frontend, PC, SP, and flags in Poly state, installs a
reserved return cookie in the callee's native return location, and switches
frontend. Ordinary native returns cross back by returning to the cookie.

## ABI

Fast calls use register-only ABI signature slots. A signature remaps native
argument/result registers without reading memory.

Software thunks handle non-register ABI work: stack arguments, aggregates,
variadics, hidden structure returns, incompatible vectors, syscalls, libcalls,
unresolved imports, and lazy binding.

## Bochs Prototype

The current Bochs encodings are temporary and not final silicon allocations.

- CPUID base: `0x40000000`
- XSAVE component: `20`
- State import layout: `8`
- x86_64 control prefix: `0f 3a fc <subop>`
- AArch64 control base: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control base: `0x0000700b | ((subop & 0x7f) << 25)`
