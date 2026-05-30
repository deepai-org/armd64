# Poly ISA

Poly is an x86_64 CPU extension for running existing AArch64 and RISC-V64
userspace code in the same virtual address space as x86_64 code.

See `docs/poly-isa-design-directions.md` for rationale and open design work.

## Contract

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, faults,
  and the baseline memory model.
- AArch64 and RISC-V64 are user-mode frontends that fetch native 32-bit
  instructions directly from `RIP`.
- Foreign frontends are not Linux emulation modes and not coprocessors.
- Poly state is explicit XSAVE-style architectural state, not hidden state
  keyed by `CR3`, PID, or TLS.
- Traps are OS-neutral packets. Hardware does not implement Linux syscalls,
  libc helpers, dynamic-linker policy, or user-memory call descriptors.
- A Poly trap packet carries the first eight native foreign ABI argument
  registers.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter a frontend from runtime or system code.
- `PSWITCH frontend, target`: branch to another frontend without a return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature
  slot `sig`.
- `PTRAPRET`: resume from a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing pad.

`PCALL` records caller frontend, PC, SP, and flags in Poly state, then installs
a reserved return cookie in the callee's native return location. Ordinary
native returns cross back by returning to that cookie.

## ABI And Traps

Fast calls use register-only ABI signature slots. The signature remaps native
argument and result registers without reading memory. Software thunks handle
stack arguments, aggregates, variadics, hidden structure returns, incompatible
vectors, syscalls, libcalls, unresolved imports, and lazy binding.

## Prototype Constants

These Bochs encodings are temporary and not final silicon allocations:

- CPUID base: `0x40000000`
- XSAVE component: `20`
- The state import layout version is `8`.
- x86_64 control prefix: `0f 3a fc <subop>`
- AArch64 control base: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control base: `0x0000700b | ((subop & 0x7f) << 25)`
