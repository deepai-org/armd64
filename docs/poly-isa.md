# Poly ISA Quick Reference

Poly is an x86_64 CPU extension prototype for running existing precompiled AArch64 and RISC-V64 userspace code in one virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_POLY_REAL_XSAVE_OK|POLYBENCH_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused targets: `make boot-poly`, `make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`, `make boot-poly-neutral-arch-traps`.

## ISA Differences From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts, scheduling, syscalls, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode frontends fetching native instructions directly from the process address space.
- Frontend IDs are `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fast paths use decoded control instructions, not one `ud2` envelope per foreign instruction.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Register-only ABI signature slots are the hardware fast path; software thunks handle stack arguments, aggregates, variadics, lazy binding, and policy.
- Foreign architectural state is explicit XSAVE-style state, not hidden emulator state.
- Foreign traps produce OS-neutral trap packets. Hardware does not emulate Linux, libc, libgcc, libatomic, or dynamic-linker policy.

## Prototype Encodings

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

| Operation | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `PENTER` | `0x03` | n/a | n/a |
| `PSWITCH` | `0x04` | `0x78` | `8` |
| `PCALL` | `0x2d` | `0x7a` | `10` |
| `PCALL_SIG_IMM` | `0x2e <slot>` | `0x60 + slot` | `16 + slot` |
| `PTRAPRET` | `0x62` | `0x76` | `6` |
| `PLANDING` | `0x05` | `0x7b` | `11` |

## References

- Status and commands: [README.md](../README.md)
- Hardware/ABI rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- CPUID/XSAVE contract: [polycpuid.h](../tools/include/polycpuid.h)
