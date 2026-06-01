# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 CPU so existing native objects can run in one address space. It is not a new portable compiler ABI; see `docs/poly-isa-design-directions.md` for rationale.

## Model

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA: boot, paging, privilege, interrupts, kernel syscalls, atomics, VM control, and TSO memory ordering.
- Foreign frontends fetch native instructions from `RIP`: AArch64 as aligned 32-bit words, RISC-V64 as 16/32-bit instructions with RVC.
- Cross-ISA calls target native ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Poly state is per-thread XSAVE-style state.

## Controls

Decoded controls, not `#UD` trap envelopes:

```text
x86_64    0f 3a fc <subop>
AArch64   0xd503201f | ((subop & 0x7f) << 5)
RISC-V64  0x0000700b | ((subop & 0x7f) << 25)
```

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

- Hardware: frontend switching, register-only ABI signatures, transition stack, return cookies, landing validation, precise trap packets.
- Native returns (`ret`, `ret x30`, RISC-V `ret`) cross back through the return cookie installed by `PCALL`.
- Runtime: stack arguments, aggregates, variadics, incompatible vectors, lazy binding, syscalls, imports, and helper libraries.
- Never in hardware: user-memory call descriptors, stack repacking, syscall translation, libc/libgcc/libatomic policy, or OS policy.
