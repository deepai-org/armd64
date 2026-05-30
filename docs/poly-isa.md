# Poly ISA

Poly lets an x86_64 process run precompiled AArch64 and RISC-V64 userspace code
in the same virtual address space. It is a compatibility ISA extension, not a
new compiler-only ABI.

## Smoke Tests

```sh
make image
make boot-poly-probe-arch-traps
make boot-poly-real-xsave-arch-traps
```

## What Changes From x86_64

- Frontends are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- x86_64 remains the system ISA for boot, privilege, paging, interrupts, OS
  scheduling, syscalls, atomics, and TSO ordering.
- AArch64 and RISC-V64 execute through native foreign frontends. AArch64 fetch
  is fixed 32-bit; RISC-V fetch is 16/32-bit so RVC remains valid.
- Poly control ops switch, call, return, and trap across frontends. There is no
  per-instruction `ud2` envelope in the fast path.
- ABI signatures remap registers only. Software thunks handle stack arguments,
  aggregates, variadics, lazy binding, and loader/runtime policy.
- Foreign state is explicit XSAVE-style state; recoverable exits produce
  OS-neutral trap packets for a Ring 3 runtime.
- Hardware does not emulate Linux, libc, libgcc, libatomic, or parse user-memory
  call descriptors.

## Prototype Encodings

These prototype encodings stand in for future dedicated silicon opcodes:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

## References

- Full status and test targets: [README.md](../README.md)
- Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- CPUID/XSAVE contract: [polycpuid.h](../tools/include/polycpuid.h)
- Bochs control decode: [proc_ctrl.cc](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
