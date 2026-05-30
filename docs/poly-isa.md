# Poly ISA

Poly adds AArch64 and RISC-V64 user frontends to an x86_64 system CPU so
precompiled objects from all three ISAs can run in one virtual address space.
Design rationale is in `docs/poly-isa-design-directions.md`.

## Contract

x86_64 owns boot, privilege, paging, interrupts, faults, and the global TSO
memory model. AArch64 and RISC-V64 execute native 32-bit user instructions from
the shared address space. Poly state is explicit XSAVE-style architectural
state. The CPU does not implement Linux, libc, dynamic linker policy,
user-memory call descriptors, or per-instruction `#UD` envelopes.

## Frontends

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

- `PENTER frontend`: enter a frontend from runtime or system code.
- `PSWITCH frontend, target`: switch frontends without a recorded return.
- `PCALL frontend, target, sig`: switch frontends and record a native return.
- `PTRAPRET`: resume execution after a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing target.

## Calls

Native ABI compatibility is the default: x86_64 SysV, AArch64 AAPCS64, and
RISC-V psABI. Register-only calls use ABI signature slots; hardware remaps
architectural register names only. `PCALL` records caller frontend, PC, SP, and
flags, then installs a return cookie in the callee's native return location.
Native returns cross back by returning to the cookie.

Stack arguments, aggregates, variadics, hidden returns, incompatible vectors,
lazy binding, syscalls, and libc helpers go through software thunks or a
user-space Poly monitor.

## Bochs Prototype

Temporary prototype allocations, not final silicon opcodes:

- CPUID `0x40000000`, XSAVE component `20`, Poly state layout `8`
- x86_64 control page: `0f 3a fc <subop>`
- AArch64 control page: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control page: `0x0000700b | ((subop & 0x7f) << 25)`
