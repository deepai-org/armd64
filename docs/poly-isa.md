# Poly ISA Quick Reference

Poly is an x86_64 extension for running existing precompiled AArch64 and
RISC-V64 userspace code in the same virtual address space. It is not a new
compiler ABI.

## Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  scheduling, atomics, hard faults, real syscalls, and TSO memory ordering.
- Foreign frontends fetch native instructions directly: AArch64 at 4-byte
  alignment, RISC-V64 at 2-byte alignment so RVC objects work.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AAPCS64, and RISC-V
  psABI.
- Fast calls use decoded frontend-control instructions plus register-only ABI
  signature slots. Stack arguments, aggregates, variadics, lazy binding, and
  incompatible vector layouts stay in software thunks.
- Foreign register state is explicit XSAVE-style architectural state, not
  hidden CR3/TLS/process-keyed emulator state.
- Recoverable foreign events produce OS-neutral trap packets for a user-mode
  Poly monitor. The OS still owns real kernel transitions.

## Not x86_64

Poly adds peer user-mode frontends; it does not replace x86_64. There are no
per-instruction `ud2` envelopes, no hardware parsing of user-memory call
descriptors, and no hardware knowledge of Linux, libc, or dynamic-linker policy.

## Bochs Encodings

Temporary prototype encodings standing in for future decoded control opcodes:

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

## References

- [Design directions](poly-isa-design-directions.md)
- [CPUID ABI](../tools/include/polycpuid.h)
- [Bochs control implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
