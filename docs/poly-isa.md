# Poly ISA

Poly is an x86_64 CPU extension prototype that adds AArch64 and RISC-V64
user-mode frontends. The goal is compatibility with existing precompiled
objects in one process address space, not a new compiler-only ABI.

Long-form rationale is in `docs/poly-isa-design-directions.md`.

## How To Run

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused targets:

```bash
make BOOT_TIMEOUT_SECONDS=900 boot-poly-call-real-xsave-arch-traps
make BOOT_TIMEOUT_SECONDS=900 boot-poly-binfmt-arch-traps
```

## How It Differs From x86_64

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, syscalls,
  atomics, and the effective memory model stay x86_64-owned.
- AArch64 and RISC-V64 are raw user-mode instruction frontends. They fetch and
  decode native 32-bit instructions directly from `RIP`.
- Mode switches are decoded control operations, not `#UD` exception envelopes.
- Cross-ISA calls target the real platform ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Foreign non-aliased registers are per-thread architectural state, modeled as
  XSAVE-style state rather than CR3-scoped hidden emulator data.
- Hardware may remap register names for cached ABI signatures, but it must not
  parse user-memory call descriptors, repack stack layouts, implement libc, or
  translate syscalls.
- Recoverable foreign traps produce OS-neutral trap packets for runtime or OS
  policy code.

## Temporary Bochs Encodings

These encodings are prototype encodings for Bochs. A hardware ISA would allocate
real opcode space.

| ISA | Control encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

| Subop | Meaning |
| --- | --- |
| `0x03` | `PENTER` |
| `0x04` | `PSWITCH` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL` |
| `0x30..0x3c` | `PCALL_SLOT` |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | setup/query |
