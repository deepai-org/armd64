# Poly ISA

Poly is an x86_64 extension that adds user-mode AArch64 and RISC-V64
frontends. The goal is to run existing precompiled objects from all three ISAs
inside one process and address space.

## Contract

- x86_64 stays the system ISA for privilege, paging, interrupts, faults,
  atomics, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 execute real native 32-bit instructions from `RIP`.
- Cross-ISA calls target native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast register-only calls use hardware ABI signature slots for register-name
  remapping.
- Stack arguments, aggregates, variadics, lazy binding, and incompatible vector
  layouts use software thunks.
- Extra foreign registers are per-thread XSAVE-style architectural state.
- Foreign syscalls and recoverable traps produce OS-neutral user-runtime trap
  packets. Hardware does not implement Linux, libc, libgcc, libatomic, or
  dynamic-linker policy.

## Differences From x86_64

- The instruction frontend can switch between variable-length x86_64 decode and
  fixed-width AArch64/RISC-V64 decode.
- Foreign modes share the x86_64 virtual address space and protection model.
- Foreign exceptions return precise trap packets instead of hard-coded OS or
  libc emulation.
- Foreign architectural state is saved by the same kind of OS-visible mechanism
  as XSAVE state, not by hidden emulator bookkeeping.

## Control Ops

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Op | Purpose |
| --- | --- |
| `PENTER` | Enter a frontend from trusted runtime or system code. |
| `PSWITCH` | Branch to another frontend without call semantics. |
| `PCALL` | Cross-ISA call using an ABI signature slot. |
| `PTRAPRET` | Resume from a Poly monitor or precise trap path. |
| `PLANDING` | Mark or validate an indirect cross-ISA landing point. |

Prototype encodings are temporary: x86_64 `0f 3a fc <subop>`, AArch64 reserved
`HINT`, and RISC-V `custom-0`.

See `docs/poly-isa-design-directions.md` for rationale and `README.md` for
build/test commands.
