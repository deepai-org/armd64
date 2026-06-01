# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system for existing native ABI code in one shared virtual address space. Design rationale: `docs/poly-isa-design-directions.md`.

## Frontends

| ID | ISA | Fetch |
| --- | --- | --- |
| `0` | x86_64 | native variable-length x86 fetch |
| `1` | AArch64 | native 4-byte aligned fetch |
| `2` | RISC-V64 | native 16/32-bit fetch, including RVC |

x86_64 remains the system ISA for boot, privilege, paging, faults, interrupts, kernel syscalls, atomics, VM control, and global TSO memory ordering.

## Controls

Decoded controls, not `#UD` envelopes. Current Bochs prototype encodings:

```text
x86_64    0f 3a fc <subop>
AArch64   0xd503201f | ((subop & 0x7f) << 5)
RISC-V64  0x0000700b | ((subop & 0x7f) << 25)
```

| Subop | Name | Purpose |
| --- | --- | --- |
| `0x03` | `PENTER` | Enter a frontend from trusted runtime code. |
| `0x04` | `PSWITCH` | Tail-switch to another frontend. |
| `0x05` | `PLANDING` | Validate an indirect landing target. |
| `0x2d` | `PCALL` | Cross-ISA call with an explicit ABI signature. |
| `0x30..0x3c` | `PCALL_SLOT` | Cross-ISA call using cached signature slot `0..12`. |
| `0x62` | `PTRAPRET` | Return from a user Poly trap monitor. |
| `0x65..0x6e` | `STATE` | Configure state keys, ABI slots, monitors, and landing policy. |

## Contract

- Compatibility targets real native ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI. There is no required custom `PolyFast` ABI.
- Fast paths use register-only ABI signature slots; hardware may implement them as fixed-latency register-name remaps.
- Runtime thunks handle stack arguments, aggregates, variadics, incompatible vectors, lazy binding, libc helpers, syscall policy, and other memory-shaped ABI work.
- `PCALL` records caller frontend, return PC, SP, and flags; applies the signature; installs a reserved native return cookie; and branches.
- Cross-ISA returns use ordinary native returns: x86_64 `ret`, AArch64 `ret x30`, and RISC-V64 `ret` / `jalr x0, ra, 0`.
- Foreign register/control state is per-thread XSAVE-style architectural state, not hidden CR3-scoped emulator state.
- Hardware may switch frontends, remap registers, validate landing pads, maintain the transition stack, and emit precise trap records.
- Hardware must not parse user-memory call descriptors, repack stacks, implement libc/libgcc/libatomic, translate syscalls, or encode OS policy.
- Recoverable exits such as foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports, and helper traps are delivered as OS-neutral trap records to a runtime or OS handler.
