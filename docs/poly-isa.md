# Poly ISA

Poly is a hardware-style extension for running existing x86_64, AArch64, and
RISC-V64 userspace code in one virtual address space. Compatibility with real
native ABI objects is the priority; this is not a new compiler-only ABI.

For build/test commands, see `README.md`. For design rationale, see
`docs/poly-isa-design-directions.md`.

## Model

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, faults,
  VM control, atomics, syscalls, and global TSO memory ordering.
- AArch64 and RISC-V64 are additional user-mode decode frontends over the same
  virtual address space.
- Frontend changes use decoded control instructions. There are no legacy
  per-instruction `#UD` envelopes.
- AArch64 fetch is fixed 32-bit. RISC-V64 fetch supports 16/32-bit instructions
  including RVC.
- AArch64 and RISC-V64 may call/switch directly; x86_64 is not a required
  trampoline frontend.

## Compatibility

- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast calls use register exchange/ABI signature slots for arguments and return
  values that already fit in registers.
- Software thunks handle stack arguments, aggregates, variadics, lazy binding,
  libc policy, and syscall-number/policy translation.
- Foreign register state is per-thread XSAVE-style architectural state. Hidden
  Bochs banks are prototype fallback/debug machinery only.
- Foreign `svc`, `ecall`, breakpoints, illegal instructions, and frontend
  faults produce OS-neutral trap packets for a runtime or OS handler.

## Controls

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
| `0x65..0x6e` | setup/query controls |

## Boundary

Hardware should stay fixed-latency: switch frontends, fetch/decode raw foreign
instructions, alias register arguments, deliver trap packets, and expose state
through XSAVE. Anything that requires interpreting user memory remains software.
