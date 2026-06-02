# Poly ISA

Quick reference for the Bochs prototype. Design rationale lives in
`docs/poly-isa-design-directions.md`.

## Purpose

Run existing precompiled x86_64, AArch64, and RISC-V64 user code in one
x86_64 virtual address space without per-instruction trap envelopes.

## Run

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused gates: `boot-poly-apps-arch-traps`,
`boot-poly-call-real-xsave-arch-traps`, and
`boot-poly-binfmt-arch-traps`.

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, syscalls, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are peer user-mode decode frontends fetched from the
  same virtual address space.
- Frontend switches are decoded control instructions, not `#UD` exceptions.
- AArch64 uses 32-bit aligned fetch. RISC-V64 supports 16/32-bit fetch.
- `PCALL` can switch ISA and apply cached native-ABI register aliases for
  x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI calls.
- Software handles memory-shaped ABI work: stack arguments, aggregates,
  variadics, lazy binding, syscalls, libcalls, and debugger policy.
- Foreign state is per-thread XSAVE-style architectural state.
- Recoverable foreign traps write OS-neutral trap records.

## Prototype Control Encodings

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops include `PENTER`, `PSWITCH`, `PCALL`, signature-slot calls, `PLANDING`,
`PTRAPRET`, setup, and query operations.
