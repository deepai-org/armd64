# Poly ISA

Poly runs existing x86_64, AArch64, and RISC-V64 userspace code in one x86_64
virtual address space.

## Run

```bash
make image
make boot-poly
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Full path: `make boot-poly-full-real-xsave-arch-traps`.

## Model

- x86_64 stays the system ISA for privilege, paging, faults, interrupts,
  atomics, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode instruction frontends over the same memory
  space, not separate emulated machines.
- Cross-ISA calls target real native ABIs: SysV x86_64, AAPCS64, RISC-V psABI.
- Register-only calls use ABI signature slots; complex calls use software
  thunks.
- Native returns cross ISA boundaries through transition cookies; recoverable
  foreign exits use OS-neutral trap packets.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Control Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Decoded control instructions, not `#UD` envelopes.

More detail: [README.md](../README.md), [design notes](poly-isa-design-directions.md),
[CPUID ABI](../tools/include/polycpuid.h), [Bochs implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc).
