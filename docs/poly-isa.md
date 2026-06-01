# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system. All code shares one x86_64 virtual address space. Full rationale: `docs/poly-isa-design-directions.md`.

## Frontends

| ID | ISA | Fetch model |
| --- | --- | --- |
| `0` | x86_64 | variable-length x86 fetch |
| `1` | AArch64 | 4-byte aligned direct fetch |
| `2` | RISC-V64 | 16/32-bit direct fetch with RVC |

## Controls

Decoded controls, not `#UD` envelopes:

```text
x86_64    0f 3a fc <subop>
AArch64   0xd503201f | ((subop & 0x7f) << 5)
RISC-V64  0x0000700b | ((subop & 0x7f) << 25)
```

| Subop | Name | Purpose |
| --- | --- | --- |
| `0x03` | `PENTER` | Enter a frontend from trusted runtime code |
| `0x04` | `PSWITCH` | Tail-switch frontends |
| `0x05` | `PLANDING` | Mark or validate an indirect landing target |
| `0x2d` | `PCALL` | Cross-call with explicit ABI signature |
| `0x30..0x3c` | `PCALL_SLOT` | Cross-call with cached signature slot `0..12` |
| `0x62` | `PTRAPRET` | Return from a user Poly trap monitor |
| `0x65..0x6e` | `STATE` | State key, ABI slots, monitor, and landing policy |

## Rules

- x86_64 owns boot, privilege, paging, interrupts, scheduling, faults, kernel syscalls, and TSO memory ordering.
- Foreign state is per-thread XSAVE-style architectural state, not hidden emulator state.
- Fast calls use register-only ABI signature remapping.
- Runtime thunks handle stack args, aggregates, variadics, incompatible vectors, and memory-shaped ABI layout work.
- `PCALL` records caller frontend, return PC, SP, and flags; applies the signature; installs a reserved native return cookie; and branches.
- Cross-ISA returns use ordinary native returns: x86_64 `ret`, AArch64 `ret x30`, and RISC-V64 `ret`.
- Hardware may switch frontends, remap registers, validate landing pads, and emit precise trap records.
- Hardware must not parse user-memory descriptors, repack stacks, implement libc/libgcc/libatomic, translate syscalls, or encode OS policy.
