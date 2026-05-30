# Poly ISA

Poly lets x86_64, AArch64, and RISC-V64 user code share one virtual address
space. The target is compatibility with existing compiled objects, not a new
compiler-only ABI. Rationale lives in `docs/poly-isa-design-directions.md`.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, and the global TSO memory model.
- AArch64 and RISC-V64 are user frontends that directly fetch normal 32-bit
  instructions from the same address space.
- Non-x86 state is explicit XSAVE-style architectural state.
- Cross-ISA transfer is architectural, not a `#UD` exception path.
- Foreign syscalls, breakpoints, illegal instructions, and unresolved imports
  produce precise Poly trap packets for software.
- The CPU does not implement OS, libc, dynamic linker, or descriptor policy.

## Instructions

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

- `PENTER frontend`: enter a frontend from runtime or system code.
- `PSWITCH frontend, target`: switch frontends without recording a return.
- `PCALL frontend, target, sig`: switch frontends and record a native return.
- `PTRAPRET`: resume after a Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing target.

## Calls

Default ABIs are SysV x86_64, AAPCS64, and RISC-V psABI. `PCALL` records the
caller frontend, PC, SP, and flags, then installs a return cookie in the
callee's native return location. Returning to the cookie restores the caller.

ABI signature slots remap architectural register names for register-only calls.
The hardware only remaps registers. Stack arguments, aggregates, variadics,
hidden returns, incompatible vectors, lazy binding, syscalls, and libc helpers
remain software responsibilities.

## Bochs Prototype Encodings

Temporary allocations, not final silicon opcodes:

- CPUID `0x40000000`, XSAVE component `20`, Poly state layout `8`
- x86_64 control page: `0f 3a fc <subop>`
- AArch64 control page: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control page: `0x0000700b | ((subop & 0x7f) << 25)`
