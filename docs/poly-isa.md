# Poly ISA

Poly is a hardware-oriented extension that lets x86_64, AArch64, and RISC-V64
user code execute in one virtual address space. The goal is compatibility with
existing compiled objects, not a new compiler-only ABI. Design rationale is in
`docs/poly-isa-design-directions.md`.

## Architectural Contract

- x86_64 owns boot, privilege, paging, faults, interrupts, and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends that fetch normal 32-bit code.
- Non-x86 registers are explicit XSAVE-style architectural state.
- Cross-ISA control transfer is a real instruction path, never `#UD` trapping.
- Syscalls, breakpoints, illegal instructions, and unresolved imports produce
  precise Poly trap packets for user/runtime software.
- Hardware does not implement OS, libc, linker, or ABI descriptor policy.

## Frontends And Control

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter a frontend from runtime/system code.
- `PSWITCH frontend, target`: switch frontends without call state.
- `PCALL frontend, target, sig`: switch frontends and record native return state.
- `PTRAPRET`: resume after software handles a Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing target.

`PCALL` records caller frontend, PC, SP, and flags in the hardware transition
stack, then installs a return cookie in the callee's native return location.
Returning to that cookie restores the caller frontend.

ABI signature slots may remap register names for register-only calls. Hardware
only renames registers. Stack arguments, aggregates, variadics, hidden returns,
incompatible vectors, lazy binding, syscalls, and libc helpers are handled by
software thunks/runtime code.

## Bochs Prototype

Temporary encodings, not final silicon allocations:

- CPUID leaf `0x40000000`, XSAVE component `20`, Poly state layout `8`
- x86_64 control page: `0f 3a fc <subop>`
- AArch64 control page: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control page: `0x0000700b | ((subop & 0x7f) << 25)`
