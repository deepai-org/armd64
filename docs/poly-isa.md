# Poly ISA Quick Reference

Poly lets an x86_64 process execute user-mode AArch64 and RISC-V64 code in the
same virtual address space. The goal is compatibility with existing
precompiled libraries, not a new source-level ABI.

## Run

```sh
make image
make boot-poly-probe-arch-traps
make boot-poly-real-xsave-arch-traps
```

## Delta From x86_64

- Frontends are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  scheduling, syscalls, atomics, and TSO ordering.
- AArch64 and RISC-V64 fetch direct native 32-bit instructions; Poly control
  ops switch/call/return/trap across frontends without per-instruction `ud2`
  envelopes.
- ABI signatures remap registers only. Software thunks handle stack arguments,
  aggregates, variadics, lazy binding, and loader policy.
- Foreign state is explicit XSAVE-style state; recoverable exits produce
  OS-neutral trap packets for a Ring 3 runtime.
- Hardware does not emulate Linux, libc, libgcc, libatomic, or parse
  user-memory call descriptors.

## Temporary Bochs Encodings

These prototype encodings stand in for future dedicated silicon opcodes:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

## References

- Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- CPUID and XSAVE ABI: [polycpuid.h](../tools/include/polycpuid.h)
- Bochs implementation: [proc_ctrl.cc](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
