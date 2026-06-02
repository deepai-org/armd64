# Poly ISA

Poly is a multi-frontend CPU extension for running existing x86_64, AArch64,
and RISC-V64 userspace code in one virtual address space. It targets ordinary
native ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  syscalls, VM control, atomics, and global TSO memory ordering.
- AArch64 and RISC-V64 are peer user-mode fetch/decode frontends, not
  coprocessors and not per-instruction `#UD` traps.
- Frontend switches are decoded control instructions with fixed-latency
  semantics suitable for hardware.
- Foreign register state is per-thread XSAVE-style architectural state.
- Recoverable foreign exits produce OS-neutral trap packets for a runtime or OS
  handler.
- Hardware may remap register names for ABI signatures. Hardware must not parse
  user-memory call descriptors, repack stacks, implement libc, or translate OS
  syscalls by policy.

## Prototype Encodings

Bochs uses temporary encodings that model dedicated hardware opcodes:

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Core subops:

| Subop | Operation |
| --- | --- |
| `0x03` | `PENTER` |
| `0x04` | `PSWITCH` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL` |
| `0x30..0x3c` | `PCALL_SLOT` |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | setup/query operations |

## Validate

```bash
make image
make boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Use `BOOT_TIMEOUT_SECONDS=900 make <target>` for longer gates. See `README.md`
for target descriptions and `docs/poly-isa-design-directions.md` for hardware
and ABI rationale.
