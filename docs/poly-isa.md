# Poly ISA

Poly extends x86_64 with AArch64 and RISC-V64 user-mode frontends. The goal is
to run existing precompiled cross-ISA code in one process address space.

## How To Run

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused checks:

```bash
make BOOT_TIMEOUT_SECONDS=900 boot-poly-call-real-xsave-arch-traps
make BOOT_TIMEOUT_SECONDS=900 boot-poly-binfmt-arch-traps
```

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  syscalls, atomics, and the effective memory model.
- AArch64 and RISC-V64 are raw user frontends that fetch native 32-bit
  instructions from `RIP`.
- Mode switches are decoded control operations, not `#UD` envelopes.
- Cross-ISA calls preserve real ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Extra foreign registers are per-thread XSAVE-style architectural state.
- Silicon should switch frontends and optionally remap register names. It should
  not parse user-memory descriptors, repack stacks, implement libc, or translate
  OS syscalls.
- Recoverable foreign traps produce OS-neutral trap packets for runtime or OS
  policy.

## Temporary Bochs Encodings

Prototype encodings only. Hardware needs allocated opcode space.

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
