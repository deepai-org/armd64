# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system. All code shares one x86_64 virtual address space; x86_64 still owns boot, privilege, paging, interrupts, scheduling, faults, and kernel syscalls.

Full rationale: `docs/poly-isa-design-directions.md`.

## Frontends

| ID | ISA | Fetch model |
| --- | --- | --- |
| `0` | x86_64 | normal variable-length fetch |
| `1` | AArch64 | 4-byte aligned direct fetch |
| `2` | RISC-V64 | 16/32-bit direct fetch, including RVC |

## Control Encodings

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)
RISC-V   0x0000700b | ((subop & 0x7f) << 25)
```

| Subop | Name | Meaning |
| --- | --- | --- |
| `0x03` | `PENTER` | Enter a frontend from trusted runtime code |
| `0x04` | `PSWITCH` | Tail-switch frontends |
| `0x05` | `PLANDING` | Mark or validate an indirect landing target |
| `0x2d` | `PCALL` | Cross-call with selected ABI signature |
| `0x30..0x3c` | `PCALL_SLOT` | Cross-call with cached signature slot `0..12` |
| `0x62` | `PTRAPRET` | Return from a user Poly trap monitor |
| `0x65..0x6e` | `STATE` | State key, ABI slots, monitor, and landing policy |

## Rules

- Memory model: x86_64 TSO.
- Foreign state: per-thread, XSAVE-style architectural state.
- `PCALL` records caller frontend, return PC, SP, and flags; applies a register signature; installs a reserved native return cookie; and branches to the target frontend.
- Targets return with ordinary native returns: x86_64 `ret`, AArch64 `ret x30`, and RISC-V64 `ret`.
- Fast ABI path is register-only signature remapping.
- Runtime thunks handle stack arguments, aggregates, variadics, and ABI layout differences.
- Hardware may switch frontends, remap register arguments, validate landing pads, and emit precise trap records.
- Hardware must not parse user-memory call descriptors, repack stacks, translate variadics, implement libc/libgcc, or encode OS policy.
