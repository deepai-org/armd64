# Poly ISA Quick Reference

Poly lets existing x86_64, AArch64, and RISC-V64 code run in one x86_64 process.
It is not a new compiler ABI. Full rationale: `docs/poly-isa-design-directions.md`.

## Scope

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, privilege, paging, interrupts, kernel entry, atomics, VM
  control, and global TSO memory ordering.
- Foreign frontends are user-mode only and fetch native instructions from `RIP`.
- ABI target: ordinary SysV x86_64, AAPCS64, and RISC-V psABI objects.
- Poly state is explicit per-thread XSAVE-style state.

## Prototype Controls

Decoded controls, not `#UD` envelopes:

| Frontend | Temporary encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

| Subop | Operation |
| --- | --- |
| `0x03` | `PENTER frontend` |
| `0x04` | `PSWITCH frontend, target` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL frontend, target, signature` |
| `0x30..0x3c` | `PCALL_SLOT 0..12` |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | state/control setup |

## Boundary

- Hardware: fixed-latency frontend switches, register-only ABI signatures,
  landing validation, return cookies, transition-stack recovery, trap packets.
- Returns: x86_64 `ret`, AArch64 `ret x30`, and RISC-V `ret` cross through the
  `PCALL` return cookie.
- Runtime: stack arguments, aggregates, variadics, vectors, lazy binding,
  imports, helper libraries, and syscall translation.
- Not hardware: user-memory call descriptors, stack repacking, libc/libgcc/
  libatomic policy, syscall policy, or OS policy.
