# Poly ISA Quick Reference

Poly is a Bochs prototype of a hardware-style CPU extension that runs existing
x86_64, AArch64, and RISC-V64 userspace code in one x86_64 virtual address
space.

## Run

```bash
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Faster smoke test: `make boot-poly`.

## Differences From x86_64

- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  atomics, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode frontends that fetch native instructions
  directly from the shared address space.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast register-only calls use ABI signature slots; stack arguments,
  aggregates, variadics, lazy binding, and policy use software thunks.
- Cross-ISA returns use ordinary native return instructions plus hardware
  transition cookies.
- Recoverable foreign exits produce OS-neutral trap packets.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Prototype Encodings

- x86_64 Poly control: `0f 3a fc <subop>`
- AArch64 Poly HINT: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 custom-0: `0x0000700b | ((subop & 0x7f) << 25)`

These are decoded control instructions, not `#UD` envelopes.

Details: [README.md](../README.md), [design](poly-isa-design-directions.md),
[CPUID](../tools/include/polycpuid.h), [Bochs](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc).
