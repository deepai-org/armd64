# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends so existing
precompiled libraries can run in one virtual address space. The goal is native
ABI compatibility, not a new compiler target.

## How It Differs From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, scheduling, real syscalls, atomics, and TSO ordering.
- AArch64 and RISC-V64 are user-mode native-instruction frontends.
- Cross-ISA control uses decoded Poly opcodes, not per-instruction `ud2`
  envelopes.
- Fast interop is register-only ABI signature switching. Stack args,
  aggregates, variadics, lazy binding, and other memory-shaped ABI cases stay in
  loader/runtime thunks.
- Foreign state is explicit XSAVE-style architectural state. Recoverable exits
  are OS-neutral trap packets for a Ring 3 monitor/runtime.
- Hardware does not emulate Linux, libc, libgcc, libatomic, dynamic-linker
  policy, or user-memory call descriptors.

## Temporary Bochs Encodings

These are prototype encodings for future dedicated decoded control opcodes:

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

## Checks

```sh
make image
make boot-poly-probe-arch-traps
make boot-poly-real-xsave-arch-traps
```

Details: [design directions](poly-isa-design-directions.md), [CPUID ABI](../tools/include/polycpuid.h), [Bochs control implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc).
