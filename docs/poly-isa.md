# Poly ISA

Poly runs existing x86_64, AArch64, and RISC-V64 userspace code in one x86_64
virtual address space. x86_64 is the system ISA; AArch64 and RISC-V64 are
user-mode frontends.

## Run

```bash
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Use `make boot-poly` for a shorter regression.

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, faults, interrupts, VM control, atomics,
  and global TSO memory ordering.
- Foreign code fetches native instructions from `RIP`: AArch64 at 4-byte
  alignment; RISC-V64 at 2-byte alignment when RVC is enabled.
- Frontend transitions are decoded Poly control instructions, not `#UD`.
- Cross-ISA calls target real native ABIs. Register-only cases use ABI
  signature slots; stack, aggregate, variadic, and loader-policy cases use
  software thunks.
- Cross-ISA returns use native return instructions plus transition cookies.
- Foreign `svc`/`ecall`, breakpoints, unsupported instructions, and recoverable
  exits produce OS-neutral trap packets.

## Bochs Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

## References

- Constants and CPUID contract: [polycpuid.h](../tools/include/polycpuid.h)
- Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- Bochs implementation: [proc_ctrl.cc](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
