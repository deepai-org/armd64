# Poly ISA

Poly extends x86_64 with direct-fetch AArch64 and RISC-V64 user frontends.
The system ISA stays x86_64; foreign code runs in the same process address
space and uses explicit architectural state, not emulator-hidden process state.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused targets: `boot-poly`, `boot-poly-call-arch-traps`,
`boot-poly-binfmt-arch-traps`, `boot-poly-neutral-arch-traps`.

## How It Differs From x86_64

- Frontend select: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 remains variable-length; foreign modes fetch aligned 32-bit
  native instructions from `RIP`.
- System contract: x86_64 owns boot, privilege, paging, interrupts, scheduling,
  syscalls, atomics, and TSO memory ordering.
- State: non-x86 registers are explicit XSAVE-style architectural state.
- Calls: register-only ABI signature slots are the fast path; software thunks
  handle stack arguments, aggregates, variadics, lazy binding, and policy.
- Traps: foreign traps produce OS-neutral trap packets. Hardware must not know
  Linux, libc, libgcc, libatomic, or dynamic linker semantics.

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

Rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
