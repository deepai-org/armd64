# Poly ISA

Poly runs precompiled x86_64, AArch64, and RISC-V64 userspace in one x86_64
virtual address space. x86_64 owns system state; foreign ISAs are user-mode
frontends only.

## Run

```bash
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

For a shorter regression run, use `make boot-poly`.

## Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, faults, interrupts, VM control, atomics,
  and memory ordering.
- Foreign fetch uses native bytes from `RIP`: AArch64 is 32-bit aligned;
  RISC-V64 is 16-bit aligned when RVC is enabled.
- Transitions use decoded Poly control opcodes, not `#UD` envelopes.
- Register-only cross-ISA calls use cached ABI signature slots; stack/aggregate
  ABI cases use loader/runtime thunks.
- Cross-ISA returns use native returns plus transition-stack cookies.
- Foreign syscalls, breakpoints, and libcalls produce OS-neutral trap packets.

## Temporary Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

## References

- Constants, CPUID leaves, subops: [polycpuid.h](../tools/include/polycpuid.h)
- Hardware design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- Bochs prototype: [proc_ctrl.cc](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
