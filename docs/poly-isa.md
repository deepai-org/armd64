# Poly ISA Quick Reference

Poly adds direct-fetch AArch64 and RISC-V64 user frontends to an x86_64
machine. x86_64 still owns boot, paging, privilege, interrupts, scheduling,
syscalls, atomics, and TSO ordering.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused smoke tests: `boot-poly`, `boot-poly-call-arch-traps`,
`boot-poly-binfmt-arch-traps`, `boot-poly-neutral-arch-traps`.

## Architectural Delta From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 uses normal variable-length decode; foreign modes fetch native
  aligned 32-bit instructions from `RIP`.
- State: non-x86 architectural registers are XSAVE-style state, not hidden
  emulator state.
- Calls: fast calls use register-only ABI signature slots; software thunks
  handle stack args, aggregates, variadics, lazy binding, and policy.
- Traps: foreign traps produce OS-neutral trap packets. Hardware must not know
  Linux, libc, libgcc, libatomic, or dynamic-linker semantics.

## Temporary Opcode Space

| Op | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| Base | `0f 3a fc <subop>` | `0xd503201f | (subop << 5)` | `0x0000700b | (subop << 25)` |
| `PENTER` | `0x03` | n/a | n/a |
| `PSWITCH` | `0x04` | `0x78` | `8` |
| `PCALL` | `0x2d` | `0x7a` | `10` |
| `PCALL_SIG_IMM` | `0x2e <slot>` | `0x60 + slot` | `16 + slot` |
| `PTRAPRET` | `0x62` | `0x76` | `6` |
| `PLANDING` | `0x05` | `0x7b` | `11` |

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
