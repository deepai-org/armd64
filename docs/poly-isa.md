# Poly ISA Quick Reference

Poly is an x86_64 CPU extension prototype that can enter AArch64 and RISC-V64
userspace frontends in the same virtual address space. The goal is compatibility
with existing precompiled code, not a new compiler-only ISA.

## Run

```sh
make image
make boot-poly-probe-arch-traps
make boot-poly-real-xsave-arch-traps
```

Useful focused targets:

```sh
make boot-poly-apps-arch-traps
make boot-poly-call-arch-traps
make boot-poly-bench-arch-traps
```

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  scheduling, syscalls, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode execution frontends that fetch native
  precompiled instructions from the same process address space.
- Foreign frontend IDs are `1` for AArch64 and `2` for RISC-V64. x86_64 is `0`.
- Fast execution uses frontend switches, not one `ud2` envelope per foreign
  instruction.
- Cross-ISA calls use register-only ABI signatures for the hot path. Software
  thunks handle stack arguments, aggregates, variadics, lazy binding, and policy.
- Foreign architectural state is exposed as XSAVE-style state, not hidden
  emulator state keyed only by process address space.
- Recoverable foreign exits produce OS-neutral trap packets for a Ring 3 runtime.
  Hardware does not emulate Linux, libc, libgcc, libatomic, or loader policy.

## Prototype Control Encodings

These temporary encodings stand in for future dedicated silicon opcodes:

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Core operations:

| Operation | x86_64 subop | AArch64 subop | RISC-V64 subop |
| --- | --- | --- | --- |
| `PENTER frontend` | `0x03` | n/a | n/a |
| `PSWITCH frontend,target` | `0x04` | `0x78` | `8` |
| `PCALL frontend,target,signature` | `0x2d` | `0x7a` | `10` |
| `PCALL_SIG_IMM frontend,target,slot` | `0x2e <slot>` | `0x60 + slot` | `16 + slot` |
| `PTRAPRET` | `0x62` | `0x76` | `6` |
| `PLANDING` | `0x05` | `0x7b` | `11` |

The x86_64 immediate form is five bytes total: `0f 3a fc 2e <slot>`. AArch64
and RISC-V64 encode the signature slot inside the 32-bit control instruction.

## More Detail

- Project status and top-level commands: [README.md](../README.md)
- Hardware/ABI direction: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- Guest CPUID/XSAVE contract: [polycpuid.h](../tools/include/polycpuid.h)
- Bochs prototype decode: [proc_ctrl.cc](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
