# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends so existing
precompiled libraries can share one virtual address space. It targets native ABI
compatibility, not a new compiler ABI.

## Difference From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 stays the system ISA for boot, privilege, paging, interrupts, real
  syscalls, hard faults, atomics, scheduling, and TSO ordering.
- AArch64 fetches native 32-bit instructions. RISC-V64 fetches native 16/32-bit
  instructions, including RVC.
- Cross-ISA calls use decoded control instructions plus register-only ABI
  signature slots. Memory-shaped ABI work stays in software thunks.
- Foreign state is explicit XSAVE-style state. Recoverable foreign events are
  OS-neutral trap packets.
- No per-instruction `ud2` envelopes, hardware-parsed call descriptors,
  Linux/libc emulation, or CPU-side dynamic-linker policy.

## Prototype Encodings

Temporary Bochs encodings stand in for future decoded control opcodes:

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Details: [design directions](poly-isa-design-directions.md),
[CPUID ABI](../tools/include/polycpuid.h),
[Bochs control implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc).
