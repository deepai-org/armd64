# Poly ISA

Poly is an x86_64 extension for running existing AArch64 and RISC-V64
userspace code in the same virtual address space as x86_64 code.

For design rationale, see `docs/poly-isa-design-directions.md`.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts, and
  faults.
- AArch64 and RISC-V64 are user-mode frontends that fetch native 32-bit
  instructions directly from `RIP`.
- Foreign frontends are not Linux-specific emulation modes and not
  coprocessors.
- Cross-ISA calls target normal native ABIs. Hardware switches frontends and
  applies register-only ABI signatures; runtime thunks handle stack and memory
  layout cases.
- Poly state is explicit XSAVE-style architectural state, not hidden emulator
  state keyed by `CR3`, PID, or TLS.
- Traps are OS-neutral packets. Hardware does not implement Linux syscalls,
  libc helpers, dynamic-linker policy, or user-memory call descriptors.
- A Poly trap packet carries the first eight native foreign ABI argument
  registers.

## Frontends And Controls

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

## ABI Boundary

Fast calls use register-only ABI signature slots. A signature remaps native
argument and result registers without reading memory.

Software thunks handle stack arguments, aggregates, variadics, hidden structure
returns, incompatible vectors, syscalls, libcalls, unresolved imports, and lazy
binding.

## Prototype Encodings

These are Bochs prototype encodings, not final silicon allocations:

- CPUID base: `0x40000000`
- XSAVE component: `20`
- The state import layout version is `8`.
- x86_64 control prefix: `0f 3a fc <subop>`
- AArch64 control base: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control base: `0x0000700b | ((subop & 0x7f) << 25)`
