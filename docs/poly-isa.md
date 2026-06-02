# Poly ISA

Poly is a hardware-style extension for running existing x86_64, AArch64, and
RISC-V64 userspace code in one virtual address space. It targets real native
ABI compatibility, not a new compiler-only ABI.

For build/test commands, see `README.md`. For design rationale, see
`docs/poly-isa-design-directions.md`.

## Execution Model

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, faults,
  VM control, atomics, syscalls, and global TSO memory ordering stay x86-owned.
- AArch64 and RISC-V64 are user-mode peer frontends fetched from the same
  address space; they may switch/call each other without x86 as a trampoline.
- Frontend changes are decoded control instructions. The fast path has no
  per-instruction `#UD` envelopes.
- Fetch rules stay native: x86_64 is variable length, AArch64 is 32-bit fixed
  width, and RISC-V64 supports 16/32-bit fetch including RVC.

## Compatibility

- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Hardware handles fixed-latency frontend switches, register exchange/ABI
  signature slots, precise trap packets, and XSAVE-style state.
- Software thunks handle stack arguments, aggregates, variadics, lazy binding,
  libc/syscall policy, and other memory-shaped ABI work.
- Foreign state is per-thread architectural state. Bochs fallback banks are
  prototype/debug machinery only.

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

## Hardware Boundary

Hardware must not parse user-memory call descriptors, repack stacks, emulate OS
policy, or implement libc/libgcc/libatomic helpers. Anything that requires
interpreting user memory remains runtime/loader software.
