# Poly ISA

Poly is an x86_64 extension for running existing precompiled AArch64 and
RISC-V64 userspace libraries in one virtual address space. The goal is native
ABI compatibility, not a new compiler target.

## Architectural Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  scheduling, real syscalls, hard faults, atomics, and TSO memory ordering.
- AArch64 fetch is direct 32-bit instruction fetch. RISC-V64 fetch accepts
  16-bit alignment so existing RVC objects work.
- Cross-ISA calls target existing ABIs: x86_64 SysV, AAPCS64, and RISC-V psABI.
- Fast calls use decoded frontend-control instructions plus register-only ABI
  signature slots. Memory-shaped ABI work stays in software thunks.
- Foreign state is explicit XSAVE-style architectural state, never hidden
  CR3/TLS/process-keyed emulator state.
- Recoverable foreign events produce OS-neutral user-mode trap packets. The OS
  still owns kernel transitions.

## Difference From x86_64

Poly adds peer user-mode frontends beside x86_64. It does not add
per-instruction `ud2` envelopes, hardware-parsed call descriptors, Linux-aware
syscall emulation, libc traps, or dynamic-linker policy in the CPU.

## Bochs Encodings

Temporary prototype encodings stand in for future decoded control opcodes:

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

## References

- [Design directions](poly-isa-design-directions.md)
- [CPUID ABI](../tools/include/polycpuid.h)
- [Bochs control implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
