# Poly ISA Quick Reference

Poly extends an x86_64 machine with AArch64 and RISC-V64 user-mode frontends.
It targets existing native ABI code and shared libraries, not a new compiler
ABI.

## Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  syscalls, scheduling, atomics, and TSO memory ordering.
- Foreign code is direct-fetched. AArch64 uses 4-byte alignment; RISC-V64 allows
  2-byte RVC alignment. There are no per-instruction `#UD` envelopes.
- Cross-ISA calls use native ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Register-only calls may use ABI signature slots. Stack args, aggregates,
  variadics, and lazy binding stay in software thunks.
- Foreign state is explicit XSAVE-style architectural state, not hidden
  CR3/TLS-keyed emulator state.
- Foreign traps produce OS-neutral trap packets.

## Controls

| Frontend | Temporary Bochs encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

These are temporary Bochs encodings for decoded frontend controls. They are not
`ud2` traps and do not carry user-memory call descriptors.

## References

- [Design directions](poly-isa-design-directions.md)
- [CPUID ABI](../tools/include/polycpuid.h)
- [Bochs control implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
