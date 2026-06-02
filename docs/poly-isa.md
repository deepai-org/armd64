# Poly ISA Quick Reference

Poly is a Bochs prototype for running existing x86_64, AArch64, and RISC-V64
userspace code in one address space. It targets native ABI compatibility, not a
new compiler-only ABI.

For build commands and status, see `README.md`. For hardware/ABI rationale, see
`docs/poly-isa-design-directions.md`.

## Architecture

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, syscalls, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are peer user-mode frontends with native fetch rules:
  32-bit aligned AArch64 and 16/32-bit RISC-V including RVC.
- Frontend transitions are decoded control instructions, not `#UD` envelopes.
- Cross-ISA calls target real native ABIs: SysV x86_64, AAPCS64, and RISC-V
  psABI.
- Per-thread foreign state is architectural XSAVE-style state. Bochs fallback
  banks are prototype/debug machinery only.
- Hardware handles frontend switching, register-only ABI signatures, precise
  trap packets, and trap return. Software handles stack/aggregate ABI work,
  lazy binding, syscall policy, libc/libgcc/libatomic helpers, and other
  user-memory reshaping.

## Prototype Controls

Temporary Bochs encodings model dedicated silicon controls. They are not final
architecture numbers.

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

Control targets must be canonical and valid for the target frontend's fetch
alignment. Monitor packet addresses must be canonical and qword-aligned.
