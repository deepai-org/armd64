# Poly ISA

Poly is a hardware-oriented extension for executing x86_64, AArch64, and
RISC-V64 user code in one x86_64 virtual address space. The goal is compatibility
with existing native objects and shared libraries, not a new compiler-only ABI.

Detailed rationale and open design directions are in
`docs/poly-isa-design-directions.md`.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused targets:

- `make boot`: plain x86_64 VM sanity check.
- `make boot-poly-arch-traps`: raw AArch64/RISC-V execution and trap tests.
- `make boot-poly-neutral-arch-traps`: direct AArch64<->RISC-V transitions.
- `make boot-poly-call-arch-traps`: cross-ISA calls, threads, and signals.
- `make boot-poly-full-arch-traps`: broad regression run.

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, atomics, virtual memory, and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends. They fetch normal 32-bit
  instructions directly from the same virtual address space.
- Cross-ISA transfer uses decoded control instructions. There is no
  per-instruction `#UD` envelope on the fast path.
- Foreign register state is explicit XSAVE-style architectural state, not hidden
  CR3-scoped emulator state.
- Foreign syscalls, breakpoints, illegal instructions, unresolved imports, and
  policy exits produce precise Poly trap packets for user/runtime software.
- Hardware does not implement an OS, libc, linker, loader policy, or
  memory-parsing ABI descriptors.

## Control Model

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter a frontend from runtime/system code.
- `PSWITCH frontend, target`: switch frontends without native call state.
- `PCALL frontend, target, sig`: switch frontends and record native return state.
- `PTRAPRET`: resume after software handles a Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing target.

`PCALL` records caller frontend, PC, SP, and flags in a hardware transition
stack, then installs a return cookie in the callee's native return location.
Returning to that cookie restores the caller frontend.

ABI signature slots may remap register names for register-only calls. Hardware
only renames registers. Stack arguments, aggregates, variadics, hidden returns,
incompatible vectors, lazy binding, syscalls, and libc helpers stay in software
thunks/runtime code.

## Bochs Prototype

Temporary encodings, not final silicon allocations:

- CPUID leaf `0x40000000`, XSAVE component `20`, Poly state layout `8`.
- x86_64 control page: `0f 3a fc <subop>`.
- AArch64 control page: `0xd503201f | ((subop & 0x7f) << 5)`.
- RISC-V64 control page: `0x0000700b | ((subop & 0x7f) << 25)`.
