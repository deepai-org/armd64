# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 machine so precompiled code from all three ISAs can share one virtual address space.

x86_64 remains the system ISA: boot, rings, paging, interrupts, faults, VM control, scheduling, and the kernel syscall ABI stay x86_64-owned. Design rationale: `docs/poly-isa-design-directions.md`.

## What Differs From x86_64

| ID | ISA | Fetch |
| --- | --- | --- |
| `0` | x86_64 | native variable-length |
| `1` | AArch64 | 4-byte aligned |
| `2` | RISC-V64 | 16/32-bit, including RVC |

- All frontends use the same x86_64 virtual memory and TSO memory model.
- AArch64/RISC-V64 run only as user-mode frontends, not as alternate kernels.
- Cross-ISA ABI compatibility uses register signatures plus runtime thunks when stack or aggregate layout differs.
- Poly state is explicit per-thread XSAVE-style architectural state.

## Control Encodings

Poly controls are decoded instructions, not `#UD` envelopes:

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)
RISC-V   0x0000700b | ((subop & 0x7f) << 25)
```

| Subop | Name | Purpose |
| --- | --- | --- |
| `0x03` | `PENTER` | Runtime/trusted entry into a frontend |
| `0x04` | `PSWITCH` | Tail-switch to another frontend |
| `0x05` | `PLANDING` | Validate indirect cross-frontend landing targets |
| `0x2d` | `PCALL` | Cross-frontend call using selected ABI signature |
| `0x30..0x3c` | `PCALL_SLOT` | Cross-frontend call using slot `0..12` |
| `0x62` | `PTRAPRET` | Return from a Poly trap/monitor packet |
| `0x65..0x6e` | `STATE` | State key, ABI slots, monitor, landing policy |

## Cross-ISA Calls

`PCALL` records caller frontend, return PC, SP, and flags; applies a register-only ABI signature; installs a reserved native return cookie; and branches to the target frontend.

Returns use ordinary native returns: x86_64 `ret`, AArch64 `ret x30`, and RISC-V64 `ret`. Returning to the reserved cookie restores the caller frontend.

## Hardware Contract

Hardware may switch frontends, remap register arguments, validate landing pads, and produce precise trap records.

Hardware must not parse user-memory call descriptors, repack stacks, translate variadics, implement libc/libgcc, or encode OS policy. Loader/runtime thunks and user-mode monitors handle those cases; hard faults, interrupts, scheduling, and real syscalls remain kernel-owned.
