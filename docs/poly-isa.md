# Poly ISA

Poly runs existing x86_64, AArch64, and RISC-V64 userspace code in one virtual
address space. The goal is native ABI compatibility, not a new compiler-only
ABI.

For build/test commands, see `README.md`. For design rationale, see
`docs/poly-isa-design-directions.md`.

## Contract

- x86_64 owns boot, privilege, paging, interrupts, faults, VM control, syscalls,
  atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode peer frontends in the same address space.
- Frontend switches/calls are decoded control instructions, not `#UD` traps.
- Native fetch rules are preserved: variable-length x86_64, 32-bit AArch64,
  and 16/32-bit RISC-V64 including RVC.
- Cross-ISA calls target real ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Per-thread foreign architectural state is saved/restored through XSAVE-style
  state. Bochs fallback banks are prototype/debug machinery only.
- Hardware may switch frontends, remap register-signature slots, deliver precise
  trap packets, and restore trap state.
- Software handles stack arguments, aggregates, variadics, lazy binding,
  syscall/libc policy, and any ABI work that requires reading user memory.
- Trap vectors must be valid frontend targets. Monitor packet addresses must be
  canonical and qword-aligned.

## Prototype Controls

Temporary Bochs encodings model dedicated silicon controls. They are prototype
opcode allocations, not final architecture numbers.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

| Subop | Control |
| --- | --- |
| `0x03` | `PENTER` |
| `0x04` | `PSWITCH` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL` |
| `0x30..0x3c` | `PCALL_SLOT` |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | setup/query |

Hardware must not parse user-memory call descriptors, repack stacks, emulate OS
policy, or implement libc/libgcc/libatomic helpers.
