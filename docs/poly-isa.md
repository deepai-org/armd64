# Poly ISA

Poly is an x86_64 CPU extension that can fetch and execute existing AArch64 and
RISC-V64 user code in the same virtual address space. See
`docs/poly-isa-design-directions.md` for rationale.

## Frontends

| ID | ISA | Fetch rule |
| --- | --- | --- |
| `0` | x86_64 | native variable-length x86 |
| `1` | AArch64 | native 32-bit aligned |
| `2` | RISC-V64 | native 16/32-bit, including RVC |

x86_64 remains the system ISA for boot, privilege, paging, interrupts, kernel
syscalls, VM control, atomics, and the global TSO memory model.

## Control Encodings

Decoded controls, not `#UD` traps. Current Bochs prototype encodings:

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

## Contract

- Target compatibility is native ABI code: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Fast calls use register-only ABI signature slots, implemented as fixed-latency register-name remaps.
- Complex ABI cases use runtime thunks: stack args, aggregates, variadics, vectors, lazy binding, syscalls, and helper libraries.
- `PCALL` records caller frontend, PC, SP, and flags; applies the selected signature; installs a native return cookie; and branches.
- Native returns cross back through that cookie: x86_64 `ret`, AArch64 `ret x30`, and RISC-V64 `ret`.
- Poly state is per-thread XSAVE-style architectural state, not hidden CR3-scoped emulator state.
- Hardware may switch frontends, remap registers, validate landings, maintain the transition stack, and emit precise trap records.
- Hardware must not parse user-memory descriptors, repack stacks, implement libc/libgcc/libatomic, translate syscalls, or encode OS policy.
