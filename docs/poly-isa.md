# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine. The
goal is compatibility with existing native ABI code and shared libraries, not a
new compiler ABI.

## Model

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, interrupts, hard faults, scheduling,
  atomics, real syscalls, and global TSO memory ordering.
- AArch64 and RISC-V64 fetch normal native instructions from the same virtual
  address space. AArch64 is 4-byte aligned; RISC-V64 allows 2-byte RVC
  alignment.
- Cross-ISA calls target native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Hardware switches frontends and aliases register arguments. Software thunks
  handle stack arguments, aggregates, variadics, lazy binding, and incompatible
  vector layouts.
- Foreign state is explicit XSAVE-style architectural state. It is not hidden
  emulator state keyed by CR3, TLS, or process identity.
- Recoverable foreign events produce OS-neutral trap packets for a user-mode
  Poly monitor. The host OS still owns real kernel transitions.

## Difference From x86_64

Poly does not replace x86_64. It adds alternate user-mode instruction
frontends that share the x86_64 address space, memory ordering, and OS contract.
There are no per-instruction `ud2` envelopes and no hardware-parsed call
descriptors in user memory.

## Prototype Controls

These temporary Bochs encodings stand in for future real decoded control
opcodes:

| Frontend | Temporary encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

## Details

- [Design directions](poly-isa-design-directions.md)
- [CPUID ABI](../tools/include/polycpuid.h)
- [Bochs control implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
