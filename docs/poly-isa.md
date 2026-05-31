# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends so existing
precompiled code can share one process, address space, and OS ABI boundary.

## Architectural Contract

- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  atomics, VM control, and global TSO ordering.
- AArch64 and RISC-V64 fetch real 32-bit native instructions from `RIP`; they
  are not coprocessors and are not wrapped in per-instruction `#UD` envelopes.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast calls may use hardware ABI signature slots that remap register names.
  Stack arguments, aggregates, variadics, lazy binding, and incompatible vector
  layouts stay in software thunks.
- Extra foreign registers are explicit per-thread XSAVE-style state, not hidden
  emulator state and not CR3-scoped scratch state.
- Foreign syscalls and recoverable traps produce OS-neutral trap packets for a
  user runtime monitor. Hardware does not implement Linux, libc, libgcc,
  libatomic, or dynamic-linker policy.

## Control Operations

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Op | Purpose |
| --- | --- |
| `PENTER` | Enter a frontend from trusted runtime or system code. |
| `PSWITCH` | Branch to another frontend without call semantics. |
| `PCALL` | Cross-ISA call using an ABI signature slot. |
| `PTRAPRET` | Resume from a Poly monitor or precise trap path. |
| `PLANDING` | Mark or validate an indirect cross-ISA landing point. |

Prototype encodings are non-final: x86_64 `0f 3a fc <subop>`, AArch64 reserved
`HINT`, and RISC-V `custom-0`.

For deeper rationale see `docs/poly-isa-design-directions.md`; for build and
test commands see `README.md`.
