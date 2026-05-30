# Poly ISA Quick Reference

Poly lets existing x86_64, AArch64, and RISC-V64 userspace code share one
x86_64 virtual address space. x86_64 remains the system ISA; AArch64 and
RISC-V64 are user-mode execution frontends.

## Run

```bash
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

For a faster smoke test:

```bash
make boot-poly
```

## What Differs From x86_64

- `RIP` is shared across frontends. AArch64 fetches 32-bit words; RISC-V64
  fetches 16-bit or 32-bit instructions.
- x86_64 owns privileged state: boot, paging, interrupts, exceptions, VM
  control, and global TSO memory ordering.
- Frontend switches use dedicated Poly control instructions, not `#UD`
  envelopes.
- Cross-ISA calls target native ABIs. Fast register-only calls use ABI
  signature slots; stack/aggregate/variadic cases use software thunks.
- Native return instructions can cross ISA boundaries through transition
  cookies.
- Foreign syscalls, breakpoints, unsupported instructions, and other recoverable
  exits produce OS-neutral trap packets.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Temporary Bochs Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

## Source Links

- Constants and CPUID contract: [polycpuid.h](../tools/include/polycpuid.h)
- Full design notes: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- Bochs implementation: [proc_ctrl.cc](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
