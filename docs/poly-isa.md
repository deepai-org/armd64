# Poly ISA

Poly adds AArch64 and RISC-V64 user frontends to an x86_64 CPU. The goal is
existing precompiled code and native-ABI shared libraries in one x86_64 virtual
address space.

## Run

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused gates: `boot-poly-apps-arch-traps`,
`boot-poly-call-real-xsave-arch-traps`, and `boot-poly-binfmt-arch-traps`.

## x86_64 Differences

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, syscalls, VM control, and global TSO ordering.
- AArch64 and RISC-V64 are peer user-mode fetch/decode frontends in the same
  virtual address space.
- Frontend changes are decoded Poly control instructions, not `#UD` envelopes.
- `PCALL` supports native ABI register alias signatures for fast x86_64 SysV,
  AAPCS64, and RISC-V psABI calls.
- Complex ABI cases stay software-owned: stack args, aggregates, variadics,
  lazy binding, syscalls, libcalls, and debugger policy.
- Foreign state is per-thread XSAVE-style architectural state.
- Recoverable foreign traps produce OS-neutral trap records for a Ring 3
  runtime monitor or OS handler.

## Temporary Encodings

These Bochs encodings model silicon behavior. Real hardware should allocate
official opcode space with the same fixed-latency semantics.

| Frontend | Control encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Control subops include `PENTER`, `PSWITCH`, `PCALL`, signature-slot calls,
`PLANDING`, `PTRAPRET`, and setup/query operations.

Full hardware and ABI rationale: `docs/poly-isa-design-directions.md`.
