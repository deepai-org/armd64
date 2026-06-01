# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 machine so existing code from all three ISAs can share one virtual address space. x86_64 remains the system ISA: boot, privilege, paging, faults, interrupts, scheduling, and kernel syscalls stay x86_64-owned.

Design rationale: `docs/poly-isa-design-directions.md`.

## Frontends

| ID | ISA | Fetch model |
| --- | --- | --- |
| `0` | x86_64 | normal variable-length x86 |
| `1` | AArch64 | 4-byte aligned direct fetch |
| `2` | RISC-V64 | 16/32-bit direct fetch, including RVC |

- One x86_64 virtual address space.
- One x86_64 TSO memory model.
- Per-thread Poly state saved with XSAVE-style architectural state.
- Cross-ISA calls use register signatures for fast arguments and runtime thunks for stack, aggregate, variadic, or ABI-specific layout work.

## Controls

Poly controls are decoded instructions, not `#UD` envelopes.

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)
RISC-V   0x0000700b | ((subop & 0x7f) << 25)
```

| Subop | Name | Meaning |
| --- | --- | --- |
| `0x03` | `PENTER` | Enter a frontend from trusted runtime code |
| `0x04` | `PSWITCH` | Tail-switch to another frontend |
| `0x05` | `PLANDING` | Mark or validate an indirect landing target |
| `0x2d` | `PCALL` | Cross-frontend call with selected ABI signature |
| `0x30..0x3c` | `PCALL_SLOT` | Cross-call using signature slot `0..12` |
| `0x62` | `PTRAPRET` | Return from a user-mode Poly trap monitor |
| `0x65..0x6e` | `STATE` | State key, ABI slots, monitor, and landing policy |

## Calls And Returns

`PCALL` records caller frontend, return PC, SP, and flags; applies a register-only ABI signature; installs a reserved native return cookie; and branches to the target frontend. Targets return with ordinary native returns: x86_64 `ret`, AArch64 `ret x30`, and RISC-V64 `ret`.

## Hardware Boundary

Hardware may switch frontends, remap register arguments, validate landing pads, and emit precise trap records. Hardware must not parse user-memory call descriptors, repack stacks, translate variadics, implement libc/libgcc, or encode OS policy.

Runtime thunks and user-mode monitors handle ABI and policy cases; hard faults, interrupts, scheduling, and real syscalls remain kernel-owned.
