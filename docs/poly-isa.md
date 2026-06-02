# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system CPU so
existing native objects can share one process address space.

## Architectural Contract

- Compatibility target: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- x86_64 owns boot, privilege, paging, interrupts, syscalls, atomics, and TSO.
- AArch64/RISC-V execute by direct raw fetch/decode, not per-instruction traps.
- Frontend switches are real decoded control operations, not `#UD` envelopes.
- Foreign register state is per-thread XSAVE-style architectural state.
- Recoverable exits report OS-neutral trap packets to runtime/OS policy code.
- Hardware may remap register names for ABI signatures, but must not parse
  user-memory descriptors, repack stacks, implement libc, or translate syscalls.

## Temporary Bochs Encodings

| ISA | Control encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

| Subop | Meaning |
| --- | --- |
| `0x03` | `PENTER` |
| `0x04` | `PSWITCH` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL` |
| `0x30..0x3c` | `PCALL_SLOT` |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | setup/query |

## Validation

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Design rationale lives in `docs/poly-isa-design-directions.md`.
