# Poly ISA

Poly is an x86_64 extension prototype for executing existing AArch64 and
RISC-V64 userspace code in the same virtual address space as x86_64 code.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_POLY_REAL_XSAVE_OK|POLYBENCH_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused targets: `boot-poly`, `boot-poly-call-arch-traps`,
`boot-poly-binfmt-arch-traps`, and `boot-poly-neutral-arch-traps`.

## Contract

- System ISA: x86_64 owns boot, privilege, paging, interrupts, scheduling,
  syscalls, atomics, and TSO ordering.
- User frontends: AArch64 and RISC-V64 fetch native 32-bit instructions from
  the process address space.
- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- State: non-x86 architectural state is explicit XSAVE-style state.
- Calls: fast register-only paths use ABI signature slots; thunks handle stack
  arguments, aggregates, variadics, lazy binding, and runtime policy.
- Traps: foreign traps produce OS-neutral trap packets. Hardware must not
  emulate Linux, libc, libgcc, libatomic, or dynamic linker behavior.

## Temporary Encodings

| Operation | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| Base format | `0f 3a fc <subop>` | `0xd503201f | (subop << 5)` | `0x0000700b | (subop << 25)` |
| `PENTER` | `0x03` | n/a | n/a |
| `PSWITCH` | `0x04` | `0x78` | `8` |
| `PCALL` | `0x2d` | `0x7a` | `10` |
| `PCALL_SIG_IMM` | `0x2e <slot>` | `0x60 + slot` | `16 + slot` |
| `PTRAPRET` | `0x62` | `0x76` | `6` |
| `PLANDING` | `0x05` | `0x7b` | `11` |

Detailed rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).
