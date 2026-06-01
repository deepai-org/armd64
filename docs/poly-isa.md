# Poly ISA Quick Reference

Poly lets existing x86_64, AArch64, and RISC-V64 user code share one virtual
address space. It is not a new source ABI; x86_64 remains the system ISA.

Longer rationale: `docs/poly-isa-design-directions.md`.

## Frontends

| ID | ISA | Fetch |
| --- | --- | --- |
| `0` | x86_64 | native variable-length |
| `1` | AArch64 | 4-byte aligned |
| `2` | RISC-V64 | 16/32-bit, including RVC |

x86_64 owns boot, privilege, paging, interrupts, faults, VM control, and the
global TSO memory model. AArch64 and RISC-V64 are user-mode peer frontends.

## Poly Opcodes

Poly control operations are real decoded instructions, not `#UD` envelopes.

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

## Calls

`PCALL` records caller frontend, return PC, SP, and flags; applies a
register-only ABI signature; installs a reserved native return cookie; and
branches to the target frontend.

Returns use ordinary native returns: x86_64 `ret`, AArch64 `ret x30`, and
RISC-V64 `ret`. Returning to the reserved cookie restores the caller frontend.

## Hardware Boundary

Hardware remaps registers and switches frontends. It does not parse user-memory
call descriptors, repack stacks, translate variadics, implement libc/libgcc, or
encode OS policy. Loader/runtime thunks handle those cases.

Poly state is explicit per-thread XSAVE-style state. Recoverable foreign traps
produce precise trap records; hard page faults, interrupts, scheduling, and real
syscalls remain owned by the kernel.
